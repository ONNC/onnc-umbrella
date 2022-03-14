#!/usr/bin/env bash
#                       The ONNC Project
#
source ./scripts/library.sh

##===----------------------------------------------------------------------===##
# Building functions
##===----------------------------------------------------------------------===##

function build_onnc
{
  show "building onnc with onnx namespace: ${ONNC_ONNX_NAMESPACE} ..."

  fail_panic "directory not found: ${ONNC_SRCDIR}" test -d "${ONNC_SRCDIR}"

  show "create build directory at '${ONNC_BUILDDIR}'"
  mkdir -p "${ONNC_BUILDDIR}"
  pushd "${ONNC_BUILDDIR}" > /dev/null

  local CMAKE_ARGUMENTS="-DUSE_MKLDNN=ON \
                         -DMKLDNN_ROOT=${ONNC_EXTDIR} \
                         -DCMAKE_INSTALL_PREFIX=${ONNC_PREFIX} \
                         -DLLVM_ROOT_DIR=${ONNC_EXTDIR} \
                         -DONNX_ROOT=${ONNC_EXTDIR} \
                         -DONNX_NAMESPACE=${ONNC_ONNX_NAMESPACE} \
                         -DSKYPAT_ROOT=${ONNC_EXTDIR} \
                         -DSYSTEMC_ROOT=${ONNC_EXTDIR}"
  case "$(platform)" in
    macosx)
      # SystemC needs gcc 8 to link with.
      CMAKE_ARGUMENTS+=" -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8"
    ;;
    *)
    ;;
  esac

  case "${ONNC_MODE}" in
    normal)
      CMAKE_ARGUMENTS+=" -DCMAKE_BUILD_TYPE=Release"
      ;;
    dbg)
      CMAKE_ARGUMENTS+=" -DCMAKE_BUILD_TYPE=Debug"
      ;;
    rgn)
      CMAKE_ARGUMENTS+=" -DCMAKE_BUILD_TYPE=Regression"
      ;;
    opt)
      CMAKE_ARGUMENTS+=" -DCMAKE_BUILD_TYPE=Optimized"
      ;;
    *)
      fatal "unexpected error: unknown mode '${ONNC_MODE}'"
      ;;
  esac

  fail_panic "CMake onnc failed." cmake ${CMAKE_ARGUMENTS} ${ONNC_SRCDIR}

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

function build_tools
{
  show "building onnc-umbrella tools..."

  local ONNC_UMBRELLA_BUILDDIR=${ONNC_BUILDDIR}/umbrella
  local ONNC_UMBRELLA_SOURCEDIR=${PWD}

  local INSTALLDIR=${ONNC_DESTDIR}
  if [ "${IS_PREFIX_GIVEN}" = "true" ]; then
    INSTALLDIR=${INSTALLDIR}${ONNC_PREFIX}
  fi

  show "create build directory at '${ONNC_UMBRELLA_BUILDDIR}/tools'"
  mkdir -p ${ONNC_UMBRELLA_BUILDDIR}/tools

  pushd "${ONNC_UMBRELLA_BUILDDIR}/tools" > /dev/null
    local TOOLS_SRC_DIR=${ONNC_UMBRELLA_SOURCEDIR}/tools
    for TOOL in `ls ${TOOLS_SRC_DIR}`; do
      if [ -f ${TOOLS_SRC_DIR}/$TOOL/Makefile ] || [ -f ${TOOLS_SRC_DIR}/$TOOL/makefile ] ; then
        show "building tools: '$TOOL'"
        mkdir -p $TOOL
        make -C ${TOOLS_SRC_DIR}/$TOOL \
          BUILD_DIR=${PWD}/$TOOL \
          INSTALL_DIR=${INSTALLDIR} \
          EXTERNAL_DIR=${ONNC_EXTDIR} \
          install
      fi
    done
  popd > /dev/null
}

##===----------------------------------------------------------------------===##
# Main
##===----------------------------------------------------------------------===##
if [ $# -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  usage_exit "$@"
fi

# Parse arguments and setup environment
setup_environment "$@"

# Build external libraries, libonnc and tools
if [ "${BUILD_EXTERNAL}" != "false" ]; then
  build_external
  if [ "${EXTERNAL_ONLY}" = "true" ]; then
    exit 0
  fi
fi
build_onnc
build_tools

# Package the installer
package_tarball

# Do post-build actions, such as printing summary
post_build
