//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "insertDummyCtable.h"
#include "quantizeWeight.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>
#include <onnc/IR/ONNCModulePrinter.h>
#include <onnc/IRReader/ONNXReader.h>
#include <onnx/common/ir_pb_converter.h>

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
  {
    if (!err.isGood()) {
      return EXIT_FAILURE;
    }
    onnc::PassManager pm;
    pm.add(::onnc::createONNCModulePrinterPass());
    pm.add(::onnc::createInsertDummyCtablePass());
    pm.run(*module);
  }

  // FIXME add ONNXIRWritter
  // write the new onnx model back to disk
  const char *fileName = "new.onnx";
  {
    ::onnx::ModelProto modelProto;
    // init IR version
    modelProto.set_ir_version(3);
    // init graph
    ::onnx::ExportModelProto(&modelProto, module->getGraphIR());
    // init metadata
    for (auto const &data : module->getMetaData()) {
      auto *metadata_props = modelProto.add_metadata_props();
      metadata_props->set_key(data.first);
      metadata_props->set_value(data.second);
    }
    // wrtie file
    std::fstream output(fileName,
                        std::ios::out | std::ios::trunc | std::ios::binary);
    modelProto.SerializeToOstream(&output);
  }

  // read and print new onnx model
  {
    std::cout << "after InsertDummyCtable" << std::endl;
    std::unique_ptr<onnc::Module> module2(
        reader.parse(onnc::Path(fileName), err));
    if (!err.isGood()) {
      return EXIT_FAILURE;
    }
    ::onnc::PassManager pm;
    pm.add(::onnc::createONNCModulePrinterPass());
    pm.run(*module);
  }

  // test quantizeWeight
  {
    onnc::PassManager pm;
    pm.add(onnc::createQuantizeWeightPass());
    pm.add(onnc::createONNCModulePrinterPass());
    pm.run(*module);
  }
}
