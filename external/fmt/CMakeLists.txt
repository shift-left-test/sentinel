# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

add_library(fmt STATIC src/format.cc src/os.cc)
target_include_directories(fmt SYSTEM PUBLIC include)
add_library(fmt::lib ALIAS fmt)
