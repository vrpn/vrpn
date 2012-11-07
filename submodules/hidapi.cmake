###
# HID and HIDAPI
###

set(HIDAPI_BACKEND_FOUND NO)
# Local HIDAPI requirements
if(WIN32)
#	find_package(WinHID)
	set(HIDAPI_BACKEND_FOUND YES)

elseif(APPLE)
	find_package(MacHID)
	if(MACHID_FOUND)
		set(HIDAPI_BACKEND_FOUND YES)
	endif()

else()
	find_package(Libusb1)
	if(LIBUSB1_FOUND)
		set(HIDAPI_BACKEND_FOUND YES)
	endif()

	set(HIDAPI_LIBUDEV_FOUND NO)
	if(CMAKE_SYSTEM_NAME MATCHES "Linux")
		find_library(HIDAPI_LIBUDEV udev)
		find_path(HIDAPI_HIDRAW_INCLUDE_DIR linux/hidraw.h)
		find_path(HIDAPI_LIBUDEV_INCLUDE_DIR libudev.h)
		if(HIDAPI_LIBUDEV AND HIDAPI_HIDRAW_INCLUDE_DIR AND HIDAPI_LIBUDEV_INCLUDE_DIR)
			mark_as_advanced(HIDAPI_LIBUDEV
				HIDAPI_HIDRAW_INCLUDE_DIR
				HIDAPI_LIBUDEV_INCLUDE_DIR)
			set(HIDAPI_LIBUDEV_FOUND YES)
			set(HIDAPI_BACKEND_FOUND YES)
		endif()
	endif()

endif()

if(EXISTS "${VRPN_SOURCE_DIR}/submodules/hidapi/hidapi/hidapi.h")
	set(LOCAL_HIDAPI_SUBMODULE_RETRIEVED TRUE)
else()
	message(STATUS
		"Local HIDAPI submodule not found. To download with Git, run git submodule update --init")
endif()

option_requires(VRPN_USE_LOCAL_HIDAPI
	"Build with HIDAPI code from within VRPN source directory"
	LOCAL_HIDAPI_SUBMODULE_RETRIEVED
	HIDAPI_BACKEND_FOUND)

if(VRPN_USE_LOCAL_HIDAPI)
	set(HIDAPI_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/hidapi")
	set(HIDAPI_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/hidapi/hidapi.h")
	set(HIDAPI_FOUND TRUE)

	set(VRPN_HIDAPI_USE_LINUXUDEV NO)
	# Permit choice between backends, when possible.
	if(HIDAPI_LIBUDEV_FOUND AND LIBUSB1_FOUND)
		option(VRPN_HIDAPI_USE_LIBUSB
			"Should the LibUSB implementation of HIDAPI be used? If not, the (less reliable) hidraw version is used."
			ON)
		if(NOT VRPN_HIDAPI_USE_LIBUSB)
			set(VRPN_HIDAPI_USE_LINUXUDEV YES)
		endif()
	elseif(LIBUSB1_FOUND)
		set(VRPN_HIDAPI_USE_LIBUSB YES)

	elseif(HIDAPI_LIBUDEV_FOUND)
		set(VRPN_HIDAPI_USE_LIBUSB NO)
		set(VRPN_HIDAPI_USE_LINUXUDEV YES)
	endif()

	# Set up desired backends
	if(APPLE)
		list(APPEND
			HIDAPI_SOURCES
			"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/mac/hid.c")
		set(HIDAPI_LIBRARIES ${MACHID_LIBRARIES})

	elseif(WIN32)
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hidapi/windows/hid.cpp)
			# Old hidapi
			list(APPEND
				HIDAPI_SOURCES
				"${PROJECT_SOURCE_DIR}/vrpn_Local_HIDAPI.C")
		elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/windows/hid.c")
			# New hidapi
			list(APPEND
				HIDAPI_SOURCES
				"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/windows/hid.c")
		else()
			message(STATUS
				"ERROR: Can't use local HIDAPI - can't find the source file!  Perhaps an unknown upstream version?")
			set(HIDAPI_FOUND FALSE)
		endif()
		set(HIDAPI_LIBRARIES ${WINHID_LIBRARIES} setupapi)

	elseif(VRPN_HIDAPI_USE_LIBUSB)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/libusb/hid.c")
			# Newest version - FreeBSD-compatible libusb backend
			list(APPEND
				HIDAPI_SOURCES
				"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/libusb/hid.c")
		elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi/linux/hid-libusb.c")
			# Earlier version - before FreeBSD support
			list(APPEND
				HIDAPI_SOURCES
				"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/linux/hid-libusb.c")
		else()
			message(STATUS
				"ERROR: Can't use local HIDAPI - can't find the source file!  Perhaps an unknown upstream version?")
			set(HIDAPI_FOUND FALSE)
		endif()
		set(HIDAPI_LIBRARIES ${LIBUSB1_LIBRARIES})
		list(APPEND HIDAPI_INCLUDE_DIRS ${LIBUSB1_INCLUDE_DIRS})

	elseif(VRPN_HIDAPI_USE_LINUXUDEV)
		list(APPEND
			HIDAPI_SOURCES
			"${CMAKE_CURRENT_SOURCE_DIR}/hidapi/linux/hid.c")
		set(HIDAPI_LIBRARIES ${HIDAPI_LIBUDEV})
		list(APPEND
			HIDAPI_INCLUDE_DIRS
			${HIDAPI_HIDRAW_INCLUDE_DIR}
			${HIDAPI_LIBUDEV_INCLUDE_DIR})

	else()
		message(STATUS
			"ERROR: Can't use local HIDAPI without either libusb1 or udev!")
		set(HIDAPI_FOUND FALSE)

	endif()
	if(VRPN_HIDAPI_USE_LIBUSB OR VRPN_HIDAPI_USE_LINUXUDEV)
		find_library(HIDAPI_LIBRT rt)
		if(HIDAPI_LIBRT)
			mark_as_advanced(HIDAPI_LIBRT)
			list(APPEND HIDAPI_LIBRARIES ${HIDAPI_LIBRT})
		endif()
	endif()

	source_group("HIDAPI Submodule" FILES ${HIDAPI_SOURCES})
endif()
