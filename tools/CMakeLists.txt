# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(deversion)
add_subdirectory(k4aviewer)
add_subdirectory(k4afastcapture_streaming)
add_subdirectory(k4afastcapture_trigger)
add_subdirectory(k4arecorder)

if(NOT ${BUILD_OB_K4A_WRAPPER})
    add_subdirectory(updater)
endif()