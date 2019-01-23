#!/bin/bash

set -ex
ONNX_MODEL_LIST="bvlc_alexnet bvlc_googlenet bvlc_reference_caffenet bvlc_reference_rcnn_ilsvrc13 densenet121 inception_v1 inception_v2 resnet50 shufflenet squeezenet vgg19 zfnet512"
ONNX_OPSET=8

# TODO: download all models from onnx model zoo
function help_info() {
  echo "Usage: build.sh [-opv|--opset-ver V]"
  echo "  -opv|--opset-ver V  Set ONNX opset version V(default: ${ONNX_OPSET})"
  echo "========================================"
  echo "ONNX model list (from ONNX model zoo):"
  echo "${ONNX_MODEL_LIST}"
}

#----------------------------------------------------------
#   Arguments parsing
#----------------------------------------------------------
while [[ $# -gt 0 ]];
do
  key="$1"
  shift

  case $key in
    -opv|--opset-ver)
      ONNX_OPSET="$2"
      shift
      ;;
    -h|--help)
      help_info
      exit 0
      ;;
    *)
      help_info
      exit 0
      ;;
  esac
done

#----------------------------------------------------------
#   Download ONNX models from ONNX model zoo
#----------------------------------------------------------
if [ -d models ]; then
  mv --backup --no-target-directory models models.old
fi
mkdir -p models

for model in ${ONNX_MODEL_LIST}; do
  if [ ! -d models/${model} ]; then
    MODEL_FILENAME="${model}.tar.gz"
    wget "https://s3.amazonaws.com/download.onnx/models/opset_${ONNX_OPSET}/${MODEL_FILENAME}" -O "$MODEL_FILENAME"
    if [ $? -ne 0 ]; then
      echo "ERROR: can not download ${model} (ONNX opset ${ONNX_OPSET})"
      exit 0
    fi

    tar -xzf "$MODEL_FILENAME" -C models
    rm "$MODEL_FILENAME"
  fi
done


docker build -t onnc-benchmarking-models -f Dockerfile.models ./models/
