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

  quantizeWeight<int8_t>(wBlob, thresX, thresY, rightShift, weightName);

  if (pOp.input_size() == 3) {
    const string &biasName = pOp.input(2);
    auto bBlob = m_Workspace->GetBlob(biasName);
    quantizeWeight<int16_t>(bBlob, 128, thresY, rightShift, biasName);
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
