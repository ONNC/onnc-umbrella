
include_directories(SYSTEM ${ONNC_INCLUDE_DIRS})
include_directories(SYSTEM ${ONNX_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR})
include_directories(SYSTEM ${PYTORCH_INCLUDE_DIRS})

add_definitions(-DTOPDIR="${ONNC_SOURCE_DIR}")
add_definitions(-DBUILDDIR="${ONNC_BINARY_DIR}")

find_library(GLOG_LIB glog)


add_compile_options(-Wall -Werror)

add_executable(calibration
    Calibration.cpp
    Kld.cpp
    ONNXOptimizer.cpp
    ONNXShapeInference.cpp
    insertDummyCtable.cpp
    main.cpp
    quantizeWeight.cpp)
target_link_libraries(calibration
    libonnc
    ${ONNX_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${PYTORCH_LIBRARIES}
    ${GLOG_LIB})
add_test(NAME calibration COMMAND calibration -h)
