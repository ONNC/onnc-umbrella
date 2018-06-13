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

#define DEBUG_BN 0
void Calibration::SpatialBN(
    const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
    tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam)
{
  const string &input_name = pOp.input(0);    // INPUT
  const string &scale_name = pOp.input(1);    // SCALE
  const string &bias_name = pOp.input(2);     // BIAS
  const string &mean_name = pOp.input(3);     // EST_MEAN
  const string &variance_name = pOp.input(4); // EST_VAR
  const string &output_name = pOp.output(0);  // OUTPUT
  float thresX = m_ThresholdY[input_name];
  float thresY = m_ThresholdY[output_name];

  // We can fuse the output computation as follows:
  //  (x - mean) * (inv_var) * scale + bias
  //  to
  //  (x * inv_var * scale) + (bias - mean * inv_var * scale)
  //
  // inv_var = (variance + epsilon).sqrt().inverse()

  std::vector<float> variance = getTensor(variance_name);
  std::vector<float> inv_var;
  float epsilon = 1e-5f;

  float variance_average =
      std::accumulate(variance.begin(), variance.end(), 0) / variance.size();
  std::vector<bool> disable_channel(variance.size(), false);
  float threshold_zero = variance_average / 256;
  for (size_t i = 0; i < variance.size(); ++i) {
    inv_var.push_back(1.0 / sqrt(variance[i] + epsilon));
    if (variance[i] < threshold_zero) {
      disable_channel[i] = true;
    }
  }

  // calculate new scale and bias
  std::vector<float> new_scale;
  std::vector<float> new_bias;
  std::vector<float> scale_var = getTensor(scale_name);
  std::vector<float> bias_var = getTensor(bias_name);
  std::vector<float> mean_var = getTensor(mean_name);
  for (size_t i = 0; i < inv_var.size(); ++i) {
    if (!disable_channel[i]) {
      new_scale.push_back(inv_var[i] * scale_var[i]);
      new_bias.push_back(bias_var[i] -
                         (mean_var[i] * scale_var[i]) * inv_var[i]);
    } else {
      new_scale.push_back(0);
      new_bias.push_back(0);
    }
  }
  int rshift = calRightShift(new_scale, thresX / thresY);
  assert(rshift >= 0);

  quantizeWeight<int8_t>(new_scale, thresX, thresY, 1 << rshift, scale_name);
  quantizeWeight<int16_t>(new_bias, 128, thresY, 1 << rshift, bias_name);
#if DEBUG_BN
  std::cout << "rshift:" << rshift << "\n";
  std::cout << "variance: [  \n";
  for (size_t i = 0; i < m_QWeights[scale_name].size(); ++i) {
    std::cout << i << ":" << (int)m_QWeights[scale_name][i] << "\n";
  }
  std::cout << "mean: [  \n";
  for (size_t i = 0; i < m_QBias[bias_name].size(); ++i) {
    std::cout << i << ":" << (int)m_QBias[bias_name][i] << "\n";
  }
#endif
  // Setup Ctable parameters.
  pLayerCalibrationParam->set_right_shift_width(rshift);
}
