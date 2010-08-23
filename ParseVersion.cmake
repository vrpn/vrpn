###
# Get the version number by parsing vrpn_Connection.C
###
include(WarningDev.cmake)
set(_vrpn_version_file "${CMAKE_CURRENT_SOURCE_DIR}/vrpn_Connection.C")
if(EXISTS "${_vrpn_version_file}")
	file(READ "${_vrpn_version_file}" _vrpn_version_contents)
endif()
set(_vrpn_version_regex "vrpn: ver. ([0-9]+).([0-9]+)")
string(REGEX
	MATCH
	"${_vrpn_version_regex}"
	_dummy
	"${_vrpn_version_contents}")

if(CMAKE_MATCH_1)
	set(CPACK_PACKAGE_VERSION_MAJOR "${CMAKE_MATCH_1}")
else()
	set(CPACK_PACKAGE_VERSION_MAJOR "07")
	warning_dev("Could not parse major version from vrpn_Connection.C")
endif()

if(CMAKE_MATCH_2)
	set(CPACK_PACKAGE_VERSION_MINOR "${CMAKE_MATCH_2}")
else()
	set(CPACK_PACKAGE_VERSION_MAJOR "26")
	warning_dev("Could not parse minor version from vrpn_Connection.C")
endif()
