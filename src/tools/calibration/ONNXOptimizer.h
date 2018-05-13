//===- ONNX_OPTIMIZER.h ---------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNX_OPTIMIZER_H
#define ONNX_OPTIMIZER_H

#include <onnc/Core/ModulePass.h>

namespace onnc {
ModulePass *createONNXOptimizerPass();
}

#endif // ONNX_OPTIMIZER_H
