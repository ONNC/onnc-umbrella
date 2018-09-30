#!/usr/bin/env bash
#                       The ONNC Project

docker build -t onnc-dev-debug --build-arg MODE=debug -f docker/Dockerfile .
docker build -t onnc-dev-normal --build-arg MODE=normal -f docker/Dockerfile .
