#!/bin/bash
set -ex

ONNC_REVISION=5264da95e1678d2c8427cfe86140f487566795cc

ONNX_MODELS=(
  bvlc_alexnet
  bvlc_googlenet
  bvlc_reference_caffenet
  bvlc_reference_rcnn_ilsvrc13
  inception_v1
  resnet50
)

DOCKER_DIR=`pwd`/docker

cd /tmp
if [ ! -d /tmp/onnc ]; then
  git clone --recurse-submodules git@git.skymizer.com:onnc/onnc.git
fi
cd onnc && git clean -fxd && git checkout $ONNC_REVISION

mkdir -p $DOCKER_DIR/loadables

CONTAINER_ID=`docker run -d -t -v /tmp/onnc:/onnc/onnc -v $DOCKER_DIR/loadables:/loadables onnc/onnc-community`
docker exec -ti $CONTAINER_ID smake -j8

for model in "${ONNX_MODELS[@]}"
do
  docker exec -ti $CONTAINER_ID ./tools/onnc/onnc -mquadruple nvdla /models/$model/model.onnx

  mkdir -p $DOCKER_DIR/loadables/$model
  docker exec -u 0 $CONTAINER_ID cp out.nvdla /loadables/$model/$model.nvdla
done

docker stop $CONTAINER_ID

echo "DONE"
