# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

if(NOT ENABLE_TEST)
  return()
endif()

find_package(LibGit2 REQUIRED)

build_static_library(
  NAME git-harness
  SRCS GitHarness.cpp
  PUBLIC_HEADERS . ${LIBGIT2_INCLUDE_DIR}
  LIBS sentinel-core ${LIBGIT2_LIBRARIES}
  NO_INSTALL
)
