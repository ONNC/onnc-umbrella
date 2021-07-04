#!/usr/bin/env bash
working_dir=$1 # receiving the build direcotory from CMakeLists.txt

# copy python test script to the build directory
cp test_onni_dump.py ${working_dir}

ONNX_MODEL_LIST=`ls -C -w 0 /models/`

for model in ${ONNX_MODEL_LIST}; do
    echo "create ${model} directory"
    mkdir -p test/regression/test_"${model}"
    echo "#!/bin/bash" > test/regression/test_"${model}"/main.sh
	echo "python ${working_dir}/test_onni_dump.py ${model}" >> test/regression/test_"${model}"/main.sh
done