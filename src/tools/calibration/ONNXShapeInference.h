//===- ONNXShapeInference.h -----------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNX_SHAPE_INFERENCE_H
#define ONNX_SHAPE_INFERENCE_H

#include <onnc/Core/ModulePass.h>

namespace onnc {
ModulePass *createONNXShapeInferencePass();
}

#endif // ONNX_SHAPE_INFERENCE_H
