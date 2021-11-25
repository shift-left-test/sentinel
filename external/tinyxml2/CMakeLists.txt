# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

build_external_static_library(
  NAME tinyxml2
  SRCS src/tinyxml2.cpp
  PUBLIC_HEADERS include
  NO_INSTALL
)
