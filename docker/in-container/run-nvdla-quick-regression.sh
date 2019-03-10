#!/usr/bin/env bash

set -ex

MODEL_ZOO=/models
LOADABLE_DIR=/onnc/loadables
MODELS=($(ls -1 $LOADABLE_DIR))

ONNC=onnc

# check output NVDLA lodable content is consistent
for model in "${MODELS[@]}"
do
  $ONNC -mquadruple nvdla $MODEL_ZOO/$model/model.onnx
  diff out.nvdla $LOADABLE_DIR/$model/$model.nvdla
done

echo "DONE"
