cmake_minimum_required(VERSION 3.5)

project(ONNC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

####################
# external libraries

add_subdirectory(external)
ExternalProject_Get_Property(onnx SOURCE_DIR BINARY_DIR)
set(ONNX_INCLUDE_DIRS ${SOURCE_DIR} ${BINARY_DIR})
set(ONNX_LIBRARIES
    ${BINARY_DIR}/libonnx.a
    ${BINARY_DIR}/libonnx_proto.a)
ExternalProject_Get_Property(skypat INSTALL_DIR)
set(SKYPAT_INCLUDE_DIRS
    ${INSTALL_DIR}/include)
set(SKYPAT_LIBRARIES
    ${INSTALL_DIR}/lib/libskypat.a)


add_subdirectory(src)
