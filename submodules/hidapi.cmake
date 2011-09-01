###
# HID and HIDAPI
###

# Local HIDAPI requirements
if(WIN32)
#	find_package(WinHID)
#	set(HIDAPI_DEPS_CHECK WINHID_FOUND)

elseif(APPLE)
	find_package(MacHID)
	set(HIDAPI_DEPS_CHECK MACHID_FOUND)

elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	find_package(Libusb1)
	set(HIDAPI_DEPS_CHECK LIBUSB1_FOUND)
endif()


endif()

if(EXISTS "${VRPN_SOURCE_DIR}/submodules/hidapi/hidapi/hidapi.h")
	set(LOCAL_HIDAPI_SUBMODULE_RETRIEVED TRUE)
else()
	message(STATUS "Local HIDAPI submodule not found. To download with Git, run git submodule update --init")
endif()

option_requires(VRPN_USE_LOCAL_HIDAPI
	"Build with HIDAPI code from within VRPN source directory"
	LOCAL_HIDAPI_SUBMODULE_RETRIEVED
	${HIDAPI_DEPS_CHECK})

if(VRPN_USE_LOCAL_HIDAPI)
	set(HIDAPI_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/hidapi")
	set(HIDAPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/hidapi/hidapi.h")
	set(HIDAPI_FOUND TRUE)
	if(APPLE)
		list(APPEND HIDAPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/mac/hid.c")
		set(HIDAPI_LIBRARIES ${MACHID_LIBRARIES})

	elseif(WIN32)
		list(APPEND HIDAPI_SOURCES "${PROJECT_SOURCE_DIR}/vrpn_Local_HIDAPI.C")
		set(HIDAPI_LIBRARIES ${WINHID_LIBRARIES} setupapi)

	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
			list(APPEND HIDAPI_SOURCES "${PROJECT_SOURCE_DIR}/vrpn_HIDAPI_Linux_Hack.c")
			set(HIDAPI_LIBRARIES ${LIBUSB1_LIBRARIES})
			list(APPEND HIDAPI_INCLUDE_DIRS ${LIBUSB1_INCLUDE_DIRS})
		find_library(HIDAPI_LIBRT rt)
		if(HIDAPI_LIBRT)
			mark_as_advanced(HIDAPI_LIBRT)
			list(APPEND HIDAPI_LIBRARIES ${HIDAPI_LIBRT})
		endif()
	else()
		set(HIDAPI_FOUND FALSE)
	endif()

	source_group("HIDAPI Submodule" FILES ${HIDAPI_SOURCES})
endif()
