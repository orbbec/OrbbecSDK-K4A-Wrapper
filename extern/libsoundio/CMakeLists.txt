if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if (NOT TARGET libsoundio_static)
        # Some of the example stuff doesn't build under MSVC
        #
        set(BUILD_EXAMPLE_PROGRAMS OFF CACHE BOOL "Disable Soundio example projects")
        set(BUILD_TESTS OFF CACHE BOOL "Disable Soundio tests")
        set(SOUNDIO_STATIC_LIBNAME libsoundio_static)

        # MSVC C compiler does not provide stdatomic.h. Patch libsoundio's own
        # CMakeLists.txt to force C++ compilation for its sources so atomics.h
        # uses <atomic> instead. We do this via CMake file operations rather
        # than git apply so it works regardless of how the source was obtained.
        if (MSVC)
            set(_LIBSOUNDIO_SRC_CMAKELISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
            file(READ ${_LIBSOUNDIO_SRC_CMAKELISTS} _libsoundio_cmake_content)
            if (NOT _libsoundio_cmake_content MATCHES "LANGUAGE CXX")
                # Find the unique main include_directories block and insert our fix just before it.
                set(_insert_marker "\n\ninclude_directories(\n    \${libsoundio_SOURCE_DIR}\n    \${libsoundio_BINARY_DIR}")
                string(FIND "${_libsoundio_cmake_content}" "${_insert_marker}" _insert_pos)
                if (_insert_pos GREATER -1)
                    string(SUBSTRING "${_libsoundio_cmake_content}" 0 ${_insert_pos} _libsoundio_cmake_before)
                    string(SUBSTRING "${_libsoundio_cmake_content}" ${_insert_pos} -1 _libsoundio_cmake_after)
                    set(_libsoundio_cmake_content_patched
                        "${_libsoundio_cmake_before}\n\nif(MSVC)\n    set_source_files_properties(\${LIBSOUNDIO_SOURCES} PROPERTIES LANGUAGE CXX)\nendif()${_libsoundio_cmake_after}")
                    file(WRITE ${_LIBSOUNDIO_SRC_CMAKELISTS} "${_libsoundio_cmake_content_patched}")
                else()
                    message(WARNING "Could not find insertion point for libsoundio MSVC atomics fix")
                endif()
                unset(_insert_marker)
                unset(_insert_pos)
                unset(_libsoundio_cmake_before)
                unset(_libsoundio_cmake_after)
            endif()
            unset(_libsoundio_cmake_content)
            unset(_libsoundio_cmake_content_patched)
            unset(_LIBSOUNDIO_SRC_CMAKELISTS)
        endif()

        add_subdirectory(src EXCLUDE_FROM_ALL)

        target_include_directories(libsoundio_static PUBLIC
            src/soundio
        )

        target_compile_definitions(libsoundio_static PUBLIC "SOUNDIO_STATIC_LIBRARY")

        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_definitions(libsoundio_static PRIVATE "_CRT_SECURE_NO_WARNINGS")
        endif()

        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
            target_compile_options(libsoundio_static PRIVATE "-Wno-unused-variable")
        endif()

    else()
        message(STATUS "libsoundio_static is already a target. Skipping adding it twice")
    endif()

    add_library(libsoundio::libsoundio ALIAS libsoundio_static)
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    find_library(LIBSOUNDIO_LIB soundio)
    add_library(libsoundio::libsoundio SHARED IMPORTED GLOBAL)
    set_property(TARGET libsoundio::libsoundio PROPERTY IMPORTED_LOCATION ${LIBSOUNDIO_LIB})
else()
    message(FATAL_ERROR "Unknown system name ${CMAKE_SYSTEM_NAME}")
endif()
