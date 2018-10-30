#!/usr/bin/env bash
#                       The ONNC Project

ONNX_MODEL_LIST=`ls -C -w 0 /models/`
RUN_MODEL=
REBUILD=
args=()

function help_info() {
  echo "Usage: run-benchmark.sh [options]... MODEL [ARGUMENTS...]"
  echo "    --rebuild           Rebuild before run. (Put the source to /${HOME}/onnc)"
  echo "    MODEL               Run model MODEL."
  echo "    ARGUMENTS           Arguments be passed to target."
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
    -h|--help)
      help_info
      exit 0
      ;;
    --rebuild)
      REBUILD=1
      ;;
    *)
      for model in ${ONNX_MODEL_LIST}; do
        echo "${model}" | grep -q "$key"
        if [ $? -eq 0 ]; then
          RUN_MODEL="${model}"
        fi
      done
      if [ -z "$RUN_MODEL" ]; then
        echo "Can not find ${key}"
        help_info
        exit 1
      fi
      args=("$@")
      set --
      ;;
  esac
done

if [ -n "$REBUILD" ]; then
  cd "$HOME/onnc-umbrella/build-normal"
  smake install -j4
  cd -
fi

set -x

onni "/models/${RUN_MODEL}/model.onnx" "/models/${RUN_MODEL}/test_data_set_0/input_0.pb" "${args[@]}"
