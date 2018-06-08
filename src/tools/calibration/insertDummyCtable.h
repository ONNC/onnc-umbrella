//===- INSERT_DUMMY_CTABLE.h ----------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef INSERT_DUMMY_CTABLE_H
#define INSERT_DUMMY_CTABLE_H

#include <onnc/Core/ModulePass.h>
#include <onnc/Target/TG/BM188x/common_calibration2.pb.h>
#include <onnx/common/ir.h>

namespace onnc {
ModulePass *createInsertDummyCtablePass();
}

#endif // INSERT_DUMMY_CTABLE_H
