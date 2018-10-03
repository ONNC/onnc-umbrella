#!/usr/bin/env bash
#                       The ONNC Project

set -ex

docker build -t onnc/onnc:external-prebuilt -f docker/Dockerfile.external-prebuilt .

docker build -t onnc/onnc --build-arg MODE=normal -f docker/Dockerfile .
docker build -t onnc/onnc-debug --build-arg MODE=dbg -f docker/Dockerfile .
