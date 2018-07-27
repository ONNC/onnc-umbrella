//===- QuantizePass.tcc ---------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
template <class T> inline T saturate(int32_t pValue)
{
  const int32_t max = std::numeric_limits<T>::max();
  const int32_t min = std::numeric_limits<T>::min();

  pValue = std::min(max, pValue);
  T sValue = std::max(min, pValue);

  return sValue;
}

template <class T> void
onnc::BM188X::QuantizePass::quantizeWeight(std::vector<float> &pTensor,
                                           float pThresX, float pThresY,
                                           int pShiftScale,
                                           const std::string &pName)
{
  auto size = pTensor.size();
  if (0 < m_QWeights.count(pName)) {
    // pName is quantized already
    return;
  }

  if (0 < m_QBias.count(pName)) {
    // pName is quantized already
    return;
  }
  if (std::is_same<T, int8_t>::value) {
    m_QWeights[pName].reserve(size);
  } else if (std::is_same<T, int16_t>::value) {
    m_QBias[pName].reserve(size);
  } else {
    errs() << Color::RED << "Error" << Color::RESET
           << ": Quantized type not support!\n";
    assert(0);
  }
  for (size_t i = 0; i < pTensor.size(); i++) {
    float fWeight =
        floor(pTensor[i] * ((pThresX / pThresY) * pShiftScale) + 0.5);
    T qWeight = saturate<T>((int32_t)fWeight);

    if (std::is_same<T, int8_t>::value) {
      m_QWeights[pName].emplace_back(qWeight);
    } else if (std::is_same<T, int16_t>::value) {
      m_QBias[pName].emplace_back(qWeight);
    }
  }
}

