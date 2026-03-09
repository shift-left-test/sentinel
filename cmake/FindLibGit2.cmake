# The following variables are defined:
#  LIBGIT2_FOUND        - System has libgit2
#  LIBGIT2_INCLUDE_DIR  - The libgit2 include directory
#  LIBGIT2_LIBRARIES    - The libraries needed to use libgit2
#  LIBGIT2_DEFINITIONS  - Compiler switches required for using libgit2

set(LIBGIT2_DEFINITIONS ${PC_LIBGIT2_CFLAGS_OTHER})

find_path(LIBGIT2_INCLUDE_DIR NAMES git2.h
   HINTS
   ${PC_LIBGIT2_INCLUDEDIR}
   ${PC_LIBGIT2_INCLUDE_DIRS}
)
mark_as_advanced(LIBGIT2_INCLUDE_DIR)

find_library(LIBGIT2_LIBRARIES NAMES git2
   HINTS
   ${PC_LIBGIT2_LIBDIR}
   ${PC_LIBGIT2_LIBRARY_DIRS}
)
mark_as_advanced(LIBGIT2_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libgit2 DEFAULT_MSG LIBGIT2_LIBRARIES LIBGIT2_INCLUDE_DIR)
