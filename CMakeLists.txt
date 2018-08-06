cmake_minimum_required(VERSION 3.5)

project(ONNC_UMBRELLA NONE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(ONNC_UMBRELLA_PATH ${CMAKE_CURRENT_LIST_DIR})

enable_testing()

####################
# external libraries
include(External)

####################
# onnc source
ExternalProject_Add(onnc_project
    PREFIX src
    GIT_REPOSITORY https://github.com/ONNC/onnc.git
    SOURCE_DIR ${ONNC_UMBRELLA_PATH}/src
    CMAKE_ARGS 
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_LLVM}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)