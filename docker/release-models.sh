#!/bin/bash

set -ex

if [ $# -lt 2  ]; then
  echo "Usage: release-models.sh OPSET-VERSION SUBVERSION"
  exit 0
fi

OPSET="$1"
SUB="$2"

docker tag \
  onnc-benchmarking-models \
  "onnc/onnc-benchmarking-models"

docker tag \
  onnc-benchmarking-models \
  "onnc/onnc-benchmarking-models:${OPSET}.${SUB}"

docker push \
  "onnc/onnc-benchmarking-models"

docker push \
  "onnc/onnc-benchmarking-models:${OPSET}.${SUB}"
