//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "insertDummpyCtable.h"
#include <iostream>
#include <memory>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>
#include <onnc/IR/ONNCModulePrinter.h>
#include <onnc/IRReader/ONNXReader.h>

using namespace onnc;

int main(int pArgc, char *pArgv[])
{
  if (pArgc != 2) {
    std::cerr << "usage:  " << pArgv[0] << " onnx_file" << std::endl;
    return EXIT_FAILURE;
  }

  onnc::onnx::Reader reader;
  onnc::SystemError err;
  std::unique_ptr<onnc::Module> module(reader.parse(onnc::Path(pArgv[1]), err));
  if (!err.isGood()) {
    return EXIT_FAILURE;
  }
  PassManager pm;
  pm.add(createONNCModulePrinterPass());
  pm.add(createInsertDummpyCtablePass());
  pm.run(*module);
}
