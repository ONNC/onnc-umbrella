#include "ONNXShapeInference.h"
#include <onnc/Core/ModulePass.h>
#include <onnc/IR/IRBuilder.h>
#include <onnc/IR/ONNXUtils.h>
#include <onnx/checker.h>
#include <onnx/common/ir.h>
#include <onnx/shape_inference/implementation.h>

using namespace onnc;

namespace {

class ONNXShapeInference : public ModulePass
{

public:
  static char ID;
  ONNXShapeInference() : ModulePass(ID) {}

  Pass::ReturnType runOnModule(Module &pModule) override
  {
    ::onnx::ModelProto modelProto;
    ::onnc::ExportModelProto(modelProto, pModule);
    try {
      ::onnx::checker::check_model(modelProto);
    } catch (::onnx::checker::ValidationError &e) {
      std::cerr << e.what() << std::endl
                << "ONNXShapeInference pass is not workable!!" << std::endl;
      return kModuleNoChanged;
    }
    ::onnx::shape_inference::InferShapes(modelProto);
    ::onnc::IRBuilder ir_b(pModule);
    ir_b.update(modelProto);
    return kModuleChanged;
  }
};

char ONNXShapeInference::ID = 0;

} // anonymous namespace

ModulePass *onnc::createONNXShapeInferencePass()
{
  return new ONNXShapeInference();
}
