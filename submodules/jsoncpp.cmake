###
# HID and HIDAPI
###

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/include/json/json.h")
	set(LOCAL_JSONCPP_SUBMODULE_RETRIEVED TRUE)
else()
	set(LOCAL_JSONCPP_SUBMODULE_RETRIEVED FALSE)
	if(NOT SUBPROJECT)
		message(STATUS
			"Local JSONCPP submodule not found. To download with Git, run git submodule update --init")
	endif()
endif()

option_requires(VRPN_USE_LOCAL_JSONCPP
	"Build with JSONCPP code from within VRPN source directory"
	LOCAL_JSONCPP_SUBMODULE_RETRIEVED)

if(VRPN_USE_LOCAL_JSONCPP)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp")
		set(_jsoncpp_base "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp")
	else()
		set(_jsoncpp_base "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp")
	endif()
	set(JSONCPP_INCLUDE_DIRS "${_jsoncpp_base}/include/")
	file(GLOB JSONCPP_SOURCES
		"${_jsoncpp_base}/include/json/*.h"
		"${_jsoncpp_base}/src/lib_json/*.inl"
		"${_jsoncpp_base}/src/lib_json/*.h"
		"${_jsoncpp_base}/src/lib_json/*.cpp")
	source_group("JSONCPP Submodule" FILES ${JSONCPP_SOURCES})
	set(JSONCPP_LIBRARIES "")
	set(JSONCPP_FOUND TRUE)
endif()
