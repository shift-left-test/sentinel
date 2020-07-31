build_static_library(
  NAME tinyxml2
  SRCS src/tinyxml2.cpp
  PUBLIC_HEADERS include
)

set_target_properties(
  tinyxml2 PROPERTIES
  CXX_CLANG_TIDY ""
  CXX_CPPCHECK ""
  CXX_CPPLINT ""
)
