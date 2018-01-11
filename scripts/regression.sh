#!/usr/bin/env bash
#
#                       The ONNC Project
#
source ./scripts/library.sh

##===----------------------------------------------------------------------===##
# Variable Dictionary
##===----------------------------------------------------------------------===##
ROOT="$(gettop)"
RGNDIR="${ROOT}/regression"
RGNREPO="ssh://git@git-bitmain.skymizer.com:6824/bmsky/onnc-regression.git"

##===----------------------------------------------------------------------===##
# Build functions
##===----------------------------------------------------------------------===##
function checkout_regression
{
  if [ -d ${RGNDIR} ]; then
    rm -rf ${RGNDIR}
  fi
  git clone --recursive ${RGNREPO} ${RGNDIR}
}

##===----------------------------------------------------------------------===##
#  Main operations
##===----------------------------------------------------------------------===##
if [ -z "$(gettop)" ]; then
  if [ ! -f ./scripts/library.sh ]; then
    echo "error: ./scripts/library.sh: file doesn't exist"
  fi

  echo "error: ./scripts/library.sh: can not source file"
  exit 1;
fi

checkout_regression
