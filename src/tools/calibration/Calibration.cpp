//===- Calibration.cpp ----------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <onnc/ADT/Color.h>
#include <onnc/Core/ModulePass.h>
#include <onnc/Support/IOStream.h>

#include "Calibration.h"
#include "Kld.h"

#include <fstream>
#include <streambuf>
#include <string>

#include <caffe2/onnx/backend.h>

using namespace onnc;
using namespace caffe2;

namespace onnc {
static int getRightShift(Blob *pBlob, float pScale)
{
  int m = 0;
  auto tensor = pBlob->Get<TensorCPU>();
  if (tensor.IsType<float>()) {
    const auto &probs = tensor.data<float>();
    auto size = tensor.size();
    // Find max abs in the tensor.
    float max = *std::max_element(probs, probs + size);
    float min = *std::max_element(probs, probs + size);
    float abs_max = (std::abs(min) > max) ? std::abs(min) : max;
    float data_max = abs_max * pScale;
    if (data_max <= 0) {
      std::cout << "data_max = " << data_max << std::endl;
      throw std::runtime_error("data_max <= 0");
    }
    while (data_max < 64) {
      m += 1;
      data_max *= 2;
    }
    while (data_max >= 128) {
      std::cout << "data_max = " << data_max << std::endl;
      std::cout << "Error in quantize_right_shift: rshift will be negative..."
                << data_max << std::endl;
      exit(-1);
    }
    return m;
  } else {
    // Assert.
    throw std::runtime_error("Blob format is not float!");
  }
  return m;
}

template <class T> static inline T saturate(int pValue)
{
  const int32_t max = std::numeric_limits<T>::max();
  const int32_t min = std::numeric_limits<T>::min();

  T sValue = std::min(max, pValue);
  sValue = std::max(min, pValue);

  return sValue;
}

static bool isBlobUsed(const string &pBlobName, caffe2::NetDef &pDef,
                       const int pOpIdx)
{
  bool start = false;
  int opIndex = 0;
  for (const OperatorDef &op : pDef.op()) {
    for (const string &in : op.input()) {
      if (start && (in == pBlobName)) {
        return true;
      }
      if ((in == pBlobName) && (opIndex == pOpIdx)) {
        start = true;
      }
    }
    opIndex++;
  }
  return false;
}

void Calibration::quantizeWeight(Blob *pBlob, float pThresX, float pThresY,
                                 int pRightShift, string pName)
{
  auto tensor = pBlob->Get<TensorCPU>();
  int shiftScale = 1 << pRightShift;
  if (tensor.IsType<float>()) {
    const auto &probs = tensor.data<float>();
    auto size = tensor.size();
    m_QWeights[pName].reserve(size);

    for (int i = 0; i < tensor.size(); i++) {
      float fWeight =
          floor(probs[i] * ((pThresX / pThresY) * shiftScale) + 0.5);
      int8_t qWeight = saturate<int8_t>((int)fWeight);
      m_QWeights[pName][i] = qWeight;
    }
  } else {
    throw std::runtime_error("Blob format is not float!");
  }
}

void Calibration::quantizeBias(Blob *pBlob, float pThresX, float pThresY,
                               int pRightShift, string pName)
{
  auto tensor = pBlob->Get<TensorCPU>();
  int shiftScale = 1 << pRightShift;
  if (tensor.IsType<float>()) {
    const auto &probs = tensor.data<float>();
    auto size = tensor.size();
    m_QBias[pName].reserve(size);

    for (int i = 0; i < tensor.size(); i++) {
      float fWeight =
          floor(probs[i] * ((pThresX / pThresY) * shiftScale) + 0.5);
      int16_t qWeight = saturate<int16_t>((int)fWeight);
      m_QBias[pName][i] = qWeight;
    }
  } else {
    throw std::runtime_error("Blob format is not float!");
  }
}

bool Calibration::readDataset(TensorCPU *pInputTensor, const string &pDataLayer,
                              int pIteration)
{
  // FIXME: Can read from onnx?
  constexpr static TIndex batch = 1;
  constexpr static TIndex channel = 3;
  constexpr static TIndex height = 224;
  constexpr static TIndex width = 224;
  constexpr auto nums = batch * channel * height * width;
  std::vector<TIndex> inputDims({ batch, channel, height, width });

  // FIXME: Should read from lmdb. If fail then return false.
  for (int run = 0; run < pIteration; run++) {
    std::vector<float> data;
    for (int i = 0; i < nums; i++) {
      float value = run * -100;
      data.emplace_back(value);
    }
    TensorCPU tensor(inputDims, data, nullptr);
    pInputTensor->ResizeLike(tensor);
    pInputTensor->ShareData(tensor);
    m_BlobData[pDataLayer].emplace_back(tensor);
  }

  return true;
}

float Calibration::calculateKLD(const string &pBlobName)
{
  int iteration = m_BlobData[pBlobName].size();
  int dataSize = 0;
  vector<float> dataCollect;
  // Count memory size to reserve.
  for (int i = 0; i < iteration; i++) {
    dataSize += m_BlobData[pBlobName][i].size();
  }
  dataCollect.reserve(dataSize);
  // Concat all batches' tensor.
  for (int i = 0; i < iteration; i++) {
    auto probs = m_BlobData[pBlobName][i].data<float>();
    dataCollect.insert(dataCollect.end(), probs,
                       probs + m_BlobData[pBlobName][i].size());
  }
  return KLDiversity(dataCollect.data(), dataCollect.size());
}

void Calibration::profileModel(int pIteration, caffe2::NetDef &pDef,
                               const string &pDataLayer)
{
  // Calculate "data layer" KLD.
  m_ThresholdY[pDataLayer] = calculateKLD(pDataLayer);

  int opIdx = 0;
  for (const OperatorDef &op : pDef.op()) {
    for (int run = 0; run < pIteration; run++) {
      // Prepare input blobs.
      for (const string &in : op.input()) {
        // Only feed data to input. (no needs to feed weights)
        if (m_BlobData.find(in) != m_BlobData.end()) {
          // Feed data from previous layer.
          auto input = m_Workspace->GetBlob(in)->GetMutable<TensorCPU>();
          input->ResizeLike(m_BlobData[in][run]);
          input->ShareData(m_BlobData[in][run]);
        }
      }
      CAFFE_ENFORCE(m_Workspace->RunOperatorOnce(op));
      // Save output blobs.
      for (const string &in : op.output()) {
        auto blob = m_Workspace->GetBlob(in);
        // Special case: Skip Dropout layer.
        // FIXME: Remove. Dropout layer should be deleted in layer-opt. phase.
        if (string(blob->TypeName()) != "nullptr (uninitialized)") {
          CPUContext cpuContext;
          TensorCPU output(m_Workspace->GetBlob(in)->Get<TensorCPU>(),
                           &cpuContext);
          m_BlobData[in].emplace_back(output);
        }
      }
    }
    // Calculate KLD. Save results in threshold_y.
    for (const string &in : op.output()) {
      auto blob = m_Workspace->GetBlob(in);
      // FIXME: Remove. Dropout layer should be deleted in layer-opt. phase.
      if (string(blob->TypeName()) != "nullptr (uninitialized)" &&
          blob->Get<TensorCPU>().IsType<float>()) {
        m_ThresholdY[in] = calculateKLD(in);
      }
    }
    // FIXME: Needs to check this input is unused anymore...
    // Free unused input blobs for saving memory.
    for (const string &in : op.input()) {
      if (m_BlobData.find(in) != m_BlobData.end()) {
        if (!isBlobUsed(in, pDef, opIdx)) {
          m_BlobData.erase(in);
        }
      }
    }
    opIdx++;
  }
}

#include "LayerImpl.h"
void Calibration::getRightShiftQuantize(caffe2::NetDef &pDef)
{
  for (const OperatorDef &op : pDef.op()) {
    // FIXME: caffe2 seems no layer name... maybe set "op.type()" + "idx".
    // layerCalibrationParam.set_name =
    if (op.type() == "Conv" || op.type() == "FC" || op.type() == "Scale") {
      Conv(op, pDef);
    }
    // TODO: Add other layers.
  }
}

Pass::ReturnType Calibration::runOnModule(Module &pModule)
{
  // TODO: Check If Ctable exist, then skip this pass.
  // FIXME: Sould be specified by user.
  constexpr int iteration = 5;

  std::ifstream ifs(m_FileName);
  std::string onnxStr((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());

  // Create caffe2 backend.
  auto *backend = new caffe2::onnx::Caffe2Backend(nullptr);
  std::vector<caffe2::onnx::Caffe2Ops> extras;
  auto rep = backend->Prepare(onnxStr, "CPU", extras);
  auto def = rep->pred_net();

  // Init net weights.
  m_Workspace = new Workspace("layer_calibration");
  CAFFE_ENFORCE(m_Workspace->RunNetOnce(rep->init_net()));

  // Find data layer's name.
  const OperatorDef &op = def.op(0);
  const string &dataLayer = op.input(0);
  auto inputTensor =
      m_Workspace->CreateBlob(dataLayer)->GetMutable<TensorCPU>();
  if (!readDataset(inputTensor, dataLayer, iteration)) {
    errs() << Color::RED << "Error" << Color::RESET << ": Read data set fail..."
           << std::endl;
    return kModuleNoChanged;
  }

  // Run inference and calculate KLD.
  profileModel(iteration, def, dataLayer);

  // TODO: Optimize threshold.

  // Caliculate right-shift each layer and Quantize weights.
  getRightShiftQuantize(def);

  // TODO: Write Ctable and qWeights into onnx.
  std::cout << m_NetCtableParam.DebugString() << std::endl;

  return kModuleChanged;
}
} // namespace onnc

char Calibration::ID = 0;
ModulePass *onnc::createCalibrationPass(std::string const &pFilename)
{
  return new Calibration(pFilename);
}
