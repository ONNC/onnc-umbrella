//===- LayerImpl.h --------------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
void Calibration::Conv(
    const OperatorDef &pOp, caffe2::NetDef &pDef,
    tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam)
{
  const string &inputName = pOp.input(0);
  const string &weightName = pOp.input(1);
  const string &outputName = pOp.output(0);

  float thresX = m_ThresholdY[inputName];
  float thresY = m_ThresholdY[outputName];

  // Get weight and calculate right shift.
  auto wBlob = m_Workspace->GetBlob(weightName);
  int rightShift = getRightShift(wBlob, thresX / thresY);

  quantizeWeight<int8_t>(wBlob, thresX, thresY, 1 << rightShift, weightName);

  if (pOp.input_size() == 3) {
    const string &biasName = pOp.input(2);
    auto bBlob = m_Workspace->GetBlob(biasName);
    quantizeWeight<int16_t>(bBlob, 128, thresY, 1 << rightShift, biasName);
  }

  // Setup Ctable parameters.
  pLayerCalibrationParam->set_right_shift_width(rightShift);
}

void Calibration::Pool(
    const OperatorDef &pOp, caffe2::NetDef &pDef,
    tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam)
{
  const string &inputName = pOp.input(0);
  const string &outputName = pOp.output(0);

  float thresX = m_ThresholdY[inputName];
  float thresY = m_ThresholdY[outputName];
  int rightShift;
  int multiplier = getMultiplier(thresX, thresY, &rightShift);

  pLayerCalibrationParam->set_right_shift_width(rightShift);
  pLayerCalibrationParam->add_threshold_x_quantized(multiplier);
}

void Calibration::Eltwise(
    const OperatorDef &pOp, caffe2::NetDef &pDef,
    tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam)
{
  const int input_num = pOp.input_size();
  const string &outputName = pOp.output(0);
  float *thresX = new float[input_num];
  float thresY = m_ThresholdY[outputName];
  int *rightShift = new int[input_num];
  int *thres_x_quantized = new int[input_num];

  for (int i = 0; i < input_num; ++i)
    thresX[i] = m_ThresholdY[pOp.input(i)];

  // calculate right shift
  if (pOp.type() == "Sum" || pOp.type() == "Max") {
    for (int i = 0; i < input_num; ++i)
      rightShift[i] =
          calRightShift(std::vector<float>{ thresX[i] / thresY }, 1.0);
  } else if (pOp.type() == "Mul") {
    float thres_x_cal = 128.0;
    for (int i = 0; i < input_num; ++i)
      thres_x_cal *= thresX[i] / 128.0;
    rightShift[0] = calRightShift(std::vector<float>{ thres_x_cal }, 1.0);
  }

  // use min rightShift as final rightShift
  for (int i = 1; i < input_num; ++i) {
    if (rightShift[0] > rightShift[i])
      rightShift[0] = rightShift[i];
  }
  pLayerCalibrationParam->set_right_shift_width(rightShift[0]);

  // calculate thres_x_quantized
  if (pOp.type() == "Sum" || pOp.type() == "Max") {
    for (int i = 0; i < input_num; ++i) {
      thres_x_quantized[i] = (int)((thresX[i] / thresY) * (1 << rightShift[0]));
      pLayerCalibrationParam->add_threshold_x_quantized(thres_x_quantized[i]);
    }
  } else if (pOp.type() == "Mul") {
    // one 128.0 is used for quantized y in the equation
    // so we init thres_x_cal as 128.0 to eliminate one 128.0 in denominator
    float thres_x_cal = 128.0;
    for (int i = 0; i < input_num; ++i)
      thres_x_cal *= thresX[i] / 128.0;
    thres_x_quantized[0] = (int)(thres_x_cal * (1 << rightShift[0]));
    pLayerCalibrationParam->add_threshold_x_quantized(thres_x_quantized[0]);
  }

  delete[] thresX;
  delete[] rightShift;
  delete[] thres_x_quantized;
}

void Calibration::PRelu(
    const OperatorDef &pOp, caffe2::NetDef &pDef,
    tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam)
{
  const string &inputName = pOp.input(0);
  const string &slopeName = pOp.input(1);
  const string &outputName = pOp.output(0);
  caffe2::Blob *sBlob = m_Workspace->GetBlob(slopeName);
  const caffe2::TensorCPU sTensor = sBlob->Get<TensorCPU>();

  float thresX = m_ThresholdY[inputName];
  float thresY = m_ThresholdY[outputName];
  int gt_rshift = 0;
  int gt_scale = 0;
  int gt_shift_scale = 0;
  int le_rshift = 0;
  int le_shift_scale = 0;
  tg::bm1880::PReLUCalibrationParameter *prelu_cal_param =
      pLayerCalibrationParam->mutable_prelu_param();

  // calculate gt_scale and gt_tshift
  gt_rshift = calRightShift(std::vector<float>{ 1.0 }, thresX / thresY);
  if (gt_rshift < 0)
    gt_shift_scale = (int)(1.0 / (1 << (-gt_rshift)));
  else
    gt_shift_scale = (1 << gt_rshift);
  gt_scale = (int)floor((((thresX / thresY) * gt_shift_scale) + 0.5));
  gt_scale = saturate<int8_t>(gt_scale);
  prelu_cal_param->set_gt_scale(gt_scale);
  prelu_cal_param->set_gt_right_shift_width(gt_rshift);

  // calculate le_rshift
  auto slopes = sTensor.data<float>();
  int size = sTensor.size();
  le_rshift =
      calRightShift(std::vector<float>(slopes, slopes + size), thresX / thresY);
  prelu_cal_param->set_le_right_shift_width(le_rshift);

  // quantize slope weights
  if (le_rshift < 0)
    le_shift_scale = (int)(1.0 / (1 << (-le_rshift)));
  else
    le_shift_scale = (1 << le_rshift);
  quantizeWeight<int8_t>(sBlob, thresX, thresY, le_shift_scale, slopeName);
}
