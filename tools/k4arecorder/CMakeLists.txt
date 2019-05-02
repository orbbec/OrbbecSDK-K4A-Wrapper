# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(k4arecorder main.cpp recorder.cpp ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

target_link_libraries(k4arecorder PRIVATE
    k4a::k4a
    k4a::k4arecord
    )

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect Recording Tool")
set(K4A_ORIGINALFILENAME "k4arecorder.exe")
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )