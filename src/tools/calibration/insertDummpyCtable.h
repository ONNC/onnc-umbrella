//===- INSERT_DUMMPY_CTABLE.h ---------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef INSERT_DUMMPY_CTABLE_H
#define INSERT_DUMMPY_CTABLE_H

#include "common_calibration.pb.h"
#include <onnc/Core/ModulePass.h>
#include <onnx/common/ir.h>

namespace onnc {
ModulePass *createInsertDummpyCtablePass();
}

namespace {

class InsertDummpyCtable : public onnc::ModulePass
{

  void insertLayerName(::onnx::Node *pNode)
  {
    LayerCalibrationParameter *layerParam = m_NetCtableParam.add_layer();
    // TODO multiple output
    assert(pNode->output()->has_unique_name());
    layerParam->set_name(pNode->output()->uniqueName());
  }

public:
  static char ID;
  InsertDummpyCtable() : ModulePass(ID) {}

  Pass::ReturnType runOnModule(onnc::Module &pModule) override
  {
    auto graph = pModule.getGraphIR();
    if (graph->has_name()) {
      m_NetCtableParam.set_name(graph->name());
    }

    for (auto it = graph->begin(), ie = graph->end(); it != ie; ++it) {
      auto *node = *it;
      auto symbol = node->kind();
      if (symbol == ::onnx::Symbol("Conv")) {
        insertLayerName(node);
      } else if (symbol == ::onnx::Symbol("MaxPool")) {
        insertLayerName(node);
      } else if (symbol == ::onnx::Symbol("Gemm")) {
        insertLayerName(node);
      } else if (symbol == ::onnx::Symbol("Relu")) {
        insertLayerName(node);
      } else if (symbol == ::onnx::Symbol("Softmax")) {
        insertLayerName(node);
      } else if (symbol == ::onnx::Symbol("Undefined")) {
        continue;
      } else {
        assert(0);
      }
    }
    return kModuleNoChanged;
  }

private:
  NetCalibrationParameter m_NetCtableParam;
};

char InsertDummpyCtable::ID = 0;

} // anonymous namespace

onnc::ModulePass *onnc::createInsertDummpyCtablePass()
{
  return new InsertDummpyCtable();
}

#endif // INSERT_DUMMPY_CTABLE_H
