#include "ONNXOptimizer.h"
#include <onnc/Core/ModulePass.h>
#include <onnc/IR/IRBuilder.h>
#include <onnc/IR/ONNXUtils.h>
#include <onnx/common/ir.h>
#include <onnx/optimizer/optimize.h>

using namespace onnc;

namespace {

class ONNXOptimizer : public ModulePass
{

public:
  static char ID;
  ONNXOptimizer() : ModulePass(ID) {}

  Pass::ReturnType runOnModule(Module &pModule) override
  {
    std::vector<std::string> passNames{ "eliminate_nop_transpose",
                                        "fuse_consecutive_transposes",
                                        "fuse_transpose_into_gemm",
                                        "fuse_add_bias_into_conv" };
    ::onnx::ModelProto modelProto;
    ::onnc::ExportModelProto(modelProto, pModule);
    ::onnx::optimization::Optimizer onnxOptimizer;
    auto optModelProto = onnxOptimizer.optimize(modelProto, passNames);
    ::onnc::IRBuilder ir_b(pModule);
    ir_b.update(optModelProto);
    return Pass::kModuleChanged;
  }
};

char ONNXOptimizer::ID = 0;

} // anonymous namespace

ModulePass *onnc::createONNXOptimizerPass() { return new ONNXOptimizer(); }
