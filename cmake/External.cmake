option(BUILD_EXTERNAL "Build external projects" ON)
if(BUILD_EXTERNAL)
message(STATUS "External project will always be built")
endif(BUILD_EXTERNAL)

option(EXTERNAL_BUILD_IN_SOURCE "Always build in source" OFF)
if(EXTERNAL_BUILD_IN_SOURCE)
message(WARNING "External project will build in its source directory")
endif(EXTERNAL_BUILD_IN_SOURCE)

set(EXTERNAL_INSTALL_PREFIX ${ONNC_UMBRELLA_PATH}/onncroot CACHE PATH "External project install path")
message(STATUS "External project will be install at ${EXTERNAL_INSTALL_PREFIX}")
set(ONNC_UMBRELLA_EXTERNAL_PATH ${ONNC_UMBRELLA_PATH}/external)

include(ExternalProject)

####################
# onnx
find_package(onnx)
if(BUILD_EXTERNAL OR ONNX_SOURCE_DIR OR NOT ONNX_INCLUDE_DIR OR NOT ONNX_LIBRARY_DIR)
    if(NOT ONNX_SOURCE_DIR)
        set(ONNX_SOURCE_DIR ${ONNC_UMBRELLA_EXTERNAL_PATH}/onnx CACHE PATH "onnx source path")
    endif(NOT ONNX_SOURCE_DIR)
    message(STATUS "Using onnx source at ${ONNX_SOURCE_DIR}")

    # Build & install onnx
    ExternalProject_Add(Ext_project_onnx
        PREFIX onnx
        DOWNLOAD_COMMAND git submodule update --recursive ${ONNX_SOURCE_DIR}
        SOURCE_DIR ${ONNX_SOURCE_DIR}
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_PREFIX}
        BUILD_ALWAYS ${BUILD_EXTERNAL}
        BUILD_IN_SOURCE ${EXTERNAL_BUILD_IN_SOURCE}
    )
    # Set include directory
    set(ONNX_INCLUDE_DIR ${EXTERNAL_INSTALL_PREFIX}/include)
    message(STATUS "Using onnx include directory at ${ONNX_INCLUDE_DIR}")
    # Set lib directory
    set(ONNX_LIBRARY_DIR ${EXTERNAL_INSTALL_PREFIX}/lib)
    message(STATUS "Using onnx library directory at ${ONNX_LIBRARY_DIR}")
endif(BUILD_EXTERNAL OR ONNX_SOURCE_DIR OR NOT ONNX_INCLUDE_DIR OR NOT ONNX_LIBRARY_DIR)

####################
# SkyPat

find_package(SkyPat)
if(BUILD_EXTERNAL OR SKYPAT_SOURCE_DIR OR NOT SKYPAT_INCLUDE_DIR OR NOT SKYPAT_LIBRARY_DIR)
    if(NOT SKYPAT_SOURCE_DIR)
        set(SKYPAT_SOURCE_DIR ${ONNC_UMBRELLA_EXTERNAL_PATH}/SkyPat CACHE PATH "SkyPat source path")
    endif(NOT SKYPAT_SOURCE_DIR)
    message(STATUS "Using SkyPat source at ${SKYPAT_SOURCE_DIR}")
    # Build & install skypat
    ExternalProject_Add(Ext_project_skypat
        PREFIX skypat
        DOWNLOAD_COMMAND git submodule update --recursive ${SKYPAT_SOURCE_DIR}
        SOURCE_DIR ${SKYPAT_SOURCE_DIR}
        CONFIGURE_COMMAND ${SKYPAT_SOURCE_DIR}/configure --prefix=${EXTERNAL_INSTALL_PREFIX}
        BUILD_ALWAYS ${BUILD_EXTERNAL}
        BUILD_IN_SOURCE ${EXTERNAL_BUILD_IN_SOURCE}
    )
    ExternalProject_Add_Step(Ext_project_skypat autoreconf
        COMMAND ./autogen.sh
        WORKING_DIRECTORY ${SKYPAT_SOURCE_DIR}
        DEPENDERS configure
    )
    # Set include directory
    set(SKYPAT_INCLUDE_DIR ${EXTERNAL_INSTALL_PREFIX}/include)
    message(STATUS "Using SkyPat include directory at ${SKYPAT_INCLUDE_DIR}")
    # Set lib directory
    set(SKYPAT_LIBRARY_DIR ${EXTERNAL_INSTALL_PREFIX}/lib)
    message(STATUS "Using SkyPat library directory at ${SKYPAT_LIBRARY_DIR}")
endif(BUILD_EXTERNAL OR SKYPAT_SOURCE_DIR OR NOT SKYPAT_INCLUDE_DIR OR NOT SKYPAT_LIBRARY_DIR)