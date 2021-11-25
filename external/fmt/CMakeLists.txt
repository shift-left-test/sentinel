# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

build_external_static_library(
  NAME fmt
  SRCS src/format.cc src/os.cc
  PUBLIC_HEADERS include
  NO_INSTALL
)
