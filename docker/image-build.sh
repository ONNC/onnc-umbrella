#!/bin/bash -e

ver=1.6.1
docker build -t iclink/onnc_dev:$ver -f Dockerfile.dev .
docker push iclink/onnc_dev:$ver
docker tag iclink/onnc_dev:$ver iclink/onnc_dev:latest
docker push iclink/onnc_dev:latest
