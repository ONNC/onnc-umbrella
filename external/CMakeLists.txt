cmake_minimum_required(VERSION 3.5)
project(ONNC_EXTERNAL)

include(ExternalProject)
include(${ONNC_EXTERNAL_SOURCE_DIR}/../src/cmake/option.cmake)

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
find_package(PythonLibs REQUIRED)
find_package(PythonInterp REQUIRED)
ExternalProject_Add(onnx
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/onnx
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    BUILD_ALWAYS ${BUILD_EXT_ALWAYS}
    BUILD_BYPRODUCTS Install/onnx/lib/libonnx.a Install/onnx/lib/libonnx_proto.a # for ninja
    CMAKE_ARGS
    -DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIRS}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    -DBUILD_ONNX_PYTHON=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)
ExternalProject_Add_Step(onnx install_hack 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different onnx_cpp2py_export.so libonnx_cpp2py_export${CMAKE_SHARED_LIBRARY_SUFFIX}
    COMMAND mkdir -p <INSTALL_DIR>/lib <INSTALL_DIR>/include
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libonnx.a <INSTALL_DIR>/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libonnx_proto.a <INSTALL_DIR>/lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/onnx <INSTALL_DIR>/include/onnx
    COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/onnx/onnx.pb.h <INSTALL_DIR>/include/onnx
    WORKING_DIRECTORY <BINARY_DIR>
    DEPENDEES build)
add_dependencies(build_external onnx)
