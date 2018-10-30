#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
PUSH=
NO_REBUILD=
TAG="latest"

function help_info() {
  echo "Usage: release.sh [options]... [TAG]"
  echo "    --push              Push to docker hub after tag images."
  echo "    --no-rebuild        Do not rebuild images before tag."
  echo "    TAG                 Release tag name (default: \"latest\")"
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
    --push)
      PUSH=1
      ;;
    --no-rebuild)
      NO_REBUILD=1
      ;;
    *)
      "$key"
      ;;
  esac
done

if [ -z "$NO_REBUILD" ]; then
  echo "== Rebuild images for release =="
  "$SCRIPT_DIR/build.sh"
fi

echo "== Tag images for release =="
docker tag onnc/onnc-dev-external-prebuilt "onnc/onnc-dev-external-prebuilt:$TAG"
docker tag onnc/onnc-dev-normal "onnc/onnc-normal:$TAG"
docker tag onnc/onnc-dev-debug "onnc/onnc-debug:$TAG"
docker tag onnc/onnc-community "onnc/onnc-community:$TAG"

if [ -n "$PUSH" ]; then
  echo "== Tag images for release =="
  docker push "onnc/onnc-dev-external-prebuilt:$TAG"
  docker push "onnc/onnc-normal:$TAG"
  docker push "onnc/onnc-debug:$TAG"
  docker push "onnc/onnc-community:$TAG"
fi
