#!/usr/bin/env bash

set -e

ONNX_MODEL_LIST="bvlc_alexnet bvlc_googlenet bvlc_reference_caffenet bvlc_reference_rcnn_ilsvrc13 densenet121 inception_v1 inception_v2 resnet50 shufflenet squeezenet vgg19 zfnet512"
ONNX_OPSET=8

#----------------------------------------------------------
#   Download ONNX models from ONNX model zoo
#----------------------------------------------------------
if [ ! -d models ]; then
  mkdir models
fi

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
