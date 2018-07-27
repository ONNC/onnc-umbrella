//===- QuantizePass.h -----------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNC_TARGET_BM188X_QUANTIZE_PASS_H
#define ONNC_TARGET_BM188X_QUANTIZE_PASS_H

#define DEBUG_TYPE "bm188x_quantize"
#include "BM188xBackend.h"
#include "ComputeOperator.h"
#include "TGConv.h"
#include "TGLRN.h"
#include "TGLeakyRelu.h"
#include "TLConv.h"
#include <onnc/ADT/Color.h>
#include <onnc/Core/ModulePass.h>
#include <onnc/Core/PassSupport.h>
#include <onnc/Support/Debug.h>
#include <onnc/Target/TG/BM188x/common_calibration2.pb.h>
#include <onnc/Target/TargetTransformInfo.h>

namespace onnc {
namespace BM188X {

class QuantizePass : public ModulePass
{
public:
  static char ID;

public:
  QuantizePass(BM1880Backend *pBackend);

  Pass::ReturnType runOnModule(Module &pModule) override;

private:
  template <class T>
  void quantizeWeight(std::vector<float> &pTensor, float pThresX, float pThresY,
                      int pShiftScale, const std::string &pName);

  void updateElementType(const std::vector<MemOperand *> &pMemOprds);

  void updateQuantizeWeight(onnx::Graph *pGraph);

  void TLConv(std::unique_ptr<ComputeOperator2> &pInst);
  void Conv(std::unique_ptr<ComputeOperator2> &pInst);
  void Gemm(std::unique_ptr<ComputeOperator2> &pInst);
  void Pool(std::unique_ptr<ComputeOperator2> &pInst);
  void PRelu(std::unique_ptr<ComputeOperator2> &pInst);
  void LeakyRelu(std::unique_ptr<ComputeOperator2> &pInst);
  void Eltwise(std::unique_ptr<ComputeOperator2> &pInst,
               const std::string &pTypeName);
  void Lrn(std::unique_ptr<ComputeOperator2> &pInst);
  void Concat(std::unique_ptr<ComputeOperator2> &pInst);
  void Scale(std::unique_ptr<ComputeOperator2> &pInst);

private:
  using QWeightData = std::unordered_map<std::string, std::vector<int8_t> >;
  using QBiasData = std::unordered_map<std::string, std::vector<int16_t> >;

private:
  QWeightData m_QWeights;
  QBiasData m_QBias;

  BM1880Backend *m_pBackend; // NOLINT
};

} // namespace BM188X

ModulePass *CreateQuantizePass(BM1880Backend *pBackend);

} // namespace onnc

#include "QuantizePass.tcc"

#endif
