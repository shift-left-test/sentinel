# MIT License
#
# Copyright (c) 2020 Sung Gon Kim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(sentinel VERSION 0.2.0)

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
  HOMEPAGE "http://mod.lge.com/hub/yocto/addons/sentinel"
  VENDOR "LG Electronics"
  SUMMARY "the mutation testing tool for C/C++ projects"
  SECTION "devel"
  DEPENDS "libgit2-dev"
)
