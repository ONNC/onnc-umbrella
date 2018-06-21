//===- Calibration.cpp ----------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <caffe/proto/caffe.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <onnc/ADT/Color.h>
#include <onnc/Core/ModulePass.h>
#include <onnc/IR/ONNXUtils.h>
#include <onnc/Support/IOStream.h>

#include "Calibration.h"
#include "Kld.h"

#include <cmath>
#include <fstream>
#include <streambuf>
#include <string>
#include <type_traits>

#include <caffe2/core/db.h>
#include <caffe2/onnx/backend.h>

using namespace onnc;
using namespace caffe2;

namespace {

const std::vector<int64_t>
getDimension(const std::vector< ::onnx::Dimension> pDims)
{
  std::vector<int64_t> int64Dim;
  for (auto &dim : pDims) {
    int64Dim.push_back(dim.dim);
  }
  return int64Dim;
}

const std::vector<int64_t> getInputDataDim(const ::onnx::Graph &pConstGraph)
{
  // FIXME
  ::onnx::Graph &graph = const_cast< ::onnx::Graph &>(pConstGraph);
  std::unordered_set<std::string> initializerNames(
      graph.initializer_names().begin(), graph.initializer_names().end());

  std::vector<const ::onnx::Value *> inputDatas;
  for (const ::onnx::Value *v : graph.inputs()) {
    if (0 == initializerNames.count(v->uniqueName())) {
      inputDatas.push_back(v);
    }
  }
  assert(1 == inputDatas.size());
  return getDimension(inputDatas[0]->sizes());
}

} // anonymous namespace

