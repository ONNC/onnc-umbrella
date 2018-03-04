#!/usr/bin/env bash
#
# ONNC Umberalla: build library functions
#
MAKE=${MAKE:-make}
NINJA=${NINJA:-ninja}

##===----------------------------------------------------------------------===##
# Helper functions
##===----------------------------------------------------------------------===##
function getabs
{
  local DIR=${1:-.}
  if [ -d "${DIR}" ]; then
    echo "$(cd "${DIR}" && pwd)"
  else
    echo "$(getabs "$(dirname "${DIR}")")/$(basename "${DIR}")"
  fi
}

function gettop
{
  local TOPFILE=onnc.spec
  if [ -f "${TOPFILE}" ]; then
    echo "$(pwd)"
  else
    local HERE=${PWD}
    T=
    while [ \( ! \( -f "${TOPFILE}" \) \) -a \( "${PWD}" != "/" \) ]; do
      cd .. > /dev/null
      T=$(PWD= pwd)
    done
    cd "${HERE}" > /dev/null
    if [ -f "${T}/${TOPFILE}" ]; then
      echo "${T}"
    fi
  fi
}

function getrel
{
  local RELPATH=${1##"$(gettop)"}
  if [ "$1" = "${RELPATH}" ]; then
    echo "$1"
  else
    echo ".${RELPATH}"
  fi
}

function platform
{
  case "$(uname -s)" in
    Linux)    echo 'linux' ;;
    Darwin)   echo 'macosx' ;;
    FreeBSD)  echo 'freebsd' ;;
    *)        echo 'unknown' ;;
  esac
}

##===----------------------------------------------------------------------===##
# Printing functions
##===----------------------------------------------------------------------===##
function color_print
{
  local NC='\033[0m' # no color
  local RED='\033[0;31m'
  local GREEN='\033[0;32m'
  local YELLOW='\033[0;33m'

  local FOLDER=$(getrel "$(pwd)")
  local COLOR=$1
  local MSG=$2
  local LONG_MSG=$3

  case "${COLOR}" in
    G*) # green
      COLOR=${GREEN}
      ;;
    R*) # red
      COLOR=${RED}
      ;;
    Y*) # yellow
      COLOR=${YELLOW}
      ;;
    *)  # unknown
      ;;
  esac

  echo -e "${COLOR}[${FOLDER}]${NC} ${YELLOW}${MSG}${NC}"
  if [ ! -z "${LONG_MSG}" ]; then
    echo -e "${LONG_MSG}"
  fi
}

function show
{
  color_print 'G' "$@"
}

function warn
{
  color_print 'R' "$@"
}

function fatal
{
  warn "$@"
  exit 1
}

function fail_panic
{
  local MESSAGE="${1:+${1}}"
  shift
  "$@"
  if [ $? != 0 ]; then
    local ERROR="Failed to execute command: '$@'"
    fatal "${MESSAGE}" "${ERROR}"
  fi
}

##===----------------------------------------------------------------------===##
# Build methods
##===----------------------------------------------------------------------===##
function build_autotools_project
{
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2

  shift; shift

  show "building ${NAME} ..."

  if [ ! -d "${BUILDDIR}" ]; then
    show "create build directory at '${BUILDDIR}'"
    mkdir -p "${BUILDDIR}"
  fi

  pushd "${SRCDIR}" > /dev/null
  show "configuring ..."
  AUTOGEN=./autogen.sh
  if [ ! -x "${AUTOGEN}" ]; then
    AUTOGEN=autoreconf
  fi
  fail_panic "Autogen autotools project - ${NAME} failed." ${AUTOGEN}
  popd > /dev/null

  pushd "${BUILDDIR}" > /dev/null
  fail_panic "Configure autotools project - ${NAME} failed." ${SRCDIR}/configure --prefix="${INSTALLDIR}" "$@"

  show "making ..."
  fail_panic "Make autotools project - ${NAME} failed." ${MAKE} -j1 all install

  show "finishing ..."
  popd > /dev/null
}

function build_cmake_project
{
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2

  shift; shift

  show "building ${NAME} ..."

  if [ ! -d "${BUILDDIR}" ]; then
    show "create build directory at '${BUILDDIR}'"
    mkdir -p "${BUILDDIR}"
  fi

  pushd "${BUILDDIR}" > /dev/null
  show "creating makefiles ..."
  fail_panic "Cmake project - ${NAME} failed." cmake -DCMAKE_INSTALL_PREFIX="${INSTALLDIR}" "${SRCDIR}" "$@"

  local PARALLEL_BUILD_FLAG=${MAX_MAKE_JOBS:+"-j${MAX_MAKE_JOBS}"}
  show "making ... #jobs=${MAX_MAKE_JOBS}"
  fail_panic "Make cmake project - ${NAME} failed." ${MAKE} ${PARALLEL_BUILD_FLAG} all install

  show "finishing ..."
  popd > /dev/null
}

