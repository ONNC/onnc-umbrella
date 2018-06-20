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

#include <onnc/Target/TG/BM188x/common_calibration2.pb.h>

namespace onnc {
ModulePass *createCalibrationPass(const std::string pDBName, int pI);
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
  Calibration(const std::string pDBName, int pI)
      : ModulePass(ID), m_DBName(pDBName), m_Iteration(pI)
  {
  }

  ~Calibration() override { delete m_Workspace; }

  Pass::ReturnType runOnModule(::onnc::Module &pModule) override;

private:
  float calculateKLD(const std::string &pBlobName);

  void updateThreshold(caffe2::NetDef &pDef);

  void profileModel(int pIteration, caffe2::NetDef &pDef,
                    const std::string &pDataLayer);

  bool readDataset(const std::vector<int64_t> &pInputDims,
                   const std::string &pDataLayer, int pIteration,
                   const Module::MetaDataMap &pMetaData);

  void thresholdFold(caffe2::NetDef &pDef);

  void Conv(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
            tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam);
  void Eltwise(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
               tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam);
  void Pool(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
            tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam);
  void PRelu(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
             tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam);
  void SpatialBN(const caffe2::OperatorDef &pOp, caffe2::NetDef &pDef,
                 tg::bm1880::LayerCalibrationParameter *pLayerCalibrationParam);

private:
  const std::string m_DBName;
  caffe2::Workspace *m_Workspace;

  using QWeightData = std::unordered_map<std::string, std::vector<int8_t> >;
  QWeightData m_QWeights;

  using QBiasData = std::unordered_map<std::string, std::vector<int16_t> >;
  QBiasData m_QBias;

  using BlobData =
      std::unordered_map<std::string, std::vector<caffe2::TensorCPU> >;
  BlobData m_BlobData;

  std::unordered_map<std::string, float> m_ThresholdY;

  tg::bm1880::NetCalibrationParameter m_NetCtableParam;
  int m_Iteration;
};
} // namespace onnc
#endif // CALIBRATION_H