namespace onnc {

static bool isBlobUsed(const string &pBlobName, caffe2::NetDef &pDef,
                       const int pOpIdx)
{
  bool start = false;
  int opIndex = 0;
  for (const OperatorDef &op : pDef.op()) {
    for (const string &in : op.input()) {
      if (start && (in == pBlobName)) {
        return true;
      }
      if ((in == pBlobName) && (opIndex == pOpIdx)) {
        start = true;
      }
    }
    opIndex++;
  }
  return false;
}

static bool isBlobSingleUser(const string &pBlobName, caffe2::NetDef &pDef)
{
  int useCount = 0;
  for (const OperatorDef &op : pDef.op()) {
    for (const string &in : op.input()) {
      if (in == pBlobName) {
        useCount++;
        if (useCount > 1) {
          return false;
        }
      }
    }
  }
  return true;
}

bool Calibration::readDataset(const std::vector<int64_t> &pInputDims,
                              const string &pDataLayer, int pIteration,
                              const Module::MetaDataMap &pMetaData)
{
  auto nums = ::onnc::getTotalCount(pInputDims);
  auto reader = std::make_unique<caffe2::db::DBReader>("lmdb", m_DBName);

  bool do_scale = false;
  bool do_sub_mean = false;
  bool do_caffe_mean_file = false;
  if (pMetaData.find("data_scale") != pMetaData.end()) {
    do_scale = true;
  }
  if (pMetaData.find("data_sub_mean") != pMetaData.end()) {
    do_sub_mean = true;
  }
  if (pMetaData.find("data_caffe_mean_file") != pMetaData.end()) {
    do_caffe_mean_file = true;
  }

  auto *curCursor = reader->cursor();
  curCursor->SeekToFirst();
  float scale = 1.0;
  std::vector<float> means;
  if (do_scale) {
    scale = std::stof(pMetaData.at("data_scale"));
  }
  if (do_sub_mean) {
    std::string token;
    std::istringstream tokenStream(pMetaData.at("data_sub_mean"));
    char delimiter = ',';
    while (std::getline(tokenStream, token, delimiter)) {
      means.push_back(std::stof(token));
    }
    assert(means.size() == 3);
  }
  if (do_caffe_mean_file) {
    caffe::BlobProto mean_proto;
    string mean_file = m_DBName + "/" + pMetaData.at("data_caffe_mean_file");
    std::ifstream finput(mean_file, std::ifstream::binary);
    if (finput.fail()) {
      errs() << "error: can not open mean file : " << mean_file << "\n";
      exit(1);
    }
    google::protobuf::io::IstreamInputStream raw_input(&finput);
    google::protobuf::io::CodedInputStream coded_input(&raw_input);

    bool success = mean_proto.ParseFromCodedStream(&coded_input);
    if (success == false) {
      errs() << "error: can not parse mean file : " << mean_file << "\n";
      exit(1);
    }

    for (int i = 0; i < mean_proto.data_size(); ++i) {
      means.push_back(mean_proto.data(i));
    }
  }
  if (do_sub_mean && do_caffe_mean_file) {
    errs() << "error: should not use 'data_sub_mean' and "
              "'data_caffe_mean_file' concurrently\n";
    exit(1);
  }
  for (int run = 0; run < pIteration; run++) {
    std::cout << "pInputDims NCHW: " << pInputDims << std::endl;
    std::vector<float> data;
    for (int b = 0; b < pInputDims.at(0); b++) {
      std::string raw(curCursor->value());
      curCursor->Next();

      caffe::Datum datum;
      datum.ParseFromString(raw);
      std::cout << "datum CHW: " << datum.channels() << " " << datum.height()
                << " " << datum.width() << std::endl;
      int h = datum.height();
      int w = datum.width();
      int h_crop = pInputDims.at(2);
      int w_crop = pInputDims.at(3);
      assert(h == w);
      assert(h_crop == w_crop);
      assert(datum.channels() == pInputDims.at(1));
      assert(means.size() ==
             (size_t)(datum.channels() * datum.height() * datum.width()));
      int h_offset = (h - h_crop) / 2;
      int w_offset = (w - w_crop) / 2;
      for (int c = 0; c < datum.channels(); ++c) {
        for (int ih = h_offset; ih < h_offset + h_crop; ++ih) {
          for (int iw = w_offset; iw < w_offset + w_crop; ++iw) {
            int i = c * h * w + ih * w + iw;
            float pixel = (uint8_t)datum.data()[i];
            if (do_sub_mean)
              pixel -= means[c];
            if (do_caffe_mean_file)
              pixel -= means[i];
            if (do_scale)
              pixel *= scale;
            data.push_back(pixel);
          }
        }
      }
    }
    if (nums != data.size()) {
      std::cout << "error: dimension mismatch, " << nums << " vs "
                << data.size() << std::endl;
      exit(1);
    }
    TensorCPU tensor(pInputDims, data, nullptr);
    m_BlobData[pDataLayer].emplace_back(tensor);
  }

  return true;
}

float Calibration::calculateKLD(const string &pBlobName)
{
  int iteration = m_BlobData[pBlobName].size();
  int dataSize = 0;
  vector<float> dataCollect;
  // Count memory size to reserve.
  for (int i = 0; i < iteration; i++) {
    dataSize += m_BlobData[pBlobName][i].size();
  }
  dataCollect.reserve(dataSize);
  // Concat all batches' tensor.
  for (int i = 0; i < iteration; i++) {
    auto probs = m_BlobData[pBlobName][i].data<float>();
    dataCollect.insert(dataCollect.end(), probs,
                       probs + m_BlobData[pBlobName][i].size());
  }
  return KLDiversity(dataCollect.data(), dataCollect.size());
}

void Calibration::profileModel(int pIteration, caffe2::NetDef &pDef,
                               const string &pDataLayer)
{
  // Calculate "data layer" KLD.
  m_ThresholdY[pDataLayer] = calculateKLD(pDataLayer);

  // Add "data layer" threshold into Ctable.
  tg::bm1880::LayerCalibrationParameter *layerCalibrationParam =
      m_NetCtableParam.add_layer();
  layerCalibrationParam->set_name(pDataLayer);
  tg::bm1880::BlobParameter *outBlobParam =
      layerCalibrationParam->add_blob_param();
  outBlobParam->set_name(pDataLayer);
  outBlobParam->set_threshold_y(m_ThresholdY[pDataLayer]);

  int opIdx = 0;
  for (const OperatorDef &op : pDef.op()) {
    for (int run = 0; run < pIteration; run++) {
      // Prepare input blobs.
      for (const string &in : op.input()) {
        // Only feed data to input. (no needs to feed weights)
        if (m_BlobData.find(in) != m_BlobData.end()) {
          // Feed data from previous layer.
          auto input = m_Workspace->GetBlob(in)->GetMutable<TensorCPU>();
          input->ResizeLike(m_BlobData[in][run]);
          input->ShareData(m_BlobData[in][run]);
        }
      }
      CAFFE_ENFORCE(m_Workspace->RunOperatorOnce(op));
      // Save output blobs.
      for (const string &in : op.output()) {
        auto blob = m_Workspace->GetBlob(in);
        // Special case: Skip Dropout layer.
        // FIXME: Remove. Dropout layer should be deleted in layer-opt. phase.
        if (string(blob->TypeName()) != "nullptr (uninitialized)") {
          CPUContext cpuContext;
          TensorCPU output(m_Workspace->GetBlob(in)->Get<TensorCPU>(),
                           &cpuContext);
          m_BlobData[in].emplace_back(output);
        }
      }
    }
    // Calculate KLD. Save results in threshold_y.
    for (const string &in : op.output()) {
      auto blob = m_Workspace->GetBlob(in);
      // FIXME: Remove. Dropout layer should be deleted in layer-opt. phase.
      if (string(blob->TypeName()) != "nullptr (uninitialized)" &&
          blob->Get<TensorCPU>().IsType<float>()) {
        m_ThresholdY[in] = calculateKLD(in);
      }
    }

    // Free unused input blobs for saving memory.
    for (const string &in : op.input()) {
      if (m_BlobData.find(in) != m_BlobData.end()) {
        if (!isBlobUsed(in, pDef, opIdx)) {
          m_BlobData.erase(in);
        }
      }
    }
    opIdx++;
  }
}

void Calibration::updateThreshold(caffe2::NetDef &pDef)
{
  for (const OperatorDef &op : pDef.op()) {
    tg::bm1880::LayerCalibrationParameter *layerCalibrationParam =
        m_NetCtableParam.add_layer();
    layerCalibrationParam->set_name(op.output(0));
    for (const string &out : op.output()) {
      tg::bm1880::BlobParameter *outBlobParam =
          layerCalibrationParam->add_blob_param();
      outBlobParam->set_name(out);
      outBlobParam->set_threshold_y(m_ThresholdY[out]);
    }
  }
  return;
}

// Optimizations for Quantization.
void Calibration::thresholdFold(caffe2::NetDef &pDef)
{
  // Forward folding.
  for (const OperatorDef &op : pDef.op()) {
    if (op.type() == "Relu" || op.type() == "Flatten") {
      const string &inputName = op.input(0);
      const string &outputName = op.output(0);
      m_ThresholdY[outputName] = m_ThresholdY[inputName];
    }
  }

  // Backward folding.
  int opSize = pDef.op_size();
  for (int i = (opSize - 1); i >= 0; i--) {
    const OperatorDef &op = pDef.op(i);

    if (op.type() == "Relu" || op.type() == "Flatten") {
      const string &inputName = op.input(0);
      const string &outputName = op.output(0);
      m_ThresholdY[inputName] = m_ThresholdY[outputName];
    }

    if (op.type() == "Concat") {
      const string &outputName = op.output(0);
      // Check the bottom layer's output is not only for concat.
      for (const string &in : op.input()) {
        if (isBlobSingleUser(in, pDef)) {
          m_ThresholdY[in] = m_ThresholdY[outputName];
        } else {
          std::cout << "Blob " << in << " of Concat input has multiple user!"
                    << std::endl;
          assert(0);
        }
      }
    }
  }
}

Pass::ReturnType Calibration::runOnModule(::onnc::Module &pModule)
{
  // TODO: Check If Ctable exist, then skip this pass.

  std::string onnxStr;
  ::onnc::SerializeToString(onnxStr, pModule);

  // Create caffe2 backend.
  auto *backend = new caffe2::onnx::Caffe2Backend(nullptr);
  std::vector<caffe2::onnx::Caffe2Ops> extras;
  auto rep = backend->Prepare(onnxStr, "CPU", extras);
  auto def = rep->pred_net();

  // Init net weights.
  m_Workspace = new Workspace("layer_calibration");
  CAFFE_ENFORCE(m_Workspace->RunNetOnce(rep->init_net()));

  // Find data layer's name.
  const OperatorDef &op = def.op(0);
  const string &dataLayer = op.input(0);
  m_Workspace->CreateBlob(dataLayer);
  auto graph = pModule.getGraphIR();
  if (!readDataset(getInputDataDim(*graph.get()), dataLayer, m_Iteration,
                   pModule.getMetaData())) {
    errs() << Color::RED << "Error" << Color::RESET << ": Read data set fail..."
           << std::endl;
    return Pass::kModuleNoChanged;
  }

  // Run inference and calculate KLD.
  profileModel(m_Iteration, def, dataLayer);

  thresholdFold(def);

  updateThreshold(def);

  std::cout << m_NetCtableParam.DebugString() << std::endl;
  // write ctable
  pModule.getMetaData().insert(
      { "bm1880_ctable", m_NetCtableParam.DebugString() });

  delete backend;

  return Pass::kModuleChanged;
}
} // namespace onnc

char Calibration::ID = 0;
ModulePass *onnc::createCalibrationPass(const std::string pDBName, int pI)
{
  return new Calibration(pDBName, pI);
}
