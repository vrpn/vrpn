cmake_minimum_required(VERSION 2.8.3)
project(VRPN)

# Changes made by:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Make Quatlib and VRPN into packages as described by Cory (?)
# Move DirectInput libraries out of vrpn_Configure.h and into the Cmake realm
# Repeat for all other configurable headers/libraries - see below for a list
# Move the shared-library code over to CMake's normal definition
# Improve this CPack installer.

###
# Local CMake Modules - keep this first
###
list(APPEND CMAKE_MODULE_PATH ${VRPN_SOURCE_DIR}/cmake)
include(UseBackportedModules)
include(MSVCMultipleProcessCompile)
include(CppcheckTargets)
include(SetDefaultBuildType)
include(OptionRequires)
include(CTest)
include(CreateDashboardScripts)


if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(SUBPROJECT TRUE)
	set(DEFAULT_OFF_IF_SUBPROJECT OFF_BY_DEFAULT)
	set(TRUE_UNLESS_SUBPROJECT FALSE)
else()
	set(SUBPROJECT FALSE)
	set(DEFAULT_OFF_IF_SUBPROJECT)
	set(TRUE_UNLESS_SUBPROJECT TRUE)
endif()

###
# On Windows 7, it does not work to install in the default location,
# which is the Program Files directory, because you have to not only have
# file permission to write there but also "run as administrator."  This
# means that "make install" from a Visual Studio project fails.  To get
# around that, we need to set CMAKE_INSTALL_PREFIX to something other
# than the default.  However, it is a cache variable that has already been
# set.  If we make a local variable, it uses this rather than the cache
# variable and never tells the poor user what happened (the GUI location
# looks standard but the files end up somewhere else).  If we make it a
# non-forced cache variable, it already has a value so does not change.
# If we make it a forced cache variable, it gets overwritten everytime
# and the user cannot change it on the GUI.  So we have a workaround here.
# We make a cache variable that records whether we have ever forced the
# install prefix.  If not, we force it.  If so, we don't force it again.
# This has the effect of setting it the first time cmake is run, showing
# the change in the GUI, and also letting the user change the value in
# the GUI if they don't like what we did.  If I knew how to do this only
# happen on Windows 7, I'd make the if(WIN32) more specific.
if (WIN32 AND NOT SUBPROJECT)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND (NOT ONCE_SET_CMAKE_INSTALL_PREFIX))
		set(ONCE_SET_CMAKE_INSTALL_PREFIX true CACHE INTERNAL
			"Have we set the install prefix yet?" FORCE)
		set(CMAKE_INSTALL_PREFIX C:/usr/local CACHE PATH
		    "Install path prefix, prepended onto install directories"
		    FORCE)
	endif()
endif ()


###
# Basic packaging options and versioning

include("${CMAKE_CURRENT_SOURCE_DIR}/ParseVersion.cmake")

message(STATUS
	"Configuring the VRPN suite version ${CPACK_PACKAGE_VERSION} using the CMake-based build system\n")

set(CPACK_PACKAGE_VENDOR
	"Russell M. Taylor II at the University of North Carolina at Chapel Hill")
set(CPACK_PACKAGE_FILE_NAME
	"${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
	"${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-src")

#-----------------------------------------------------------------------------
# Compiler flags we got from Hans for Windows and from Sheldon Andrews
# for other architectures.
if(MSVC)	# MS-Windows Visual Studio, both 32 and 64 bits
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   /fp:fast")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FIXED:NO")

else()
	# GCC compilers on 64-bit machines require -fPIC for shared libraries or libs
	# linked into shared libraries.

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SHARED_LIBRARY_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_SHARED_LIBRARY_CXX_FLAGS}")
endif()

#-----------------------------------------------------------------------------
# Options that control what gets built and how.

if(NOT SUBPROJECT)
	# We can build two configurations (passing defs to the compile lines) - which ones do we want?
	# Note that we can build both now, if desired!
	option(VRPN_BUILD_CLIENT_LIBRARY
		"Build the vrpn library including only client code"
		ON)
	option(VRPN_BUILD_SERVER_LIBRARY
		"Build the vrpnserver library including client and server code"
		ON)

	# Build various applications if we want them.
	option(VRPN_BUILD_CLIENTS "Build VRPN client apps and tests" ON)
	option(VRPN_BUILD_SERVERS "Build VRPN servers" ON)
	option(VRPN_BUILD_TEST_RPC_GENERATION "Build VRPN RPC generation" OFF)
