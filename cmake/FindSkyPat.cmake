find_library(SKYPAT_LIBRARIES
    NAMES skypat
)
find_path(SKYPAT_INCLUDE_DIR
    NAMES skypat/skypat.h
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SKYPAT DEFAULT_MSG SKYPAT_INCLUDE_DIR SKYPAT_LIBRARIES)
link_directories(${SKYPAT_LIBRARIES})
include_directories(${SKYPAT_INCLUDE_DIR})