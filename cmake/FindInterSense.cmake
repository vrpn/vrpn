# - try to find InterSense library
#
# Cache Variables: (probably not for direct use in your scripts)
#  INTERSENSE_INCLUDE_DIR
#  INTERSENSE_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  INTERSENSE_FOUND
#  INTERSENSE_INCLUDE_DIRS
#  INTERSENSE_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Author:
# 2013 Eric Marsh <bits@wemarsh.com>
# http://wemarsh.com/
# Kognitiv Neuroinformatik, Universität Bremen
#
# (building on Ryan Pavlik's templates)


find_library(INTERSENSE_LIBRARY
  NAMES isense)

find_path(INTERSENSE_INCLUDE_DIR
  NAMES isense.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(INTERSENSE
  DEFAULT_MSG
  INTERSENSE_LIBRARY
  INTERSENSE_INCLUDE_DIR)

if(INTERSENSE_FOUND)
  set(INTERSENSE_LIBRARIES "${INTERSENSE_LIBRARY}" ${CMAKE_DL_LIBS})

  set(INTERSENSE_INCLUDE_DIRS "${INTERSENSE_INCLUDE_DIR}")
endif()

mark_as_advanced(INTERSENSE_INCLUDE_DIR INTERSENSE_LIBRARY)
