//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Calibration.h"
#include "ONNXOptimizer.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <onnc/Analysis/UpdateGraphOutputSize.h>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>
#include <onnc/IR/ONNCModulePrinter.h>
#include <onnc/IR/ONNXUtils.h>
#include <onnc/IRReader/ONNXReader.h>
#include <onnc/Transforms/removeUnusedNodes.h>
#include <onnx/common/ir_pb_converter.h>

using namespace onnc;

namespace po = boost::program_options;

int main(int pArgc, char *pArgv[])
{
  onnc::Path onnxPath;
  std::string datasetPath;
  {
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("onnx,x", po::value(&onnxPath)->required(), "*.onnx file")
        ("dataset,s", po::value(&datasetPath)->required(), " calibration dataset path(.lmdb)");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(pArgc, pArgv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 0;
    }
    po::notify(vm);
  }

  onnc::onnx::Reader reader;
  onnc::SystemError err;
  std::unique_ptr<onnc::Module> module(reader.parse(onnxPath, err));

  if (!err.isGood()) {
    return EXIT_FAILURE;
  }

  // Run onnx optimizer
  {
    onnc::PassManager pm;
    pm.add(onnc::createONNCModulePrinterPass());
    pm.add(onnc::createRemoveUnusedNodesPass());
    pm.add(onnc::CreateUpdateGraphOutputSizePass());
    pm.add(onnc::createONNXOptimizerPass());
    pm.run(*module);
  }
  std::cout << "after onnx optimizer pass" << std::endl;

  // Run calibration pass.
  {
    onnc::PassManager pm;
    pm.add(onnc::createONNCModulePrinterPass());
    pm.add(onnc::createCalibrationPass(datasetPath));
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
