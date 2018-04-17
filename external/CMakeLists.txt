include(ExternalProject)

option(BUILD_EXT_ALWAYS "always build external project" OFF)

add_custom_target(build_external)
set_directory_properties(PROPERTIES EP_BASE ${CMAKE_CURRENT_BINARY_DIR})

# build LLVM
option(BUILD_LLVM "build llvm" OFF)
if (BUILD_LLVM)
    set(LLVM_VERSION 5.0.1 CACHE STRING "llvm version")
    ExternalProject_Add(llvm
        URL https://releases.llvm.org/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz
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
    BUILD_BYPRODUCTS Build/onnx/libonnx.a Build/onnx/libonnx_proto.a # for ninja
    CMAKE_ARGS
    -DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIRS}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    -DBUILD_ONNX_PYTHON=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)
ExternalProject_Add_Step(onnx rename
    COMMAND ${CMAKE_COMMAND} -E copy_if_different onnx_cpp2py_export.so libonnx_cpp2py_export${CMAKE_SHARED_LIBRARY_SUFFIX}
    WORKING_DIRECTORY <BINARY_DIR>
    DEPENDEES build)
add_dependencies(build_external onnx)
