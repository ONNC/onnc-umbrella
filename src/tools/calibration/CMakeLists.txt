
include_directories(${ONNC_INCLUDE_DIRS})
include_directories(${ONNX_INCLUDE_DIRS})
add_definitions(-DONNX_NAMESPACE=onnx)

add_definitions(-DTOPDIR="${ONNC_SOURCE_DIR}")
add_definitions(-DBUILDDIR="${ONNC_BINARY_DIR}")

add_executable(calibration main.cpp)
target_link_libraries(calibration libonnc ${ONNX_LIBRARIES} ${PROTOBUF_LIBRARIES}) 
