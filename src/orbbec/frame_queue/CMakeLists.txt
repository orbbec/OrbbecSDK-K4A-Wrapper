# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_frame_queue STATIC 
            frame_queue.c
            )

# Consumers should #include <k4ainternal/queue.h>
target_include_directories(k4a_frame_queue PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_frame_queue PUBLIC 
    azure::aziotsharedutil
    k4ainternal::allocator
    k4ainternal::logging
)

# Define alias for other targets to link against
add_library(k4ainternal::frame_queue ALIAS k4a_frame_queue)