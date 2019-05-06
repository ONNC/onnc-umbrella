cmake_minimum_required(VERSION 3.5)
enable_testing()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ONNC_UMBRELLA_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Set build type
set(CMAKE_BUILD_TYPE "normal")
#set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type [Release(default)/Debug/Regression]")
Message("build type is ${CMAKE_BUILD_TYPE}")
 # for build.cmake.sh
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_UNITTEST "enable unittest" ON)
    set(build_mode_for_command "dbg")
elseif(CMAKE_BUILD_TYPE STREQUAL "Regression")
    option(ENABLE_UNITTEST "enable unittest" OFF)
    set(build_mode_for_command "rgn")
else()
    option(ENABLE_UNITTEST "enable unittest" OFF)
    set(build_mode_for_command "normal")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Build
# TODO: Integrate src CMake later
#add_subdirectory(src)

add_custom_target(onnc ALL
    COMMAND MAX_MAKE_JOBS=$ENV{MAX_MAKE_JOBS} ./build.cmake.sh ${build_mode_for_command} ${CMAKE_INSTALL_PREFIX}
    WORKING_DIRECTORY ${ONNC_UMBRELLA_ROOT}
)

# create folder for each model
execute_process(
	COMMAND sh make_models_test.sh ${PROJECT_BINARY_DIR}
	WORKING_DIRECTORY ${ONNC_UMBRELLA_ROOT}
	)

# Test
add_subdirectory(test)
