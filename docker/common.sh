#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


# Pre-defined constant variables.

# Keeps building order
ALL_IMAGES_LIST=(base prebuilt-external prebuilt)


# for_each_to call_back_function args...
function for_each_to
{
  local cb="$1"
  shift
  local exit_code=0
  local temp_exit_code
  for arg in "$@"
  do
    "$cb" "$arg"
    temp_exit_code=$?
    >&2 echo "exit code: $temp_exit_code"
    exit_code=$(($exit_code || $temp_exit_code))
  done
  return "$exit_code"
}


# build_image IMAGE_SUFFIX
function build_image
{
  local exit_code
  pushd $SCRIPT_DIR >/dev/null
  if [ -f "Dockerfile.${1}" ]; then
    local cmd=(docker build -f "Dockerfile.${1}" -t "onnc-${1}" ../)
    >&2 echo "${cmd[@]}"
    time ${cmd[@]} 2>&1
    exit_code=$?
  else
    >&2 echo "Dockerfile.${1} does not exist."
    exit_code=1
  fi
  popd >/dev/null
  return $exit_code
}

# get_result IMAGE_SUFFIX
function get_result
{
  case "$1" in
    prebuilt-external|prebuilt)
      test $(docker run --rm onnc-${1} cat ci-build-result.txt) = "0"
      return $?
      ;;
    *)
      if [ ! -f "${SCRIPT_DIR}/Dockerfile.${1}" ]; then
        >&2 echo "Dockerfile.${arg} does not exist."
        return 1
      else
        return 0
      fi
  esac
}
