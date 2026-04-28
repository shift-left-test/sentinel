# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(sentinel VERSION 1.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SKIP_RPATH TRUE)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(GNUInstallDirs)

add_subdirectory(external)

if(CMAKE_TESTING_ENABLED)
  add_library(coverage_options INTERFACE)
  target_compile_options(coverage_options INTERFACE -O0 -fprofile-arcs -ftest-coverage)
  target_link_options(coverage_options INTERFACE -fprofile-arcs -ftest-coverage)

  find_program(CPPCHECK_PROGRAM NAMES cppcheck)
  if(CPPCHECK_PROGRAM)
    add_custom_target(cppcheck
      COMMAND ${CPPCHECK_PROGRAM}
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --enable=warning,style,performance,portability
        --library=googletest --inline-suppr --error-exitcode=1
        -i ${CMAKE_SOURCE_DIR}/external
      COMMENT "Running cppcheck..."
    )
  endif()

  find_program(CPPLINT_PROGRAM NAMES cpplint cpplint.py)
  if(CPPLINT_PROGRAM)
    add_custom_target(cpplint
      COMMAND ${CPPLINT_PROGRAM} --recursive
        ${CMAKE_SOURCE_DIR}/src/
        ${CMAKE_SOURCE_DIR}/include/
        ${CMAKE_SOURCE_DIR}/test/
      COMMENT "Running cpplint..."
    )
  endif()

  find_package(Doxygen)
  if(NOT DOXYGEN_FOUND)
    message(WARNING "Unable to locate the doxygen executable")
    add_custom_target(doc)
  elseif(NOT EXISTS ${CMAKE_SOURCE_DIR}/Doxyfile)
    message(WARNING "Unable to locate the Doxyfile")
    add_custom_target(doc)
  else()
    add_custom_target(doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
  endif()

  enable_testing()
endif()

add_subdirectory(src)
add_subdirectory(test)

configure_file(${CMAKE_SOURCE_DIR}/include/sentinel/version.hpp.in ${CMAKE_SOURCE_DIR}/include/sentinel/version.hpp)
configure_file(${CMAKE_SOURCE_DIR}/resources/man/sentinel.1.in ${CMAKE_SOURCE_DIR}/resources/man/sentinel.1)

install(FILES ${CMAKE_SOURCE_DIR}/resources/man/sentinel.1 DESTINATION man/man1)

# CPack configuration
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sung Gon Kim <sunggon82.kim@lge.com>")
set(CPACK_DEBIAN_PACKAGE_CONTACT "sunggon82.kim@lge.com")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/shift-left-test/sentinel")
set(CPACK_PACKAGE_VENDOR "LG Electronics")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "the mutation testing tool for C/C++ projects")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE.md")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
endif()

find_program(DPKG_PATH dpkg)
if(DPKG_PATH)
  execute_process(
    COMMAND "${DPKG_PATH}" --print-architecture
    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
endif()

set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)