endif()

# Development tools
if(MSVC)
	option(VRPN_BUILD_WITH_PROJECT_FOLDERS
		"Use project folders in build system - not compatible with Visual C++ Express editions!"
		OFF)
else()
	set(VRPN_BUILD_WITH_PROJECT_FOLDERS ON)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ${VRPN_BUILD_WITH_PROJECT_FOLDERS})

# Set a default build type
set_default_build_type("RelWithDebInfo")

# Force use of our CMake-processed configuration header before the stock one.
include_directories("${PROJECT_BINARY_DIR}")


# Include directory needed by all of the files
include_directories(${VRPN_SOURCE_DIR}
	${VRPN_SOURCE_DIR}/atmellib
	${VRPN_SOURCE_DIR}/quat)


#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
#
# CMake variables:
#   SERVER_EXTRA_LIBS - libraries to link against when building the server lib
#   EXTRA_LIBS - libraries to link against when building any VRPN lib
#
# Note that library linking is, by default, transitive:
# Specify linking here (even though static libraries might not use it
# directly - think of shared libs and your fellow developer) rather than
# in the included apps.

###
# Quatlib
###
add_subdirectory(quat)
list(APPEND EXTRA_LIBS quat)

###
# Threading (not on win32)
###
if(NOT WIN32)
	find_package(Threads REQUIRED)
	list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

###
# Windows-specific (non-Cygwin) dependencies
###
if(WIN32 AND NOT UNIX)
	# Winsock - needed for endianness conversion
	list(APPEND EXTRA_LIBS ws2_32)

	# Windows multimedia - needed for joywin32
	list(APPEND EXTRA_LIBS winmm)
endif()

###
# Optional packages
###
message(STATUS
	"Now searching for auto-configurable optional packages...\n")

###
# Submodules - bundled libraries/sources
###
add_subdirectory(submodules)

###
# SWIG and Python Libs (for python wrapper)
###
find_package(SWIG)
find_package(PythonLibs)
option_requires(VRPN_BUILD_PYTHON
	"Build VRPN Python bindings"
	${DEFAULT_OFF_IF_SUBPROJECT}
	SWIG_FOUND
	PYTHONLIBS_FOUND)

###
# javac, jar, and javah (for java wrapper)
###
find_package(Java COMPONENTS Development)
find_package(JNI)
find_program(JAVAH_EXECUTABLE NAMES javah)
mark_as_advanced(JAVAH_EXECUTABLE)
option_requires(VRPN_BUILD_JAVA
	"Build VRPN Java bindings"
	${DEFAULT_OFF_IF_SUBPROJECT}
	Java_JAVAC_EXECUTABLE
	Java_JAR_EXECUTABLE
	JNI_FOUND
	JAVAH_EXECUTABLE)

###
# MPI
###
find_package(MPI)
# XXX Safe to enable by default if we find it?
option_requires(VRPN_USE_MPI "Build with MPI support" OFF_BY_DEFAULT MPI_FOUND)

if(VRPN_USE_MPI)
	# XXX what else needs to be done here?
	add_definitions(${MPI_COMPILE_FLAGS})
	include_directories(${MPI_INCLUDE_PATH})
	list(APPEND EXTRA_LIBS ${MPI_LIBRARIES})
endif()

###
# Libusb1
###
find_package(Libusb1)
option_requires(VRPN_USE_LIBUSB_1_0
	"Attempt to use LibUSB-1.0 to talk directly to USB devices."
	${DEFAULT_OFF_IF_SUBPROJECT}
	LIBUSB1_FOUND)

