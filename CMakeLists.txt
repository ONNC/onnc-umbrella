cmake_minimum_required(VERSION 3.5)

project(ONNC_UMBRELLA)

enable_testing()

####################
# external libraries

add_subdirectory(external)
ExternalProject_Get_Property(onnx INSTALL_DIR)
set(ONNX_INSTALL_DIR ${INSTALL_DIR} CACHE PATH "onnx install path")
ExternalProject_Get_Property(skypat INSTALL_DIR)
set(SKYPAT_INSTALL_DIR ${INSTALL_DIR} CACHE PATH "skypat install path")


add_subdirectory(src)
