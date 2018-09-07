#!/usr/bin/env bash
#                       The ONNC Project
#
source ./scripts/library.sh

##===----------------------------------------------------------------------===##
# Print usage of this script
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

  # detect MAKE for specific platforms
  export MAKE=${MAKE:-make}
  case "$(platform)" in
    freebsd)
      MAKE=gmake
      ;;
  esac
}

##===----------------------------------------------------------------------===##
# Building functions
##===----------------------------------------------------------------------===##
function build_external
{
  show "building external libraries..."

  fail_panic "directory not found: ${ONNC_EXTSRCDIR}" test -d "${ONNC_EXTSRCDIR}"

  build_skypat  "${ONNC_EXTSRCDIR}/SkyPat"   "${ONNC_EXTDIR}"
  build_llvm    "${ONNC_EXTSRCDIR}/llvm"     "${ONNC_EXTDIR}"
  build_onnx    "${ONNC_EXTSRCDIR}/onnx"     "${ONNC_EXTDIR}"
}

function build_onnc
{
  show "building onnc..."

  fail_panic "directory not found: ${ONNC_SRCDIR}" test -d "${ONNC_SRCDIR}"

  show "create build directory at '${ONNC_BUILDDIR}'"
  mkdir -p "${ONNC_BUILDDIR}"
  pushd "${ONNC_BUILDDIR}" > /dev/null

  case "${ONNC_MODE}" in
    normal)
      fail_panic "CMake onnc failed." cmake -DCMAKE_BUILD_TYPE=Release \
                          -DCMAKE_INSTALL_PREFIX="${ONNC_PREFIX}" \
                          -DLLVM_ROOT_DIR="${ONNC_EXTDIR}" \
                          -DONNX_ROOT="${ONNC_EXTDIR}" \
                          -DONNX_NAMESPACE=onnx \
                          -DSKYPAT_ROOT="${ONNC_EXTDIR}" \
                          ${ONNC_SRCDIR}
      ;;
    dbg)
      fail_panic "CMake onnc failed." cmake -DCMAKE_BUILD_TYPE=Debug \
                          -DCMAKE_INSTALL_PREFIX="${ONNC_PREFIX}" \
                          -DLLVM_ROOT_DIR="${ONNC_EXTDIR}" \
                          -DONNX_ROOT="${ONNC_EXTDIR}" \
                          -DONNX_NAMESPACE=onnx \
                          -DSKYPAT_ROOT="${ONNC_EXTDIR}" \
                          ${ONNC_SRCDIR}
      ;;
    rgn)
      fail_panic "CMake onnc failed." cmake -DCMAKE_BUILD_TYPE=Regression \
                          -DCMAKE_INSTALL_PREFIX="${ONNC_PREFIX}" \
                          -DLLVM_ROOT_DIR="${ONNC_EXTDIR}" \
                          -DONNX_ROOT="${ONNC_EXTDIR}" \
                          -DONNX_NAMESPACE=onnx \
                          -DSKYPAT_ROOT="${ONNC_EXTDIR}" \
                          ${ONNC_SRCDIR}
      ;;
    opt)
      fail_panic "CMake onnc failed." cmake -DCMAKE_BUILD_TYPE=Optimized \
                          -DCMAKE_INSTALL_PREFIX="${ONNC_PREFIX}" \
                          -DLLVM_ROOT_DIR="${ONNC_EXTDIR}/lib/cmake/llvm" \
                          -DONNX_ROOT="${ONNC_EXTDIR}" \
                          -DONNX_NAMESPACE=onnx \
                          -DSKYPAT_ROOT="${ONNC_EXTDIR}" \
                          ${ONNC_SRCDIR}
      ;;
    *)
      fatal "unexpected error: unknown mode '${ONNC_MODE}'"
      ;;
  esac

  local PARALLEL_BUILD_FLAG=${MAX_MAKE_JOBS:+"-j${MAX_MAKE_JOBS}"}
  show "making ... #jobs=${MAX_MAKE_JOBS}"
  if [ "${IS_PREFIX_GIVEN}" = "true" ]; then
    fail_panic "Make onnc failed." ${MAKE} ${PARALLEL_BUILD_FLAG} DESTDIR="${ONNC_DESTDIR}" install
  else
    fail_panic "Make onnc failed." ${MAKE} ${PARALLEL_BUILD_FLAG} install
  fi

  show "leave"
  popd > /dev/null
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

##===----------------------------------------------------------------------===##
# Main
##===----------------------------------------------------------------------===##
if [ $# -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  usage_exit "$@"
fi

# Parse arguments and setup environment
setup_environment "$@"

# Build external libraries and libonnc
if [ "${BUILD_EXTERNAL}" != "false" ]; then
  build_external
  if [ "${EXTERNAL_ONLY}" = "true" ]; then
    exit 0
  fi
fi
build_onnc

# Package the installer
package_tarball

# Do post-build actions, such as printing summary
post_build
