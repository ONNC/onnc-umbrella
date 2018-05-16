//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Calibration.h"
#include "ONNXOptimizer.h"
#include "insertDummyCtable.h"
#include "quantizeWeight.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>
#include <onnc/IR/ONNCModulePrinter.h>
#include <onnc/IR/ONNXUtils.h>
#include <onnc/IRReader/ONNXReader.h>
#include <onnx/common/ir_pb_converter.h>

using namespace onnc;

int main(int pArgc, char *pArgv[])
{
  if (pArgc != 3) {
    std::cerr << "usage:  " << pArgv[0] << " onnx_file" << std::endl;
    std::cerr << "usage:  " << pArgv[1] << " calibration dataset path(.lmdb)"
              << std::endl;
    return EXIT_FAILURE;
  }

  onnc::onnx::Reader reader;
  onnc::SystemError err;
  std::unique_ptr<onnc::Module> module(reader.parse(onnc::Path(pArgv[1]), err));

  if (!err.isGood()) {
    return EXIT_FAILURE;
  }

  // Run onnx optimizer
  {
    onnc::PassManager pm;
    pm.add(onnc::createONNCModulePrinterPass());
    pm.add(onnc::createONNXOptimizerPass());
    pm.run(*module);
  }
  std::cout << "after onnx optimizer pass" << std::endl;

  // Run calibration pass.
  {
    onnc::PassManager pm;
    pm.add(onnc::createONNCModulePrinterPass());
    pm.add(onnc::createCalibrationPass(pArgv[2]));
    pm.run(*module);
  }

  std::cout << "after createCalibrationPass" << std::endl;
  {
    onnc::PassManager pm;
    pm.add(onnc::createONNCModulePrinterPass());
    pm.run(*module);
  }

  // write the new onnx model back to disk
  const char *fileName = "new.onnx";
  {
    ::onnx::ModelProto modelProto;
    ::onnc::ExportModelProto(modelProto, *module);
    // wrtie file
    std::fstream output(fileName,
                        std::ios::out | std::ios::trunc | std::ios::binary);
    modelProto.SerializeToOstream(&output);
  }

  return 0;
}
