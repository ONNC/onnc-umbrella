#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. "${SCRIPT_DIR}/common.sh"

if [ "$#" -lt 1 ]; then
  echo "$0 IMAGE_SUFFIX [IMAGE_SUFFIX...]"
  echo ""
  echo "All IMAGE_SUFFIX: ${ALL_IMAGES_LIST[@]}"
  exit 1
else
  for_each_to get_result "$@"
fi
