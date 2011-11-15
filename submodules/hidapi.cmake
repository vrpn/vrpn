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
	find_library(HIDAPI_LIBUDEV
		udev)
	find_path(HIDAPI_HIDRAW_INCLUDE_DIR
		linux/hidraw.h)
	find_path(HIDAPI_LIBUDEV_INCLUDE_DIR
		libudev.h)
	if(HIDAPI_LIBUDEV AND HIDAPI_HIDRAW_INCLUDE_DIR AND HIDAPI_LIBUDEV_INCLUDE_DIR)
		mark_as_advanced(HIDAPI_LIBUDEV HIDAPI_HIDRAW_INCLUDE_DIR HIDAPI_LIBUDEV_INCLUDE_DIR)
		set(HIDAPI_LIBUDEV_FOUND YES)
	else()
		set(HIDAPI_LIBUDEV_FOUND NO)
	endif()

	if(HIDAPI_LIBUDEV_FOUND OR LIBUSB1_FOUND)
		set(HIDAPI_LINUX_BACKEND_FOUND YES)
	else()
		set(HIDAPI_LINUX_BACKEND_FOUND NO)
	endif()
	set(HIDAPI_DEPS_CHECK HIDAPI_LINUX_BACKEND_FOUND)
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
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hidapi/windows/hid.cpp)
			# Old hidapi
			list(APPEND HIDAPI_SOURCES "${PROJECT_SOURCE_DIR}/vrpn_Local_HIDAPI.C")
		else()
			# New hidapi
			list(APPEND HIDAPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/windows/hid.c")
		endif()
		set(HIDAPI_LIBRARIES ${WINHID_LIBRARIES} setupapi)

	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")

		if(HIDAPI_LIBUDEV_FOUND AND LIBUSB1_FOUND)
			option(VRPN_HIDAPI_USE_LIBUSB
				"Should the LibUSB implementation of HIDAPI be used? If not, the (less reliable) hidraw version is used." ON)

		elseif(LIBUSB1_FOUND)
			set(VRPN_HIDAPI_USE_LIBUSB YES)

		elseif(HIDAPI_LIBUDEV_FOUND)
			set(VRPN_HIDAPI_USE_LIBUSB NO)

		else()
			message(STATUS "ERROR: Can't use local HIDAPI without either libusb1 or udev!")
			set(HIDAPI_FOUND FALSE)
		endif()

		if(VRPN_HIDAPI_USE_LIBUSB)
			list(APPEND HIDAPI_SOURCES "${PROJECT_SOURCE_DIR}/vrpn_HIDAPI_Linux_Hack.c")
			#list(APPEND HIDAPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/linux/hid-libusb.c")
			set(HIDAPI_LIBRARIES ${LIBUSB1_LIBRARIES})
			list(APPEND HIDAPI_INCLUDE_DIRS ${LIBUSB1_INCLUDE_DIRS})
		else()
			list(APPEND HIDAPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/linux/hid.c")
			set(HIDAPI_LIBRARIES ${HIDAPI_LIBUDEV})
			list(APPEND HIDAPI_INCLUDE_DIRS ${HIDAPI_HIDRAW_INCLUDE_DIR} ${HIDAPI_LIBUDEV_INCLUDE_DIR})
		endif()


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
