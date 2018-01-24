#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. "${SCRIPT_DIR}/common.sh"

if [ "$#" -lt 1 ]; then
  echo "$0 IMAGE_SUFFIX [IMAGE_SUFFIX...]"
  echo "$0 all"
  echo ""
  echo "All IMAGE_SUFFIX: ${ALL_IMAGES_LIST[@]}"
  exit 1
fi

cd $SCRIPT_DIR

if [ "$#" -eq 1 ] && [ "$1" = "all" ]; then
  # Reset parameters
  set -- "${ALL_IMAGES_LIST[@]}"
fi


function build_one
{
  build_image "$1" && get_result "$1"
}

date | tee build.log
for_each_to build_one "$@" 2> >(tee -a build.log)
exit_code="$?"
date | tee -a build.log
exit "$exit_code"
