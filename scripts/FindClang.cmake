# The following variables are defined:
#  LLVM_CONFIG_EXECUTABLE - llvm-config executable name
#  LLVM_INCLUDE_DIRS      - where to find llvm include files
#  LLVM_LIBRARY_DIRS      - where to find llvm libs
#  LLVM_CXXFLAGS          - llvm C++ compiler flags
#  LLVM_CFLAGS            - llvm C compiler flags
#  LLVM_LFLAGS            - llvm linker flags
#  LLVM_LIBS              - list of llvm libs
#  CLANG_LIBS             - list of clang libs
#  LLVM_CLANG_LIBS        - combined list of llvm and clang libs

# List up possible names of llvm-config executable
# Yocto Version: LLVM Version
# Krogoth: 3.8
# Morty: 3.9 (3.9.1)
# Pyro: 4.0 (4.0.1)
# Rocko: 5.0 (5.0.1)
# Sumo: 6.0 (6.0.1)
# Thud: 7.1 (7.1.0)
# Warrior: 8.0 (8.0.1)
# Zeus: 9.0 (9.0.1)
# Dunfell: 10.0 (10.0.1)
# Gatesgarth: 11.0 (11.0.1)
# Hardknott: 12.0 (12.0.0)
# Honister: 13.0 (13.0.0)
# Kirkstone: 14.0 (14.0.3)
# Langdale: 15.0 (15.0.1)
# Mickledore: 16.0 (16.0.0)
# Nanbield: 17.0 (17.0.1)
set(llvm_config_names llvm-config
                      llvm-config-17.0 llvm-config170 llvm-config-17
                      llvm-config-16.0 llvm-config160 llvm-config-16
                      llvm-config-15.0 llvm-config150 llvm-config-15
                      llvm-config-14.0 llvm-config140 llvm-config-14
                      llvm-config-13.0 llvm-config130 llvm-config-13
                      llvm-config-12.0 llvm-config120 llvm-config-12
                      llvm-config-11.0 llvm-config110 llvm-config-11
                      llvm-config-10.0 llvm-config100 llvm-config-10
                      llvm-config-9.0 llvm-config90 llvm-config-9
                      llvm-config-8.0 llvm-config80 llvm-config-8
                      llvm-config-7.0 llvm-config70 llvm-config-7
                      llvm-config-6.0 llvm-config60 llvm-config-6
                      llvm-config-5.0 llvm-config50 llvm-config-5
                      llvm-config-4.0 llvm-config40 llvm-config-4
                      llvm-config-3.9 llvm-config-39
                      llvm-config-3.8 llvm-config-38)

# Search for llvm-config executable among the listed llvm_config_names.
# If user specifies LLVM_ROOT_DIR, find llvm-config in there first.
# If not, find llvm-config in system default locations such as /usr/local/bin.
find_program(LLVM_CONFIG_EXECUTABLE
  NAMES ${llvm_config_names}
  PATHS ${LLVM_ROOT_DIR}/bin NO_DEFAULT_PATH
  DOC "Path to llvm-config tool.")
find_program(LLVM_CONFIG_EXECUTABLE NAMES ${llvm_config_names})
mark_as_advanced(LLVM_CONFIG_EXECUTABLE)

if (NOT LLVM_CONFIG_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find 'llvm-config' executable")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
  OUTPUT_VARIABLE LLVM_LIBRARY_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
  OUTPUT_VARIABLE LLVM_CFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --link-static
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find Clang libraries
macro(FIND_AND_ADD_CLANG_LIB _libname_)
  string(TOUPPER ${_libname_} _prettylibname_)
  find_library(CLANG_${_prettylibname_}_LIB NAMES ${_libname_} HINTS ${LLVM_LIBRARY_DIRS} ${ARGN})
  if (CLANG_${_prettylibname_}_LIB)
    set(CLANG_LIBS ${CLANG_LIBS} ${CLANG_${_prettylibname_}_LIB})
  endif()
endmacro(FIND_AND_ADD_CLANG_LIB)

FIND_AND_ADD_CLANG_LIB(clangFrontend)
FIND_AND_ADD_CLANG_LIB(clangParse)
FIND_AND_ADD_CLANG_LIB(clangRewrite)
FIND_AND_ADD_CLANG_LIB(clangRewriteFrontend)
FIND_AND_ADD_CLANG_LIB(clangSerialization)
FIND_AND_ADD_CLANG_LIB(clangSema)
FIND_AND_ADD_CLANG_LIB(clangEdit)
FIND_AND_ADD_CLANG_LIB(clangLex)
FIND_AND_ADD_CLANG_LIB(clangAnalysis)
FIND_AND_ADD_CLANG_LIB(clangAST)
FIND_AND_ADD_CLANG_LIB(clangASTMatchers)
FIND_AND_ADD_CLANG_LIB(clangStaticAnalyzerFrontend)
FIND_AND_ADD_CLANG_LIB(clangTooling)
FIND_AND_ADD_CLANG_LIB(clangDriver)
FIND_AND_ADD_CLANG_LIB(clangBasic)
FIND_AND_ADD_CLANG_LIB(clangSupport)

if (NOT CLANG_LIBS)
  message(FATAL_ERROR "Could NOT find Clang libraries in ${LLVM_LIBRARY_DIRS}")
endif()

set(LLVM_CLANG_LIBS ${CLANG_LIBS} ${LLVM_LIBS} pthread z tinfo zstd)
set(CLANG_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})

mark_as_advanced(LLVM_CLANG_LIBS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(llvmclang LLVM_CLANG_LIBS CLANG_INCLUDE_DIRS)

# message(STATUS "Found ${LLVM_CONFIG_EXECUTABLE}")
# message(STATUS "  Include dirs  : ${LLVM_INCLUDE_DIRS}")
# message(STATUS "  Library dirs  : ${LLVM_LIBRARY_DIRS}")
# message(STATUS "  LLVM libraries: ${LLVM_LIBS}")
