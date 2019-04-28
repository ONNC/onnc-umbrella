#!/usr/bin/env bash
#                       The ONNC Project

ONNX_MODEL_LIST=`ls -C -w 0 /models/`

for model in ${ONNX_MODEL_LIST}; do
    echo "create ${model} directory"
    mkdir -p test/regression/onni-"${model}"
    echo "#!/bin/bash" > test/regression/onni-"${model}"/main.sh
	echo "onni /models/${model}/model.onnx /models/${model}/test_data_set_0/input_0.pb -o test/regression/onni-"${model}"/output-onni.pb" \
		>> test/regression/onni-"${model}"/main.sh
done