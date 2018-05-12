#include "quantizeWeight.h"
#include <algorithm>
#include <cstddef>
#include <onnc/Core/ModulePass.h>
#include <onnx/common/ir.h>
#include <onnx/common/tensor.h>

using namespace onnc;

namespace {

const size_t getTotalCount(const std::vector<int64_t> &pSize)
{
  size_t s = 1;
  for (auto &size : pSize)
    s *= size;
  return s;
}

void copyTensor(::onnx::Tensor &pDstTensor, const ::onnx::Tensor &pSrcTensor,
                const std::string &pValueName,
                const ::onnx::TensorProto_DataType pDataType)
{
  // copy sizes
  auto &newTensorSize = pDstTensor.sizes();
  newTensorSize = pSrcTensor.sizes();
  // copy dataType
  auto &newTensorType = pDstTensor.elem_type();
  newTensorType = pDataType;
  // copy segment info
  if (pSrcTensor.is_segment()) {
    pDstTensor.set_segment_begin_and_end(pSrcTensor.segment_begin(),
                                         pSrcTensor.segment_end());
  }
  // copy name
  if (pSrcTensor.hasName()) {
    pDstTensor.setName(pSrcTensor.name());
  }
}

template <class T>
void copyData2Tensor(::onnx::Tensor &pTensor, const std::vector<T> &pDataVector)
{
  // ::onnx does not support int8 tensor
  auto &tensorData = pTensor.int32s();
  tensorData.reserve(pDataVector.size());
  for (auto &data : pDataVector) {
    tensorData.push_back(data);
  }
}

template <class T>
void copyTensor2Data(std::vector<T> &pDataVector, const ::onnx::Tensor &pTensor)
{
  assert("only support to quantize float type" &&
         ::onnx::TensorProto_DataType_FLOAT == pTensor.elem_type());
  size_t count = getTotalCount(pTensor.sizes());
  pDataVector.reserve(count);
  if (pTensor.raw().empty()) {
    auto floats = pTensor.floats();
    for (auto f : floats) {
      // quantize, just for test
      pDataVector.push_back((char)f);
    }
  } else {
    const std::string rawString = pTensor.raw();
    const char *raw = rawString.c_str();
    for (int i = 0; i < rawString.length(); i += sizeof(float) / sizeof(char)) {
      auto *f = reinterpret_cast<const float *>(&raw[i]);
      // quantize, just for test
      pDataVector.push_back((char)*f);
    }
  }
}

const ::onnx::Tensor &getTensor(std::string pName, const ::onnx::Graph &pGraph)
{
  auto initNames = const_cast< ::onnx::Graph &>(pGraph).initializer_names();
  std::ptrdiff_t idx = std::distance(
      initNames.begin(), std::find(initNames.begin(), initNames.end(), pName));
  return const_cast< ::onnx::Graph &>(pGraph).initializers()[idx];
}

class quantizeWeight : public ModulePass
{

public:
  using QuantizedInt8Pair = std::pair<std::string, std::vector<int8_t> >;
  using QuantizedInt8Map =
      std::unordered_map<std::string, std::vector<int8_t> >;
  using QuantizedInt16Pair = std::pair<std::string, std::vector<int16_t> >;
  using QuantizedInt16Map =
      std::unordered_map<std::string, std::vector<int16_t> >;
  static char ID;
  quantizeWeight() : ModulePass(ID) {}

