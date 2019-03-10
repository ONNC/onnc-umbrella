#!/usr/bin/env bash
#                       The ONNC Project

set -ex

docker build -t onnc/onnc-dev-external-prebuilt -f docker/Dockerfile.external-prebuilt .

docker build -t onnc/onnc-community -f docker/Dockerfile.community .
