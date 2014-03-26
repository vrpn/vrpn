###
# HID and HIDAPI
###

if(EXISTS "${VRPN_SOURCE_DIR}/submodules/jsoncpp/jsoncpp/include/json/json.h")
	set(LOCAL_JSONCPP_SUBMODULE_RETRIEVED TRUE)
else()
	message(STATUS
		"Local JSONCPP submodule not found. To download with Git, run git submodule update --init")
endif()

option_requires(VRPN_USE_LOCAL_JSONCPP
	"Build with HIDAPI code from within VRPN source directory"
	LOCAL_HIDAPI_SUBMODULE_RETRIEVED)

if(VRPN_USE_LOCAL_JSONCPP)
	set(JSONCPP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/")
	set(JSONCPP_SOURCES
		# jsoncpp/jsoncpp/include/json/version.h # not included as a "generated" file
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/forwards.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/reader.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/assertions.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/value.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/writer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/config.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/json.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/autolink.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/include/json/features.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_value.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_batchallocator.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_internalarray.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_writer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_tool.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_internalmap.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_valueiterator.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp/src/lib_json/json_reader.cpp")
	source_group("JSONCPP Submodule" FILES ${JSONCPP_SOURCES})
	set(JSONCPP_LIBRARIES "")
	set(JSONCPP_FOUND TRUE)
endif()
