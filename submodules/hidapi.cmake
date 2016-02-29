###
# HID and HIDAPI
###

set(LOCAL_HIDAPI_SUBMODULE_RETRIEVED FALSE)

set(HIDAPI_BACKEND_FOUND NO)
# Local HIDAPI requirements
if(ANDROID)
	find_package(Libusb1)

	# Check to see if our submodule is new enough.
	if(EXISTS "${VRPN_SOURCE_DIR}/submodules/hidapi/android/jni/Android.mk")
		set(VRPN_HIDAPI_SOURCE_ROOT "${VRPN_SOURCE_DIR}/submodules/hidapi")
	endif()

	if(NOT LIBUSB1_FOUND)
		message(STATUS "Android 'Local' HIDAPI: Requires Libusb1.0, not found")
	elseif(NOT VRPN_HIDAPI_SOURCE_ROOT OR NOT EXISTS "${VRPN_HIDAPI_SOURCE_ROOT}")
		message(STATUS "Android 'Local' HIDAPI: Requires a separate, recent HIDAPI source tree, none specified in VRPN_HIDAPI_SOURCE_ROOT")
	elseif(NOT EXISTS "${VRPN_HIDAPI_SOURCE_ROOT}/android/jni/Android.mk")
		message(STATUS "Android 'Local' HIDAPI: Requires a separate, recent HIDAPI source tree (with an android directory), but VRPN_HIDAPI_SOURCE_ROOT doesn't have that")
	else()
		set(HIDAPI_BACKEND_FOUND YES)
		set(LOCAL_HIDAPI_SUBMODULE_RETRIEVED TRUE) # close enough - we have the source, though it's not necessarily a local submodule.
	endif()

elseif(WIN32)
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
endif()

if(NOT LOCAL_HIDAPI_SUBMODULE_RETRIEVED AND NOT SUBPROJECT)
	message(STATUS
		"Local HIDAPI submodule not found. To download with Git, run git submodule update --init")
endif()

option_requires(VRPN_USE_LOCAL_HIDAPI
	"Build with HIDAPI code from within VRPN source directory"
	LOCAL_HIDAPI_SUBMODULE_RETRIEVED
	HIDAPI_BACKEND_FOUND)

if(VRPN_USE_LOCAL_HIDAPI)
	if(ANDROID)
		set(HIDAPI_SOURCE_DIR "${VRPN_HIDAPI_SOURCE_ROOT}")
	else()
		set(HIDAPI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hidapi")
	endif()

	set(HIDAPI_INCLUDE_DIRS "${HIDAPI_SOURCE_DIR}/hidapi")
	set(HIDAPI_SOURCES "${HIDAPI_SOURCE_DIR}/hidapi/hidapi.h")
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
			"${HIDAPI_SOURCE_DIR}/mac/hid.c")
		set(HIDAPI_LIBRARIES ${MACHID_LIBRARIES})

	elseif(WIN32)
		if(EXISTS "${HIDAPI_SOURCE_DIR}/windows/hid.cpp")
			# Old hidapi
			message(STATUS "Warning: Outdated HIDAPI submodule in VRPN! git submodule update submodules/hidapi in the VRPN source directory is recommended!")
			list(APPEND
				HIDAPI_SOURCES
				"${HIDAPI_SOURCE_DIR}/windows/hid.cpp")
		elseif(EXISTS "${HIDAPI_SOURCE_DIR}/windows/hid.c")
			# New hidapi
			list(APPEND
				HIDAPI_SOURCES
				"${HIDAPI_SOURCE_DIR}/windows/hid.c")
		else()
			message(STATUS
				"ERROR: Can't use local HIDAPI - can't find the source file!  Perhaps an unknown upstream version?")
			set(HIDAPI_FOUND FALSE)
		endif()
		set(HIDAPI_LIBRARIES setupapi)

	elseif(VRPN_HIDAPI_USE_LIBUSB)
		if(EXISTS "${HIDAPI_SOURCE_DIR}/libusb/hid.c")
			# Newest version - FreeBSD-compatible libusb backend
			list(APPEND
				HIDAPI_SOURCES
				"${HIDAPI_SOURCE_DIR}/libusb/hid.c")
		elseif(EXISTS "${HIDAPI_SOURCE_DIR}/linux/hid-libusb.c")
			# Earlier version - before FreeBSD support
			message(STATUS "Warning: Outdated HIDAPI submodule in VRPN! git submodule update submodules/hidapi in the VRPN source directory is recommended!")
			list(APPEND
				HIDAPI_SOURCES
				"${HIDAPI_SOURCE_DIR}/linux/hid-libusb.c")
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
			"${HIDAPI_SOURCE_DIR}/linux/hid.c")
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