function build_handcraft_project
{
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2
  local CONFIGURE=$3

  shift; shift; shift

  show "building ${NAME} ..."

  if [ ! -d "${BUILDDIR}" ]; then
    show "create build directory at '${BUILDDIR}'"
    cp -R "${SRCDIR}" "${BUILDDIR}"
  fi

  pushd "${BUILDDIR}" > /dev/null
  show "configuring ..."
  fail_panic "Configure handcraft project - ${NAME} failed." ./${CONFIGURE} --prefix="${INSTALLDIR}" "$@"
  show "making ..."
  fail_panic "Make handcraft project - ${NAME} failed." ${MAKE} -j1 all install
  show "finishing ..."
  popd > /dev/null
}

##===----------------------------------------------------------------------===##
# Build functions
##===----------------------------------------------------------------------===##
function build_skypat
{
  build_autotools_project "$1" "$2"
}

function build_llvm
{
  local LLVM_VERSION="5.0.1"

  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2

  shift; shift

  show "building ${NAME} ..."

  if [ ! -d "${BUILDDIR}" ]; then
    show "create build directory at '${BUILDDIR}'"
    mkdir -p "${BUILDDIR}"
  fi

  pushd "${BUILDDIR}" > /dev/null

  if [ ! -d "${SRCDIR}" ]; then
    show "${SRCDIR} not found, downloading llvm-${LLVM_VERSION} to ${SRCDIR} ..."
    fail_panic "download llvm failed." \
      curl -SLOJ "https://releases.llvm.org/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz" \
      && tar Jxf "llvm-${LLVM_VERSION}.src.tar.xz" \
      && mv "llvm-${LLVM_VERSION}.src" ${SRCDIR} \
      && rm "llvm-${LLVM_VERSION}.src.tar.xz"
  fi

  show "creating makefiles ..."

  fail_panic "Cmake project - ${NAME} failed." \
    cmake \
    "-DCMAKE_BUILD_TYPE=Release" \
    "-DCMAKE_INSTALL_PREFIX=${INSTALLDIR}" \
    "-DLLVM_TARGETS_TO_BUILD=host;X86;ARM;AArch64" \
    "${SRCDIR}"

  local MAX_MAKE_JOBS=${MAX_MAKE_JOBS-2}
  local PARALLEL_BUILD_FLAG=${MAX_MAKE_JOBS:+"-j${MAX_MAKE_JOBS}"}
  show "making ... #jobs=${MAX_MAKE_JOBS}"
  fail_panic "Make ${NAME} failed." ${MAKE} ${PARALLEL_BUILD_FLAG} all install

  show "finishing ..."
  popd > /dev/null
}

function build_onnx
{
  local SRCDIR="$1"
  local INSTALLDIR=$2
  local NAME=$(basename "${SRCDIR}")

  show "pip installing ..."
  fail_panic "pip install ${NAME} failed." pip install "${SRCDIR}"

  if [ ! -d "${INSTALLDIR}" ]; then
    show "create install directory at '${INSTALLDIR}'"
    mkdir -p "${INSTALLDIR}"
  fi
  local PYTHON_PATH=$(python -c "import onnx, os; print(os.path.dirname(onnx.__path__[0]))")
  cp ${PYTHON_PATH}/onnx/onnx_cpp2py_export.so ${INSTALLDIR}/lib/libonnx.so
  show "finishing ..."
}

function build_bmtap
{
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local EXTDIR=$(dirname "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2

  shift; shift

  show "building ${NAME} src=${SRCDIR} build=${BUILDDIR} install=${INSTALLDIR} ..."
  pushd ${EXTDIR}
  source ${EXTDIR}/build/envsetup.sh
  source ${EXTDIR}/build/envsetup_bmtap.sh
  build_bmtap
  popd
}

function build_bmnet
{
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local EXTDIR=$(dirname "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2

  shift; shift

  show "building ${NAME} ..."
  pushd ${EXTDIR}
  source ${EXTDIR}/build/envsetup.sh
  source ${EXTDIR}/build/envsetup_bmnet.sh
  build_bmnet_all
  popd
}

