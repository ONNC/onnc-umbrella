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
option_enum(NAME CMAKE_BUILD_TYPE_LLVM VALUE Debug Release CHECK BUILD_LLVM)
if (BUILD_LLVM)
    ExternalProject_Add(llvm
        URL https://releases.llvm.org/${BUILD_LLVM_VERSION}/llvm-${BUILD_LLVM_VERSION}.src.tar.xz
        BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_LLVM}
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
option_enum(NAME CMAKE_BUILD_TYPE_ONNX VALUE Debug Release)
ExternalProject_Add(onnx
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/onnx
    BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
    BUILD_BYPRODUCTS Install/onnx/lib/libonnx.a Install/onnx/lib/libonnx_proto.a # for ninja
    CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_ONNX}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)
add_dependencies(build_external onnx)

option_enum(NAME CMAKE_BUILD_TYPE_TG VALUE Debug Release CHECK TARGET_TG)
option_enum(NAME TARGET_TG_ARCH VALUE BM1880 BM1680 BM1682 CHECK TARGET_TG)
if (TARGET_TG)
    ExternalProject_Add(bmtap2
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bmtap2
        BUILD_BYPRODUCTS
        Install/bmtap2/lib/libbmnet.so
        Install/bmtap2/lib/libbmkernel-static.a
        Install/bmtap2/lib/libbmutils.so
        Install/bmtap2/lib/libbmruntime.so
        BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_TG}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCHIP=${TARGET_TG_ARCH}
        -Dbuild_bmnet=ON)
    add_dependencies(build_external bmtap2)
endif()
