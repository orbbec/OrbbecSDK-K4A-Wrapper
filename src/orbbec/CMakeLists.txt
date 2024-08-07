# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(record)
add_subdirectory(frame_queue)
add_subdirectory(imu_sync)

set(TRANSFORMATION_PATH ${CMAKE_SOURCE_DIR}/src/transformation)
set(TRANSFORMATION_SOURCES
    ${TRANSFORMATION_PATH}/extrinsic_transformation.c
    ${TRANSFORMATION_PATH}/intrinsic_transformation.c
    ${TRANSFORMATION_PATH}/mode_specific_calibration.c
    ${TRANSFORMATION_PATH}/rgbz.c
    ${TRANSFORMATION_PATH}/transformation.c
)

# Create K4A library
add_library(k4a SHARED
            ob_k4a_impl.cpp
            dll_main.c
            ob_type_helper.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/version.rc
            ${TRANSFORMATION_SOURCES})

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "amd64.*|x86_64.*|AMD64.*|i686.*|i386.*|x86.*")
        target_compile_options(k4a PRIVATE "-msse4.1")
    endif()
endif()

if(CACHE_OB_CONTEXT)
target_compile_definitions(k4a PRIVATE -DCACHE_OB_CONTEXT)
endif()

# Generate k4a_export.h
# This is a platform specific header defining the macros for the export functions
# of the shared library. This header is referenced by k4a.h and needs to be installed
# with the SDK
include(GenerateExportHeader)
generate_export_header(k4a
    EXPORT_FILE_NAME "include/k4a/k4a_export.h")

configure_file(
    "${K4A_INCLUDE_DIR}/k4a/k4aversion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/k4a/k4aversion.h"
    )

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect SDK")
set(K4A_ORIGINALFILENAME "k4a.dll")
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )

# ${K4A_INCLUDE_DIR} contains k4a/k4a.h which is the primary header consumers should include
# ${CMAKE_CURRENT_BINARY_DIR}/include contains the generated k4a/k4a_export.h
target_include_directories(k4a PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${K4A_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
    )

# Find Orbbec SDK
option(USER_LOCAL_ORBBECSDK "Use local OrbbecSDK" OFF)
if(USER_LOCAL_ORBBECSDK)
    # custom path such as:
    set(OrbbecSDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OrbbecSDK_local)
else()
    # git submodule
    set(OrbbecSDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OrbbecSDK)
endif()
find_package(OrbbecSDK REQUIRED)

# Link in libraries
target_link_libraries(k4a PRIVATE
    k4ainternal::calibration
    # k4ainternal::transformation
    k4ainternal::logging
    k4ainternal::queue
    k4ainternal::frame_queue
	k4ainternal::imusync
    OrbbecSDK::OrbbecSDK
    k4ainternal::math
    k4ainternal::deloader
    k4ainternal::tewrapper
	)

# Define alias for k4a
add_library(k4a::k4a ALIAS k4a)

set(K4A_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(K4A_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(K4A_VERSION_PATCH "1")
set_target_properties(
    k4a
    PROPERTIES
        VERSION
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}.${K4A_VERSION_PATCH}"
        SOVERSION
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}")

set(NAMELINK_IF_AVAILABLE)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    set(NAMELINK_IF_AVAILABLE NAMELINK_COMPONENT development)
endif()

add_subdirectory(docs)

# Setup install
include(GNUInstallDirs)
install(
    TARGETS
        k4a
    EXPORT
        k4aTargets
    LIBRARY
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        COMPONENT
            runtime
        ${NAMELINK_IF_AVAILABLE}
    ARCHIVE
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        COMPONENT
            development
    RUNTIME
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            runtime
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    k4aConfigVersion.cmake
    VERSION ${K4A_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

install(
    EXPORT
        k4aTargets
    FILE
        k4aTargets.cmake
    NAMESPACE
        k4a::
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/k4a
    COMPONENT
        development
)

configure_file("${PROJECT_SOURCE_DIR}/cmake/k4aConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/k4aConfig.cmake" COPYONLY)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/k4aConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/k4aConfigVersion.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/k4a
    COMPONENT
        development
)

install(
    FILES
        ${K4A_INCLUDE_DIR}/k4a/k4a.h
        ${K4A_INCLUDE_DIR}/k4a/k4a.hpp
        ${K4A_INCLUDE_DIR}/k4a/k4atypes.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/k4a/k4aversion.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/k4a/k4a_export.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/k4a
    COMPONENT
        development
)

get_target_property(K4A_OUTPUT_DIR k4a RUNTIME_OUTPUT_DIRECTORY)

file(COPY ${OrbbecSDK_DLL_FILES} DESTINATION ${K4A_OUTPUT_DIR}/)
file(COPY ${OrbbecSDK_RUNTIME_RESOURCE_FILES} DESTINATION ${K4A_OUTPUT_DIR}/)

if(WIN32)
    install(FILES ${OrbbecSDK_DLL_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(FILES ${OrbbecSDK_DLL_FILES} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(FILES ${OrbbecSDK_RUNTIME_RESOURCE_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${OrbbecSDK_SCRIPT_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/scripts)
install(FILES ${CMAKE_SOURCE_DIR}/README_PUBLIC.md DESTINATION ${CMAKE_INSTALL_PREFIX} RENAME README.md)
install(FILES ${CMAKE_SOURCE_DIR}/docs/Access_AKDK_Application_Software_with_Femto_Bolt.pdf DESTINATION ${CMAKE_INSTALL_PREFIX}/doc RENAME Access_AKDK_Application_Software_with_Femto_Bolt.pdf)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:k4a>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            runtime
        OPTIONAL
    )
endif()
