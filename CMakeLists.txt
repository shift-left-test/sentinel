# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(sentinel VERSION 0.4.1)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/scripts)

include(CMakeUtils)

set_cxx_standard(14)
set_default_build_type(Release)

if(ENABLE_TEST)
  enable_static_analysis(NO_IWYU)
  enable_testing()
  enable_test_coverage()
  enable_doxygen()
endif()

add_all_subdirectories()

install(FILES ${CMAKE_SOURCE_DIR}/resources/man/sentinel.1 DESTINATION man/man1)

build_debian_package(
  MAINTAINER "Sung Gon Kim <sunggon82.kim@lge.com>"
  CONTACT "sunggon82.kim@lge.com"
  HOMEPAGE "https://github.com/shift-left-test/sentinel"
  VENDOR "LG Electronics"
  SUMMARY "the mutation testing tool for C/C++ projects"
  SECTION "devel"
  DEPENDS "libgit2-dev"
)