  Pass::ReturnType runOnModule(Module &pModule) override;

private:
  void genQuantizedWeight(const ::onnx::Graph &pGraph,
                          QuantizedInt8Map &pQuantizedInt8,
                          QuantizedInt16Map &pQuantizedInt16);
};

void quantizeWeight::genQuantizedWeight(const ::onnx::Graph &pGraph,
                                        QuantizedInt8Map &pQuantizedInt8,
                                        QuantizedInt16Map &pQuantizedInt16)
{
  for (auto it = pGraph.begin(), ie = pGraph.end(); it != ie; ++it) {
    auto *node = *it;
    auto symbol = node->kind();
    if (symbol == ::onnx::Symbol("Conv")) {
      auto inputs = node->inputs();
      // inputs[1] is weight
      {
        const std::string tensorName = inputs[1]->uniqueName();
        const ::onnx::Tensor &tensor = getTensor(tensorName, pGraph);
        std::vector<int8_t> int8DataVector;
        copyTensor2Data(int8DataVector, tensor);
        pQuantizedInt8.emplace(tensorName, int8DataVector);
      }

      // inputs[2] is bias
      if (3 == inputs.size()) {
        const std::string tensorName = inputs[2]->uniqueName();
        const ::onnx::Tensor &tensor = getTensor(tensorName, pGraph);
        std::vector<int16_t> int16DataVector;
        copyTensor2Data(int16DataVector, tensor);
        pQuantizedInt16.emplace(tensorName, int16DataVector);
      }

    } else if (symbol == ::onnx::Symbol("MaxPool")) {
      continue;
    } else if (symbol == ::onnx::Symbol("Gemm")) {
      auto inputs = node->inputs();
      // inputs[1] is weight
      {
        const std::string tensorName = inputs[1]->uniqueName();
        const ::onnx::Tensor &tensor = getTensor(tensorName, pGraph);
        std::vector<int8_t> int8DataVector;
        copyTensor2Data(int8DataVector, tensor);
        pQuantizedInt8.emplace(tensorName, int8DataVector);
      }

      // inputs[2] is bias
      if (3 == inputs.size()) {
        const std::string tensorName = inputs[2]->uniqueName();
        const ::onnx::Tensor &tensor = getTensor(tensorName, pGraph);
        std::vector<int16_t> int16DataVector;
        copyTensor2Data(int16DataVector, tensor);
        pQuantizedInt16.emplace(tensorName, int16DataVector);
      }
    } else if (symbol == ::onnx::Symbol("Relu")) {
      continue;
    } else if (symbol == ::onnx::Symbol("Softmax")) {
      continue;
    } else if (symbol == ::onnx::Symbol("Undefined")) {
      continue;
    } else {
      assert(0);
    }
  }
}

Pass::ReturnType quantizeWeight::runOnModule(Module &pModule)
{
  auto graph = pModule.getGraphIR().get();

  // generate radom quantized weight
  QuantizedInt8Map quantizedInt8;
  QuantizedInt16Map quantizedInt16;
  genQuantizedWeight(*graph, quantizedInt8, quantizedInt16);

  // update elemType
  for (auto input : graph->inputs()) {
    auto elemType = input->elemType();
    if (elemType == ::onnx::TensorProto_DataType_FLOAT) {
      auto name = input->uniqueName();
      if (quantizedInt8.count(name)) {
        input->setElemType(::onnx::TensorProto_DataType_INT8);
      } else if (quantizedInt16.count(name)) {
        input->setElemType(::onnx::TensorProto_DataType_INT16);
      } else {
        // input data default is INT8
        input->setElemType(::onnx::TensorProto_DataType_INT8);
      }
    } else {
      std::cout << "unsupported quantize type:"
                << TensorProto_DataType_Name(elemType) << std::endl;
      assert(0);
    }
  }

  // update Tensor
  std::unordered_map<std::string, ::onnx::Tensor> valueTensorMap;
  const std::vector< ::onnx::Tensor> initTensors = graph->initializers();
  const std::vector<std::string> tensorNames = graph->initializer_names();
  for (int i = 0; i < initTensors.size(); ++i) {
    auto valueName = tensorNames[i];
    auto oldTensor = initTensors[i];
    if (1 == quantizedInt8.count(valueName)) {
      ::onnx::Tensor newTensor;
      copyTensor(newTensor, oldTensor, valueName,
                 ::onnx::TensorProto_DataType_INT8);
      assert(quantizedInt8[valueName].size() ==
             getTotalCount(oldTensor.sizes()));
      copyData2Tensor(newTensor, quantizedInt8[valueName]);
      valueTensorMap.emplace(valueName, newTensor);
      continue;
    } else if (1 == quantizedInt16.count(valueName)) {
      ::onnx::Tensor newTensor;
      copyTensor(newTensor, oldTensor, valueName,
                 ::onnx::TensorProto_DataType_INT16);
      assert(quantizedInt16[valueName].size() ==
             getTotalCount(oldTensor.sizes()));
      copyData2Tensor(newTensor, quantizedInt16[valueName]);
      valueTensorMap.emplace(valueName, newTensor);
      continue;
    }
    assert(0);
  }

  graph->clearInitializers();
  for (auto &kv : valueTensorMap) {
    graph->addInitializer(kv.second, kv.first);
  }

  return kModuleNoChanged;
}

char quantizeWeight::ID = 0;

} // anonymous namespace

ModulePass *onnc::createQuantizeWeightPass() { return new quantizeWeight(); }
