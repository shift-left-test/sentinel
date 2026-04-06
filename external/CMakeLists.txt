# Copyright (c) 2026 LG Electronics Inc.
# SPDX-License-Identifier: MIT

add_subdirectory(args-6.2.3)
add_subdirectory(fmt)
add_subdirectory(tinyxml2)
add_subdirectory(yaml-cpp)

# libgit2 - build as static library with minimal features
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(USE_THREADS OFF CACHE BOOL "" FORCE)
set(USE_SSH OFF CACHE STRING "" FORCE)
set(USE_HTTPS OFF CACHE STRING "" FORCE)
set(USE_HTTP_PARSER "builtin" CACHE STRING "" FORCE)
set(USE_BUNDLED_ZLIB ON CACHE BOOL "" FORCE)
set(USE_NTLMCLIENT OFF CACHE BOOL "" FORCE)
set(SONAME OFF CACHE BOOL "" FORCE)
set(DEPRECATE_HARD ON CACHE BOOL "" FORCE)
add_subdirectory(libgit2 EXCLUDE_FROM_ALL)
target_include_directories(libgit2package SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libgit2/include>)

# ftxui - terminal UI library (must follow libgit2; find_package(Threads)
# pollutes CMake cache and breaks libgit2 GIT_USE_NSEC detection if loaded first)
add_subdirectory(ftxui)
