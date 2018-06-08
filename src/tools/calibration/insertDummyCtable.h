//===- INSERT_DUMMY_CTABLE.h ----------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef INSERT_DUMMY_CTABLE_H
#define INSERT_DUMMY_CTABLE_H

#include "bm188x_common_calibration.pb.h"
#include <onnc/Core/ModulePass.h>
#include <onnx/common/ir.h>

namespace onnc {
ModulePass *createInsertDummyCtablePass();
}

#endif // INSERT_DUMMY_CTABLE_H
