cmake_minimum_required(VERSION 3.5)
project(ONNC_EXTERNAL NONE)

include(ExternalProject)
include(${ONNC_EXTERNAL_SOURCE_DIR}/../src/cmake/option.cmake)
include(${ONNC_EXTERNAL_SOURCE_DIR}/../src/cmake/onnc_target.cmake)
enum_onnc_targets()

option(BUILD_EXT_ALWAYS "always build external project" OFF)

add_custom_target(build_external)
set_directory_properties(PROPERTIES EP_BASE ${CMAKE_CURRENT_BINARY_DIR})

# build LLVM
option(BUILD_LLVM "build llvm" OFF)
option_enum(NAME BUILD_LLVM_VERSION HELP "llvm version" VALUE 5.0.1 6.0.0 CHECK BUILD_LLVM)
if (BUILD_LLVM)
    ExternalProject_Add(llvm
        URL https://releases.llvm.org/${BUILD_LLVM_VERSION}/llvm-${BUILD_LLVM_VERSION}.src.tar.xz
        BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DLLVM_TARGETS_TO_BUILD=host;X86;ARM;AArch64)
endif()

# build SkyPat
ExternalProject_Add(skypat
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SkyPat
    BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
    BUILD_BYPRODUCTS Install/skypat/lib/libskypat.a # for ninja
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE})
ExternalProject_Add_Step(skypat autoreconf
    COMMAND ./autogen.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SkyPat
    DEPENDERS configure)
add_dependencies(build_external skypat)

# build ONNX
ExternalProject_Add(onnx
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/onnx
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
    BUILD_BYPRODUCTS Install/onnx/lib/libonnx.a Install/onnx/lib/libonnx_proto.a # for ninja
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)
ExternalProject_Add_Step(onnx install_hack 
    COMMAND mkdir -p <INSTALL_DIR>/lib <INSTALL_DIR>/include
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libonnx.a <INSTALL_DIR>/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libonnx_proto.a <INSTALL_DIR>/lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/onnx <INSTALL_DIR>/include/onnx
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/onnx/onnx.pb.h <INSTALL_DIR>/include/onnx
    WORKING_DIRECTORY <BINARY_DIR>
    DEPENDEES build)
add_dependencies(build_external onnx)

if (TARGET_TG)
    # build bmtap
    ExternalProject_Add(bmtap
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bmtap
        INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/libbmtap.a # for ninja
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-chip=bm1680 --enable-bmkernel-host-api=yes --enable-fw-api=bmkernel)
    ExternalProject_Add_Step(bmtap autoreconf
        COMMAND autoreconf -i
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bmtap
        DEPENDERS configure)

    ExternalProject_Get_Property(bmtap INSTALL_DIR)
    set(BMTAP_INSTALL_DIR ${INSTALL_DIR})

    # build bmnet
    ExternalProject_Add(bmnet
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bmnet
        INSTALL_DIR ${BMTAP_INSTALL_DIR}
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Build/bmnet/src/targets/.libs/libtargets.a # for ninja
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>  --enable-debug)
    ExternalProject_Add_Step(bmnet autoreconf
        COMMAND autoreconf -i
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bmnet
        DEPENDERS configure)
    add_dependencies(bmnet bmtap)
    add_dependencies(build_external bmnet)
endif()
