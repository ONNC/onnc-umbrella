#!/usr/bin/env bash
#                       The ONNC Project

set -ex

docker build -t onnc-dev-external-prebuilt -f docker/Dockerfile.external-prebuilt .

docker build -t onnc-dev-debug --build-arg MODE=dbg -f docker/Dockerfile .
docker build -t onnc-dev-normal --build-arg MODE=normal -f docker/Dockerfile .
