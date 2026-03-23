# The following variables are defined:
#  LIBGIT2_FOUND        - System has libgit2
#  LIBGIT2_INCLUDE_DIR  - The libgit2 include directory
#  LIBGIT2_LIBRARIES    - The libraries needed to use libgit2
#  LIBGIT2_DEFINITIONS  - Compiler switches required for using libgit2

find_path(LIBGIT2_INCLUDE_DIR NAMES git2.h)
mark_as_advanced(LIBGIT2_INCLUDE_DIR)

find_library(LIBGIT2_LIBRARIES NAMES git2)
mark_as_advanced(LIBGIT2_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libgit2 DEFAULT_MSG LIBGIT2_LIBRARIES LIBGIT2_INCLUDE_DIR)
