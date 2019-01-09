#!/usr/bin/env bash
#                       The ONNC Project

set -ex

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
$SCRIPT_DIR/download-models.sh

docker build -t onnc/onnc-dev-external-prebuilt -f docker/Dockerfile.external-prebuilt .

docker build -t onnc/onnc-dev-normal --build-arg MODE=normal -f docker/Dockerfile .
docker build -t onnc/onnc-dev-debug --build-arg MODE=dbg -f docker/Dockerfile .

docker build -t onnc/onnc-benchmarking-models -f docker/Dockerfile.models ./models/
docker build -t onnc/onnc-community -f docker/Dockerfile.community .
