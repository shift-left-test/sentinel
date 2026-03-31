# Copyright (c) 2020 LG Electronics Inc.
# SPDX-License-Identifier: MIT

add_library(tinyxml2 STATIC src/tinyxml2.cpp)
target_include_directories(tinyxml2 SYSTEM PUBLIC include)
add_library(tinyxml2::lib ALIAS tinyxml2)
