
include_directories(${ONNC_INCLUDE_DIRS})
include_directories(${ONNX_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DONNX_NAMESPACE=onnx)

add_definitions(-DTOPDIR="${ONNC_SOURCE_DIR}")
add_definitions(-DBUILDDIR="${ONNC_BINARY_DIR}")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/common_calibration.pb.cc
                   COMMAND protoc --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
                   --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/common_calibration.proto
                   COMMENT "Generating common_calibration.pb.cc from common_calibration.proto"
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common_calibration.proto)

add_executable(calibration
    ${CMAKE_CURRENT_BINARY_DIR}/common_calibration.pb.cc
    main.cpp)
target_link_libraries(calibration libonnc ${ONNX_LIBRARIES} ${PROTOBUF_LIBRARIES}) 
