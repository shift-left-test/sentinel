# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

add_library(args INTERFACE)
target_include_directories(args SYSTEM INTERFACE .)
add_library(args::lib ALIAS args)
