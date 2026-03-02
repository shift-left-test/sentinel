# Copyright (c) 2026 LG Electronics Inc.
# SPDX-License-Identifier: MIT

build_external_static_library(
  NAME yaml-cpp
  SRCS
    src/binary.cpp
    src/convert.cpp
    src/depthguard.cpp
    src/directives.cpp
    src/emit.cpp
    src/emitfromevents.cpp
    src/emitter.cpp
    src/emitterstate.cpp
    src/emitterutils.cpp
    src/exceptions.cpp
    src/exp.cpp
    src/fptostring.cpp
    src/memory.cpp
    src/node.cpp
    src/node_data.cpp
    src/nodebuilder.cpp
    src/nodeevents.cpp
    src/null.cpp
    src/ostream_wrapper.cpp
    src/parse.cpp
    src/parser.cpp
    src/regex_yaml.cpp
    src/scanner.cpp
    src/scanscalar.cpp
    src/scantag.cpp
    src/scantoken.cpp
    src/simplekey.cpp
    src/singledocparser.cpp
    src/stream.cpp
    src/tag.cpp
  PUBLIC_HEADERS include
  NO_INSTALL
)
