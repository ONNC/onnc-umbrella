cmake_minimum_required(VERSION 3.5)

project(ONNC_UMBRELLA NONE)

enable_testing()

####################
# external libraries

add_subdirectory(external)
ExternalProject_Get_Property(onnx INSTALL_DIR)
set(ONNX_INSTALL_DIR ${INSTALL_DIR} CACHE PATH "onnx install path")
ExternalProject_Get_Property(skypat INSTALL_DIR)
set(SKYPAT_INSTALL_DIR ${INSTALL_DIR} CACHE PATH "skypat install path")

if (TARGET_TG)
    ExternalProject_Get_Property(bmtap INSTALL_DIR)
    set(BM_INSTALL_DIR ${INSTALL_DIR} CACHE PATH "bmtap/bmnet install path")
    ExternalProject_Get_Property(bmtap SOURCE_DIR)
    set(BMTAP_SOURCE_DIR ${SOURCE_DIR} CACHE PATH "bmtap source path")
    ExternalProject_Get_Property(bmnet SOURCE_DIR)
    set(BMNET_SOURCE_DIR ${SOURCE_DIR} CACHE PATH "bmnet source path")
    ExternalProject_Get_Property(bmnet BINARY_DIR)
    set(BMNET_BINARY_DIR ${BINARY_DIR} CACHE PATH "bmnet binary path")
endif()

add_subdirectory(src)
