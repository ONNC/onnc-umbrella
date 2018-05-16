//===- Calibration.h ------------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <onnc/Core/ModulePass.h>

#include <caffe2/core/init.h>
#include <caffe2/core/net.h>

#include "common_calibration.pb.h"

namespace onnc {
ModulePass *createCalibrationPass();
}

namespace onnc {
// Calibration Pass will:
// 1. Run inference via caffe2-backend.
// 2. Calibrate the range of each blobs. (Create Ctable)
// 3. Quantize weights.
class Calibration : public ModulePass
{
public:
  static char ID;
  Calibration() : ModulePass(ID) {}

  Pass::ReturnType runOnModule(Module &pModule) override;

private:
  float calculateKLD(const std::string &pBlobName);

  void getRightShiftQuantize(caffe2::NetDef &pDef);

  void profileModel(int pIteration, caffe2::NetDef &pDef,
                    const std::string &pDataLayer);

  template <class T>
  void quantizeWeight(caffe2::Blob *pBlob, float pThresX, float pThresY, int pRightShift,
                      caffe2::string pWName);

  bool readDataset(caffe2::TensorCPU *pInputTensor, const std::string &pDataLayer,
                   int pIteration);

  void updateQuantizeWeight(::onnx::Graph *pGraph);

  void thresholdFold(caffe2::NetDef &pDef);

  void Conv(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef);
  void Pool(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef);

private:
  caffe2::Workspace *m_Workspace;

  using QWeightData = std::unordered_map<std::string, std::vector<int8_t> >;
  QWeightData m_QWeights;

  using QBiasData = std::unordered_map<std::string, std::vector<int16_t> >;
  QBiasData m_QBias;

  using BlobData =
      std::unordered_map<std::string, std::vector<caffe2::TensorCPU> >;
  BlobData m_BlobData;

  std::unordered_map<std::string, float> m_ThresholdY;

  NetCalibrationParameter m_NetCtableParam;
};
} // namespace onnc
#endif // CALIBRATION_H