if(VRPN_USE_LIBUSB_1_0)
	include_directories(${LIBUSB1_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${LIBUSB1_LIBRARIES})
endif()


###
# HID and HIDAPI
###

# Setting up the local HIDAPI was handled above, in the submodules directory
if(NOT VRPN_USE_LOCAL_HIDAPI)
	find_package(HIDAPI)	
endif()

# HID requires either local or system-installed HIDAPI
# Both set HIDAPI_FOUND, HIDAPI_LIBRARIES, and HIDAPI_INCLUDE_DIRS
# If the user chose VRPN_USE_LOCAL_HIDAPI, the HIDAPI_SOURCES
# variable, as included in the source list below, will also be set.
option_requires(VRPN_USE_HID
	"Build with support for HID devices using HIDAPI"
	${DEFAULT_OFF_IF_SUBPROJECT}
	HIDAPI_FOUND)

if(VRPN_USE_HID)
	include_directories(${HIDAPI_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${HIDAPI_LIBRARIES})
	option(VRPN_BUILD_HID_GUI "Should we build a GUI for analyzing live HID streams?" ${TRUE_UNLESS_SUBPROJECT})
else()
	# Clear this variable if they don't want HID after all.
	message(STATUS "NOTE: You have VRPN_USE_LOCAL_HIDAPI enabled, but "
	"VRPN_USE_HID disabled: HIDAPI will only be built if you enable HID support for VRPN")
	set(HIDAPI_SOURCES)
endif()

###
# OpenHaptics HDAPI/HLAPI
###
find_package(OpenHaptics)
option_requires(VRPN_USE_PHANTOM_SERVER
	"Build with SensAble Phantom support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	OPENHAPTICS_FOUND)

if(VRPN_USE_PHANTOM_SERVER)
	include_directories(${OPENHAPTICS_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${OPENHAPTICS_LIBRARIES})
endif()

###
# WiiUse
###
find_package(WiiUse)
option_requires(VRPN_USE_WIIUSE
		"Build with WiiUse library support (makes servers GPL)"
		${DEFAULT_OFF_IF_SUBPROJECT}
		WIIUSE_FOUND)

if(VRPN_USE_WIIUSE)
	include_directories(${WIIUSE_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${WIIUSE_LIBRARIES})
endif()

###
# JsonCpp
###
find_package(JsonCpp)
option_requires(VRPN_USE_JSONNET
	"Build with JSONCPP (for Android widgets)"
	${DEFAULT_OFF_IF_SUBPROJECT}
	JSONCPP_FOUND)

if(VRPN_USE_JSONNET)
	include_directories(${JSONCPP_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${JSONCPP_LIBRARIES})
	if(MSVC)
		message("\nSee README.jsoncpp for important instructions on how to build JSONCPP and avoid LNK2005 errors \n")
	endif()
endif()




###
# libnifalcon
###
find_package(LibNifalcon)
option_requires(VRPN_USE_LIBNIFALCON
		"Build with libnifalcon support to access Novint Falcon devices"
		${DEFAULT_OFF_IF_SUBPROJECT}
		LIBNIFALCON_FOUND)

if(VRPN_USE_LIBNIFALCON)
	include_directories(${LIBNIFALCON_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${LIBNIFALCON_LIBRARIES})
endif()

###
# DirectShow
###
# Note: FindDirectShow.cmake still needs some TLC to bring it in line
# with CMake standards and make it work in a variety of environments
if(WIN32)
	find_package(DirectShow)
	option_requires(VRPN_USE_DIRECTSHOW
			"Build with Microsoft DirectShow support"
			${DEFAULT_OFF_IF_SUBPROJECT}
			DIRECTSHOW_FOUND)
	option_requires(VRPN_BUILD_DIRECTSHOW_VIDEO_SERVER
			"Enable to build DirectShow Video Server (Windows)"
			${DEFAULT_OFF_IF_SUBPROJECT}
			DIRECTSHOW_FOUND)

	if(VRPN_USE_DIRECTSHOW)
		include_directories(${DirectShow_INCLUDE_DIRS})
		# XXX What else should be done here?
	endif()
endif()

###
# DirectInput
###
if(WIN32)
	find_package(DirectInput)
	option_requires(VRPN_USE_DIRECTINPUT
			"Build with Microsoft DirectInput support"
			${DEFAULT_OFF_IF_SUBPROJECT}
			DIRECTINPUT_FOUND)
	if(VRPN_USE_DIRECTINPUT)
		include_directories(${DIRECTINPUT_INCLUDE_DIRS})
		list(APPEND SERVER_EXTRA_LIBS ${DIRECTINPUT_LIBRARIES})
		find_path(DIRECTINPUT_XINPUT_INCLUDE_DIR xinput.h HINTS ${DIRECTINPUT_INCLUDE_DIRS})
		mark_as_advanced(DIRECTINPUT_XINPUT_INCLUDE_DIR)
		if(DIRECTINPUT_XINPUT_INCLUDE_DIR)
			set(VRPN_USE_WINDOWS_XINPUT TRUE)
		else()
			set(VRPN_USE_WINDOWS_XINPUT FALSE)
		endif()
	endif()
endif()

###
# GPM
###
if(UNIX)
	find_package(GPM)
	option_requires(VRPN_USE_GPM_MOUSE
		"Build with GPM Linux mouse interface support"
		${DEFAULT_OFF_IF_SUBPROJECT}
		GPM_FOUND)
endif()

if(VRPN_USE_GPM_MOUSE)
	list(APPEND SERVER_EXTRA_LIBS ${GPM_LIBRARIES})
endif()

###
# NIDAQMX
###
find_package(NIDAQmx)
option_requires(VRPN_USE_NATIONAL_INSTRUMENTS_MX
	"Build with National Instruments NIDAQMX support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	NIDAQMX_FOUND)

if(NIDAQMX_FOUND)
	include_directories(${NIDAQMX_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${NIDAQMX_LIBRARIES})
endif()

###
# Arrington Research ViewPoint EyeTracker
###
find_package(ViewPoint)
option_requires(VRPN_USE_VIEWPOINT
	"Build with support for ViewPoint EyeTracker"
	${DEFAULT_OFF_IF_SUBPROJECT}
	VIEWPOINT_FOUND)

if(VIEWPOINT_FOUND)
	include_directories(${VIEWPOINT_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${VIEWPOINT_LIBRARIES})

	# Needed for the config file, apparently - it was added to the non-CMake one.
	get_filename_component(VRPN_VIEWPOINT_LIB_PATH "${VIEWPOINT_LIBRARY}" PATH)
	file(TO_CMAKE_PATH "${VRPN_VIEWPOINT_LIB_PATH}" VRPN_VIEWPOINT_LIB_PATH)
endif()

###
# XXX Other libraries needing detection and handling (TODO)
###
# possibly GHOST?
# Adrienne time-code generator
# InterSense Interface Libraries SDK
# National Instruments Nidaq traditional
# US Digital SEI/A2
# microscribe3D library
# PhaseSpace OWL API
# GLI Interactive LLC MotionNode library
# Hillcrest Labs' Freespace
#
# All include paths should be moved out of at least vrpn_Configure.h.cmake_in
# as well as all #pragma comment (lib, "" ) lines, since cmake replaces
# them more flexibly (include_directories and target_link_libraries)

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h")

add_definitions(-DVRPN_USING_CMAKE)

# Configuration options controlling what gets included in the build.
# These are the default options - if a library is detected above it will
# be pre-set to ON instead.
option(VRPN_INCLUDE_TIMECODE_SERVER
	"Build with Adrienne time-code server support"
	OFF)
option(VRPN_INCLUDE_INTERSENSE
	"Build with InterSense native library support"
	OFF)
option(VRPN_USE_NATIONAL_INSTRUMENTS
	"Build with National Instruments (old library) support"
	OFF)
option(VRPN_USE_NIDAQ "Build with NIDAQ support ca. 1999" OFF)
option(VRPN_USE_USDIGITAL
	"Build with US Digital SEI/A2 library support"
	OFF)
option(VRPN_USE_MICROSCRIBE
	"Build with MicroScribe3D library support"
	OFF)
option(VRPN_INCLUDE_PHASESPACE
	"Build with PhaseSpace library support"
	OFF)
option(VRPN_USE_MOTIONNODE
	"Build with GLI Interactive LLC MotionNode library support"
	OFF)
option(VRPN_USE_FREESPACE
	"Build with Hillcrest Labs' Freespace devices support"
	OFF)
option(VRPN_USE_TRIVISIOCOLIBRI
	"Build with support for TrivisioColibri tracker"
	OFF)
option(VRPN_USE_JSONNET
	"Build with support JSON over UDP (for Android widgets)"
	 OFF)
option(VRPN_BUILD_EXTRA_COMPILER_WARNINGS
	"Build with flags to enable extra warnings."
	OFF)

if(WIN32)
	option(VRPN_USE_DIRECTINPUT
		"Build with Microsoft DirectInput support"
		OFF)
	option(VRPN_USE_SHARED_LIBRARY
		"Enable to use DLLs on Windows (see vrpn_Configure.h for more info)"
		OFF)
elseif(UNIX)
	option(VRPN_BUILD_PROFILING_SUPPORT
		"Build with flags to enable profiling."
		OFF)
endif()

if(VRPN_BUILD_EXTRA_COMPILER_WARNINGS)
	include(EnableExtraCompilerWarnings)
	globally_enable_extra_compiler_warnings()
endif()

if(VRPN_BUILD_PROFILING_SUPPORT)
	include(EnableProfiling)
	globally_enable_profiling()
endif()

if(SUBPROJECT)
	set(BUILD_TESTING FALSE)
endif()


#-----------------------------------------------------------------------------
# Build the library itself and declare what bits need to be installed

set(VRPN_CLIENT_SOURCES
	vrpn_Analog.C
	vrpn_Analog_Output.C
	vrpn_Auxiliary_Logger.C
	vrpn_BaseClass.C
	vrpn_Button.C
	vrpn_Connection.C
	vrpn_Dial.C
	vrpn_FileConnection.C
	vrpn_FileController.C
	vrpn_ForceDevice.C
	vrpn_Forwarder.C
	vrpn_ForwarderController.C
	vrpn_Imager.C
	vrpn_LamportClock.C
	vrpn_Mutex.C
	vrpn_Poser.C
	vrpn_RedundantTransmission.C
	vrpn_Serial.C
	vrpn_Shared.C
	vrpn_SharedObject.C
	vrpn_Sound.C
	vrpn_Text.C
	vrpn_Tracker.C)

set(VRPN_CLIENT_PUBLIC_HEADERS
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h"
	vrpn_Analog.h
	vrpn_Analog_Output.h
	vrpn_Auxiliary_Logger.h
	vrpn_BaseClass.h
	vrpn_BufferUtils.h
	vrpn_Button.h
	vrpn_Connection.h
	vrpn_Dial.h
	vrpn_FileConnection.h
	vrpn_FileController.h
	vrpn_ForceDevice.h
	vrpn_ForwarderController.h
	vrpn_Forwarder.h
	vrpn_Imager.h
	vrpn_LamportClock.h
	vrpn_Log.h
	vrpn_MainloopContainer.h
	vrpn_MainloopObject.h
	vrpn_Mutex.h
	vrpn_SendTextMessageStreamProxy.h
	vrpn_Serial.h
	vrpn_Shared.h
	vrpn_SharedObject.h
	vrpn_Sound.h
	vrpn_Text.h
	vrpn_Tracker.h
	vrpn_Types.h)

set(VRPN_SERVER_SOURCES
	${VRPN_CLIENT_SOURCES}
	vrpn_3DConnexion.C
	vrpn_3DMicroscribe.C
	vrpn_3Space.C
	vrpn_5DT16.C
	vrpn_ADBox.C
	vrpn_Analog_5dt.C
	vrpn_Analog_5dtUSB.C
	vrpn_Analog_Radamec_SPI.C
	vrpn_Analog_USDigital_A2.C
	vrpn_Atmel.C
	vrpn_Button_NI_DIO24.C
	vrpn_Button_USB.cpp
	vrpn_CerealBox.C
	vrpn_DirectXFFJoystick.C
	vrpn_DirectXRumblePad.C
	vrpn_DreamCheeky.C
	vrpn_Dyna.C
	vrpn_Event_Analog.C
	vrpn_Event.C
	vrpn_Event_Mouse.C
	vrpn_Flock.C
	vrpn_Flock_Parallel.C
	vrpn_ForceDeviceServer.C
	vrpn_Freespace.C
	vrpn_FunctionGenerator.C
	vrpn_GlobalHapticsOrb.C
	vrpn_HumanInterface.C
	vrpn_Imager_Stream_Buffer.C
	vrpn_ImmersionBox.C
	vrpn_inertiamouse.C
	vrpn_JoyFly.C
	vrpn_Joylin.C
	vrpn_Joywin32.C
	vrpn_Keyboard.C
	vrpn_LUDL.C
	vrpn_Magellan.C
	vrpn_Mouse.C
	vrpn_NationalInstruments.C
	vrpn_Nidaq.C
	vrpn_nikon_controls.C
	vrpn_Poser_Analog.C
	vrpn_Poser_Tek4662.C
	vrpn_raw_sgibox.C
	vrpn_sgibox.C
	vrpn_Spaceball.C
	vrpn_Tng3.C
	vrpn_Tracker_3DMouse.C
	vrpn_Tracker_AnalogFly.C
	vrpn_Tracker_ButtonFly.C
	vrpn_Tracker_Crossbow.C
	vrpn_Tracker_DTrack.C
	vrpn_Tracker_Fastrak.C
	vrpn_Tracker_GameTrak.C
	vrpn_Tracker_GPS.C
	vrpn_Tracker_isense.C
	vrpn_Tracker_Isotrak.C
	vrpn_Tracker_JsonNet.C
	vrpn_Tracker_Liberty.C
	vrpn_Tracker_MotionNode.C
	vrpn_Tracker_NDI_Polaris.C
	vrpn_Tracker_NovintFalcon.C
	vrpn_Tracker_PhaseSpace.C
	vrpn_Tracker_RazerHydra.C
	vrpn_Tracker_SpacePoint.C
	vrpn_Tracker_TrivisioColibri.C
	vrpn_Tracker_WiimoteHead.C
	vrpn_Tracker_zSight.C
	vrpn_Tracker_ViewPoint.C
	vrpn_UNC_Joystick.C
	vrpn_VPJoystick.C
	vrpn_Wanda.C
	vrpn_WiiMote.C
	vrpn_XInputGamepad.C
	vrpn_Xkeys.C
	vrpn_Zaber.C
	server_src/vrpn_Generic_server_object.C
	${HIDAPI_SOURCES})

set(VRPN_SERVER_PUBLIC_HEADERS
	${VRPN_CLIENT_PUBLIC_HEADERS}
	vrpn_3DConnexion.h
	vrpn_3DMicroscribe.h
	vrpn_3Space.h
	vrpn_5DT16.h
	vrpn_ADBox.h
	vrpn_Analog_5dt.h
	vrpn_Analog_5dtUSB.h
	vrpn_Analog_Radamec_SPI.h
	vrpn_Analog_USDigital_A2.h
	vrpn_Atmel.h
	vrpn_Button_NI_DIO24.h
	vrpn_Button_USB.h
	vrpn_CerealBox.h
	vrpn_DirectXFFJoystick.h
	vrpn_DirectXRumblePad.h
	vrpn_DreamCheeky.h
	vrpn_Dyna.h
	vrpn_Event_Analog.h
	vrpn_Event.h
	vrpn_Event_Mouse.h
	vrpn_Flock.h
	vrpn_Flock_Parallel.h
	vrpn_ForceDeviceServer.h
	vrpn_Freespace.h
	vrpn_FunctionGenerator.h
	vrpn_GlobalHapticsOrb.h
	vrpn_HashST.h
	vrpn_HumanInterface.h
	vrpn_Imager_Stream_Buffer.h
	vrpn_ImmersionBox.h
	vrpn_inertiamouse.h
	vrpn_JoyFly.h
	vrpn_Joylin.h
	vrpn_Joywin32.h
	vrpn_Keyboard.h
	vrpn_LUDL.h
	vrpn_Magellan.h
	vrpn_Mouse.h
	vrpn_NationalInstruments.h
	vrpn_Nidaq.h
	vrpn_nikon_controls.h
	vrpn_Poser_Analog.h
	vrpn_Poser.h
	vrpn_Poser_Tek4662.h
	vrpn_raw_sgibox.h
	vrpn_RedundantTransmission.h
	vrpn_sgibox.h
	vrpn_Spaceball.h
	vrpn_Tng3.h
	vrpn_Tracker_3DMouse.h
	vrpn_Tracker_AnalogFly.h
	vrpn_Tracker_ButtonFly.h
	vrpn_Tracker_Crossbow.h
	vrpn_Tracker_DTrack.h
	vrpn_Tracker_Fastrak.h
	vrpn_Tracker_GameTrak.h
	vrpn_Tracker_GPS.h
	vrpn_Tracker_isense.h
	vrpn_Tracker_Isotrak.h
	vrpn_Tracker_JsonNet.h
	vrpn_Tracker_Liberty.h
	vrpn_Tracker_MotionNode.h
	vrpn_Tracker_NDI_Polaris.h
	vrpn_Tracker_NovintFalcon.h
	vrpn_Tracker_PhaseSpace.h
	vrpn_Tracker_RazerHydra.h
	vrpn_Tracker_SpacePoint.h
	vrpn_Tracker_TrivisioColibri.h
	vrpn_Tracker_WiimoteHead.h
	vrpn_Tracker_zSight.h
	vrpn_Tracker_ViewPoint.h
	vrpn_UNC_Joystick.h
	vrpn_VPJoystick.h
	vrpn_Wanda.h
	vrpn_WiiMote.h
	vrpn_XInputGamepad.h
	vrpn_Xkeys.h
	vrpn_Zaber.h
	server_src/vrpn_Generic_server_object.h)

if(VRPN_BUILD_SERVER_LIBRARY)

	add_library(vrpnserver
		${VRPN_SERVER_SOURCES}
		${VRPN_SERVER_PUBLIC_HEADERS})
	target_link_libraries(vrpnserver ${EXTRA_LIBS} ${SERVER_EXTRA_LIBS})
	set_property(TARGET
		vrpnserver
		PROPERTY
		PUBLIC_HEADER
		${VRPN_SERVER_PUBLIC_HEADERS})
	set_property(TARGET
		vrpnserver
		PROPERTY
		PROJECT_LABEL
		"Core VRPN Server Library")
	set_property(TARGET
		vrpnserver
		PROPERTY
		FOLDER
		"Library")

	if(UNIX)
		add_subdirectory(atmellib)
		target_link_libraries(vrpnserver vrpn_atmel)
	endif()

	add_subdirectory(gpsnmealib)
	target_link_libraries(vrpnserver gpsnmea)

	install(TARGETS
		vrpnserver
		ARCHIVE
		DESTINATION
		lib
		COMPONENT serversdk

		PUBLIC_HEADER
		DESTINATION
		include
		COMPONENT serversdk)

	add_cppcheck(vrpnserver STYLE UNUSED_FUNCTIONS)
endif()


if(VRPN_BUILD_CLIENT_LIBRARY)
	add_library(vrpn ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	target_link_libraries(vrpn ${EXTRA_LIBS})

	set_property(TARGET
		vrpn
		PROPERTY
		PUBLIC_HEADER
		${VRPN_CLIENT_PUBLIC_HEADERS})

	set_property(TARGET
		vrpn
		PROPERTY
		COMPILE_DEFINITIONS
		"VRPN_CLIENT_ONLY")
	set_property(TARGET
		vrpn
		PROPERTY
		PROJECT_LABEL
		"Core VRPN Client Library")
	set_property(TARGET
		vrpn
		PROPERTY
		FOLDER
		"Library")

	install(TARGETS
		vrpn
		ARCHIVE
		DESTINATION
		lib
		COMPONENT clientsdk

		PUBLIC_HEADER
		DESTINATION
		include
		COMPONENT clientsdk)

	add_cppcheck(vrpn STYLE UNUSED_FUNCTIONS)
endif()

add_subdirectory(client_src)

#-----------------------------------------------------------------------------
# Build the server applications if we've been asked to and we didn't build
# the library client-only.

if(VRPN_BUILD_SERVERS AND VRPN_BUILD_SERVER_LIBRARY)
	add_subdirectory(server_src)
endif()

#-----------------------------------------------------------------------------
# Build the RPC generation if we've been asked to

if(VRPN_BUILD_TEST_RPC_GENERATION)
	add_subdirectory(util/gen_rpc)
endif()

#-----------------------------------------------------------------------------
# Create documentation
if(NOT SUBPROJECT)
	add_subdirectory(doxygen)
endif()

#-----------------------------------------------------------------------------
# Python wrappers
add_subdirectory(python_vrpn)

#-----------------------------------------------------------------------------
# Java wrappers
add_subdirectory(java_vrpn)

#-----------------------------------------------------------------------------
# HID Gui
if(VRPN_BUILD_HID_GUI)
	add_subdirectory(hid_gui)
endif()

#-----------------------------------------------------------------------------
# Testing applications that live in the main directory.
if(VRPN_BUILD_SERVERS AND VRPN_BUILD_SERVER_LIBRARY AND BUILD_TESTING)
	foreach(SOURCE time_test.cpp)
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} vrpnserver)
		set_target_properties(${APP} PROPERTIES FOLDER Tests)
		install(TARGETS ${APP} RUNTIME DESTINATION bin COMPONENT tests)
	endforeach()
endif()

#-----------------------------------------------------------------------------
# Do a little check for GPL and GPL-incompatible libraries

# What flags cause us to link against GPL libraries?
set(ALL_GPL_SERVER_FLAGS
		VRPN_USE_WIIUSE)

set(GPL_SERVER_FLAGS)
foreach(POSSIBLE_GPL_FLAG ${ALL_GPL_SERVER_FLAGS})
	if(${POSSIBLE_GPL_FLAG})
		list(APPEND GPL_SERVER_FLAGS "${POSSIBLE_GPL_FLAG}")
	endif()
endforeach()

# What flags cause us to link against GPL-incompatible libraries?
set(ALL_GPLINCOMPAT_SERVER_FLAGS
		VRPN_USE_PHANTOM_SERVER)

set(GPLINCOMPAT_SERVER_FLAGS)
foreach(POSSIBLE_GPLINCOMPAT_FLAG ${ALL_GPLINCOMPAT_SERVER_FLAGS})
	if(${POSSIBLE_GPLINCOMPAT_FLAG})
		list(APPEND GPLINCOMPAT_SERVER_FLAGS "${POSSIBLE_GPLINCOMPAT_FLAG}")
	endif()
endforeach()


if(GPL_SERVER_FLAGS)
	# Some GPL options are enabled
	message(STATUS "")
	message(STATUS
		"NOTE: The following build options may produce a GPL-licensed server library/binary.")
	message(STATUS "  ${GPL_SERVER_FLAGS}")
	message(STATUS
		"NOTE: I am not a lawyer, and this is not legal advice!")

	option(VRPN_GPL_SERVER
		"Check this to accept the possibility of linking GPL libraries with the server."
		OFF)
endif()

if(GPLINCOMPAT_SERVER_FLAGS)
	# Some GPL-incompatible options are enabled
	message(STATUS "")
	message(STATUS
		"NOTE: The following build options may produce a server library/binary")
	message(STATUS
		"  that is incompatible with the GPL/undistributable if linked with GPL libraries.")
	message(STATUS "  ${GPLINCOMPAT_SERVER_FLAGS}")
	message(STATUS
		"NOTE: I am not a lawyer, and this is not legal advice!")
endif()

# Check for errors.
if(VRPN_BUILD_SERVER_LIBRARY)
	if(GPL_SERVER_FLAGS AND NOT VRPN_GPL_SERVER)
		message(STATUS "")
		message(STATUS "Selected build options produce a GPL server library.")
		message(STATUS
			"You may disable them, otherwise set VRPN_GPL_SERVER to acknowledge this and build anyway.")
		message(FATAL_ERROR
			"Need VRPN_GPL_SERVER to build server library with GPL options enabled!")
	endif()

	if(GPL_SERVER_FLAGS AND GPLINCOMPAT_SERVER_FLAGS)
		message(STATUS "")
		message(STATUS "IMPORTANT LICENSING NOTE!")
		message(STATUS
			"Building with the current settings may produce a legally non-distributable server binary!")
		message(STATUS
			"NOTE: I am not a lawyer, and this is not legal advice!")
	endif()
endif()

#-----------------------------------------------------------------------------
# Enable testing/dashboards
create_dashboard_scripts("${CMAKE_CURRENT_SOURCE_DIR}/DashboardBuildInitialCache.cmake.in")

#-----------------------------------------------------------------------------
# If we succeeded, we can go on and include packaging!
include(CPack)
cpack_add_component(serversdk
	DISPLAY_NAME "VRPN Server Library and C++ Headers")
cpack_add_component(clientsdk
	DISPLAY_NAME "VRPN Client Library and C++ Headers")
cpack_add_component(tests
	DISPLAY_NAME "Test applications")
cpack_add_component(clients
	DISPLAY_NAME "Client applications")
cpack_add_component(servers
	DISPLAY_NAME "Server applications")
cpack_add_component(mainserver
	DISPLAY_NAME "VRPN main server application")
cpack_add_component(python
	DISPLAY_NAME "Python bindings")
cpack_add_component(java
	DISPLAY_NAME "Java bindings")
cpack_add_component(doc
	DISPLAY_NAME "C++ API Documentation")
