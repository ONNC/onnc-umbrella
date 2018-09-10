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
  local SRCDIR=$1
  local NAME=$(basename "${SRCDIR}")
  local BUILDDIR=$(getabs "build-${NAME}")
  local INSTALLDIR=$2
  local NAMESPACE=$3

  show "building ${NAME} ..."

  if [ ! -d "${BUILDDIR}" ]; then
    show "create build directory at '${BUILDDIR}'"
    mkdir -p "${BUILDDIR}"
  fi

  pushd "${BUILDDIR}" > /dev/null

  show "creating makefiles ..."

  fail_panic "Cmake project - ${NAME} failed." \
    cmake \
    "-DCMAKE_BUILD_TYPE=Release" \
    "-DCMAKE_INSTALL_PREFIX=${INSTALLDIR}" \
    "-DONNX_NAMESPACE=${NAMESPACE}" \
    "${SRCDIR}"

  local MAX_MAKE_JOBS=${MAX_MAKE_JOBS-2}
  local PARALLEL_BUILD_FLAG=${MAX_MAKE_JOBS:+"-j${MAX_MAKE_JOBS}"}
  show "making ... #jobs=${MAX_MAKE_JOBS}"
  fail_panic "Make ${NAME} failed." ${MAKE} ${PARALLEL_BUILD_FLAG} all

  show "installing ..."
  rm -rf ${INSTALLDIR}/lib/libonnx.a ${INSTALLDIR}/lib/libonnx_proto.a ${INSTALLDIR}/include/onnx
  fail_panic "Install ${NAME} failed." ${MAKE} install 
  show "finishing ..."
  popd > /dev/null
}

##===----------------------------------------------------------------------===##
# Print usage
##===----------------------------------------------------------------------===##

function usage
{
  show "Usage of $(basename $0)"
  echo
  echo "  $0 [mode] [install folder]"
  echo
  echo "mode"
  echo "  * normal: default"
  echo "  * dbg   : debugging mode"
  echo "  * rgn   : regression mode"
  echo "  * opt   : optimized mode"
  echo
  echo "install folder"
  echo "  * /opt/onnc: default"
}

function usage_exit
{
  usage
  exit 1
}

##===----------------------------------------------------------------------===##
# Setup environment variables
##===----------------------------------------------------------------------===##
function check_mode
{
  local MODE=$1
  case "${MODE}" in
    normal) ;;
    dbg) ;;
    rgn) ;;
    opt) ;;
    *)
      return 1
      ;;
  esac
  return 0
}

function setup_environment
{
  # building mode
  export ONNC_MODE=${1:-normal}
  check_mode "${ONNC_MODE}"
  if [ $? -ne 0 ]; then
    usage_exit "$@"
  fi

  # root to the source & external source folder
  export ONNC_SRCDIR=$(getabs "src")
  export ONNC_EXTSRCDIR=$(getabs "external")

  # check out the submodules if we forget to use --recursive when cloning.
  if [ ! -d "${ONNC_SRCDIR}" ]; then
    show "clone onnc source tree"
    git clone https://github.com/ONNC/onnc.git src
  fi
  git submodule update --init --recursive

  # root to the installation place for external libraries
  export ONNC_EXTDIR=$(getabs "onncroot")

  # root to the building folder
  export ONNC_BUILDDIR=$(getabs "build-${ONNC_MODE}")
  if [ -d "${ONNC_BUILDDIR}" ]; then
    show "remove build directory"
    rm -rf "${ONNC_BUILDDIR}"
  fi

  # root to the destination folder
  export ONNC_DESTDIR=$(getabs "install-${ONNC_MODE}")
  if [ -d "${ONNC_DESTDIR}" ]; then
    show "remove destination directory"
    rm -rf "${ONNC_DESTDIR}"
  fi

  # root to the target installation place (PREFIX given when configuring)
  # use DESTDIR as PREFIX when $2 is not empty
  export IS_PREFIX_GIVEN="${2:+true}"
  export ONNC_PREFIX=$(getabs "${2:-"${ONNC_DESTDIR}"}")
  local GIT_HEAD=$(cat .git/HEAD)
  export ONNC_BRANCH_NAME="${GIT_HEAD#ref: refs/heads/}"

  # root to the tarball of files inside ${DESTDIR}
  export ONNC_TARBALL=$(getabs "onnc-${ONNC_BRANCH_NAME}.tar.gz")
  if [ -f "${ONNC_TARBALL}" ]; then
    show "remove existing tarball"
    rm -rf "${ONNC_TARBALL}"
  fi

  # define ONNX namespace
  export ONNC_ONNX_NAMESPACE="onnx"

  # detect MAKE for specific platforms
  export MAKE=${MAKE:-make}
  case "$(platform)" in
    freebsd)
      MAKE=gmake
      ;;
  esac
}

##===----------------------------------------------------------------------===##
# Building external
##===----------------------------------------------------------------------===##
function build_external
{
  show "building external libraries..."

  fail_panic "directory not found: ${ONNC_EXTSRCDIR}" test -d "${ONNC_EXTSRCDIR}"

  build_skypat  "${ONNC_EXTSRCDIR}/SkyPat"   "${ONNC_EXTDIR}"
  build_llvm    "${ONNC_EXTSRCDIR}/llvm"     "${ONNC_EXTDIR}"
  build_onnx    "${ONNC_EXTSRCDIR}/onnx"     "${ONNC_EXTDIR}" "${ONNC_ONNX_NAMESPACE}"
}

##===----------------------------------------------------------------------===##
# Packaging functions
##===----------------------------------------------------------------------===##
function package_tarball
{
  local INSTALLDIR=${ONNC_DESTDIR}
  if [ "${IS_PREFIX_GIVEN}" = "true" ]; then
    INSTALLDIR=${INSTALLDIR}${ONNC_PREFIX}
  fi

  pushd "$(dirname "${INSTALLDIR}")" > /dev/null
  show "packaging tarball '${ONNC_TARBALL}'"
  tar zcf "${ONNC_TARBALL}" "$(basename "${INSTALLDIR}")"
  popd > /dev/null
}

##===----------------------------------------------------------------------===##
# Post-build functions
##===----------------------------------------------------------------------===##
function post_build
{
  case "${ONNC_MODE}" in
    normal) ;;
    dbg) ;;
    rgn) ;; # TODO: execute ./script/regression.sh
    opt) ;;
    *)   ;;
  esac

  local SUCCESS=success
  if [ ! -f "${ONNC_TARBALL}" ]; then
    SUCCESS=failed
  elif [ "$(tar tvf "${ONNC_TARBALL}" | head -n 1 | wc -l)" -eq 0 ]; then
    SUCCESS=failed
  fi
  show "build ${ONNC_TARBALL} for installation on ${ONNC_PREFIX}: ${SUCCESS}"
}