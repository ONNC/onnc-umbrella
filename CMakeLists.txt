cmake_minimum_required(VERSION 3.5)

project(ONNC_UMBRELLA NONE)

option(EXTERNAL_BUILD_ALWAYS "Always build external" OFF)
option(EXTERNAL_BUILD_IN_SOURCE "Always build in source" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(ONNC_UMBRELLA_PATH ${CMAKE_CURRENT_LIST_DIR})

enable_testing()

####################
# external libraries

include(External)
