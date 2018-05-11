#include "quantizeWeight.h"
#include <onnc/Core/ModulePass.h>
#include <onnx/common/ir.h>
#include <onnx/common/tensor.h>

using namespace onnc;

namespace {

const size_t getDimCount(const std::vector< ::onnx::Dimension> &pDim)
{
  size_t s = 1;
  for (auto &dim : pDim)
    s *= dim.dim;
  return s;
}

const size_t getTotalCount(const std::vector<int64_t> &pSize)
{
  size_t s = 1;
  for (auto &size : pSize)
    s *= size;
  return s;
}

void copyTensor(::onnx::Tensor &pNewTensor, const ::onnx::Tensor &pOldTensor,
                const std::string &pValueName,
                const ::onnx::TensorProto_DataType pDataType)
{
  // copy sizes
  auto &newTensorSize = pNewTensor.sizes();
  newTensorSize = pOldTensor.sizes();
  // copy dataType
  auto &newTensorType = pNewTensor.elem_type();
  newTensorType = pDataType;
  // copy segment info
  if (pOldTensor.is_segment()) {
    pNewTensor.set_segment_begin_and_end(pOldTensor.segment_begin(),
                                         pOldTensor.segment_end());
  }
  // copy name
  if (pOldTensor.hasName()) {
    pNewTensor.setName(pOldTensor.name());
  }
}

template <class T>
void copyTensorData(::onnx::Tensor &pNewTensor,
                    const std::vector<T> &pDataVector)
{
  // onnx does not support int8 tensor
  auto &newTensorData = pNewTensor.int32s();
  newTensorData.reserve(pDataVector.size());
  for (auto &data : pDataVector) {
    newTensorData.push_back(data);
  }
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
  void genRandomQuantizedWeight(const ::onnx::Graph &pGraph,
                                QuantizedInt8Map &pQuantizedInt8,
                                QuantizedInt16Map &pQuantizedInt16);
};

void quantizeWeight::genRandomQuantizedWeight(
    const ::onnx::Graph &pGraph, QuantizedInt8Map &pQuantizedInt8,
    QuantizedInt16Map &pQuantizedInt16)
{
  // weight name
  const std::unordered_set<std::string> tensorNames(
      const_cast< ::onnx::Graph &>(pGraph).initializer_names().begin(),
      const_cast< ::onnx::Graph &>(pGraph).initializer_names().end());

  for (auto it = pGraph.begin(), ie = pGraph.end(); it != ie; ++it) {
    auto *node = *it;
    auto symbol = node->kind();
    if (symbol == ::onnx::Symbol("Conv")) {
      auto inputs = node->inputs();
      // weight
      const ::onnx::Value *weight = inputs[1];
      size_t weightCount = getDimCount(weight->sizes());
      pQuantizedInt8.emplace(weight->uniqueName(),
                             std::vector<int8_t>(weightCount, 'W'));
      assert(0 != tensorNames.count(weight->uniqueName()));
      // bias
      if (3 == inputs.size()) {
        const ::onnx::Value *bias = inputs[2];
        size_t biasCount = getDimCount(bias->sizes());
        pQuantizedInt16.emplace(bias->uniqueName(),
                                std::vector<int16_t>(biasCount, 'B'));
        assert(0 != tensorNames.count(bias->uniqueName()));
      }

    } else if (symbol == ::onnx::Symbol("MaxPool")) {
      continue;
    } else if (symbol == ::onnx::Symbol("Gemm")) {
      auto inputs = node->inputs();
      // weight
      const ::onnx::Value *weight = inputs[1];
      size_t weightCount = getDimCount(weight->sizes());
      pQuantizedInt8.emplace(weight->uniqueName(),
                             std::vector<int8_t>(weightCount, 'F'));
      assert(0 != tensorNames.count(weight->uniqueName()));
      // bias
      if (3 == inputs.size()) {
        const ::onnx::Value *bias = inputs[2];
        size_t biasCount = getDimCount(bias->sizes());
        pQuantizedInt16.emplace(bias->uniqueName(),
                                std::vector<int16_t>(biasCount, 'B'));
        assert(0 != tensorNames.count(bias->uniqueName()));
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
  genRandomQuantizedWeight(*graph, quantizedInt8, quantizedInt16);

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
      copyTensorData(newTensor, quantizedInt8[valueName]);
      valueTensorMap.emplace(valueName, newTensor);
      continue;
    } else if (1 == quantizedInt16.count(valueName)) {
      ::onnx::Tensor newTensor;
      copyTensor(newTensor, oldTensor, valueName,
                 ::onnx::TensorProto_DataType_INT16);
      assert(quantizedInt16[valueName].size() ==
             getTotalCount(oldTensor.sizes()));
      copyTensorData(newTensor, quantizedInt16[valueName]);
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
