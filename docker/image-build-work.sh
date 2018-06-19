#!/bin/bash -e

docker pull iclink/onnc_dev:latest

docker build -t iclink/onnc_dev:work -f Dockerfile.dev.work .
docker push iclink/onnc_dev:work
