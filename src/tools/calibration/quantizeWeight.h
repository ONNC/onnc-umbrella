//===- QUANTIZE_WEIGHT.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef QUANTIZE_WEIGHT_H
#define QUANTIZE_WEIGHT_H

#include <onnc/Core/ModulePass.h>
#include <onnx/common/ir.h>

namespace onnc {
ModulePass *createQuantizeWeightPass();
}

#endif // QUANTIZE_WEIGHT_H
