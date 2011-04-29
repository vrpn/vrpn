# - try to find JSONCPP library
#
# Cache Variables: (probably not for direct use in your scripts)
#  JSONCPP_INCLUDE_DIR
#  JSONCPP_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  JSONCPP_FOUND
#  JSONCPP_INCLUDE_DIRS
#  JSONCPP_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Author:
# 2011 Philippe Crassous (ENSAM ParisTech / Institut Image) p.crassous _at_ free.fr


set(JSONCPP_ROOT_DIR
	"${JSONCPP_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for JSONCPP")

find_library(
	JSONCPP_LIBRARY
	NAMES 
		json_vc6_libmt 
		json_vc7_libmt 
		json_vc71_libmt 
		json_vc71_libmt 
		json_vc80_libmt
		json_vc90_libmt
		json_suncc_libmt
		json_vacpp_libmt
		json_mingw_libmt
		json_linux-gcc_libmt
	PATHS "${JSONCPP_ROOT_DIR}/libs"
	PATH_SUFFIXES suncc vacpp mingw msvc6 msvc7 msvc71 msvc80 msvc90 linux-gcc 

	)

get_filename_component(_libdir "${JSONCPP_LIBRARY}" PATH)

find_path(JSONCPP_INCLUDE_DIR
	NAMES json.h
	PATHS "${JSONCPP_ROOT_DIR}"
	PATH_SUFFIXES "include/json"
	)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSONCPP
	DEFAULT_MSG
	JSONCPP_LIBRARY
	JSONCPP_INCLUDE_DIR
	)

if(JSONCPP_FOUND)
	set(JSONCPP_LIBRARIES "${JSONCPP_LIBRARY}")
	set(JSONCPP_INCLUDE_DIRS "${JSONCPP_INCLUDE_DIR}/..")
endif()

mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIBRARY)
