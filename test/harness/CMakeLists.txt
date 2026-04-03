# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

if(NOT CMAKE_TESTING_ENABLED)
  return()
endif()

add_library(git-harness STATIC GitHarness.cpp)
target_include_directories(git-harness PUBLIC .)
target_link_libraries(git-harness PUBLIC sentinel-core)
