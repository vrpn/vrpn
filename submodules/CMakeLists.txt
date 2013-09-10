
# This file includes a cmake file per submodule, and then
# sends the variables it sets to the parent scope, which is the parent
# (main) directory.

###
# HIDAPI
###

include("hidapi.cmake")
if(VRPN_USE_LOCAL_HIDAPI)
	set(HIDAPI_FOUND ${HIDAPI_FOUND} PARENT_SCOPE)
	set(HIDAPI_INCLUDE_DIRS ${HIDAPI_INCLUDE_DIRS} PARENT_SCOPE)
	set(HIDAPI_LIBRARIES ${HIDAPI_LIBRARIES} PARENT_SCOPE)
	set(HIDAPI_SOURCES ${HIDAPI_SOURCES} PARENT_SCOPE)
endif()

###
# JSONCPP
###
include("jsoncpp.cmake")
if(VRPN_USE_LOCAL_JSONCPP)
	set(JSONCPP_FOUND ${JSONCPP_FOUND} PARENT_SCOPE)
	set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS} PARENT_SCOPE)
	set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARIES} PARENT_SCOPE)
	set(JSONCPP_SOURCES ${JSONCPP_SOURCES} PARENT_SCOPE)
endif()
