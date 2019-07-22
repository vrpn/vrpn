cmake_minimum_required(VERSION 2.8.3)
project(VRPN)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
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
include(CheckIncludeFileCXX)

if((NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR) OR VRPN_SUBPROJECT_BUILD)
	# If you're using this as a subproject and want things installed, set VRPN_INSTALL TRUE.
	set(SUBPROJECT TRUE)
	set(DEFAULT_OFF_IF_SUBPROJECT OFF_BY_DEFAULT)
	set(TRUE_UNLESS_SUBPROJECT FALSE)
	set(QUIET_IF_SUBPROJECT QUIET) # silently search for optional deps as a subproject
	set(BUILD_TESTING FALSE) # don't build testing or create dashboard scripts if a subproject
else()
	set(VRPN_INSTALL TRUE)
	set(SUBPROJECT FALSE)
	set(DEFAULT_OFF_IF_SUBPROJECT)
	set(TRUE_UNLESS_SUBPROJECT TRUE)
	set(QUIET_IF_SUBPROJECT)
	include(CreateDashboardScripts)
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
# If we make it a forced cache variable, it gets overwritten every time
# and the user cannot change it on the GUI.  So we have a workaround here.
# We make a cache variable that records whether we have ever forced the
# install prefix.  If not, we force it.  If so, we don't force it again.
# This has the effect of setting it the first time cmake is run, showing
# the change in the GUI, and also letting the user change the value in
# the GUI if they don't like what we did.  If I knew how to do this only
# happen on Windows 7, I'd make the if(WIN32) more specific.
if(WIN32 AND NOT SUBPROJECT)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
		AND
		(NOT
		ONCE_SET_CMAKE_INSTALL_PREFIX))
		set(ONCE_SET_CMAKE_INSTALL_PREFIX
			true
			CACHE
			INTERNAL
			"Have we set the install prefix yet?"
			FORCE)
		set(CMAKE_INSTALL_PREFIX
			C:/usr/local
			CACHE
			PATH
			"Install path prefix, prepended onto install directories"
			FORCE)
	endif()
endif()


###
# Basic packaging options and versioning

if (VRPN_INSTALL)
	include("${CMAKE_CURRENT_SOURCE_DIR}/ParseVersion.cmake")

	message(STATUS
		"Configuring the VRPN suite version ${CPACK_PACKAGE_VERSION} using the CMake-based build system\n")

	set(CPACK_PACKAGE_VENDOR
		"VRPN Community led by Sensics, Inc.")
	set(CPACK_PACKAGE_FILE_NAME
		"${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
	set(CPACK_SOURCE_PACKAGE_FILE_NAME
		"${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-src")
	set(CPACK_PACKAGE_CONTACT "Sensics VRPN Contact <vrpn@sensics.com>")
endif()

if (NOT SUBPROJECT)
	#-----------------------------------------------------------------------------
	# Compiler flags we got from Hans for Windows and from Sheldon Andrews
	# for other architectures.
	if(MSVC)	# MS-Windows Visual Studio, both 32 and 64 bits
		set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
		if(MSVC_VERSION GREATER 1310) # This compiler flag needs newer than VS.NET 2003 (7.1)
			# Choose fast, possibly less accurate floating point
			# See http://msdn.microsoft.com/en-us/library/e7s85ffb(v=vs.80).aspx
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
		endif()

		# Do not assume fixed base address (probably for DLL integration?)
		# http://msdn.microsoft.com/en-us/library/w368ysh2(v=vs.80).aspx
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FIXED:NO")
	else()
		# GCC compilers on 64-bit machines require -fPIC for shared libraries or libs
		# linked into shared libraries.

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SHARED_LIBRARY_C_FLAGS}")
		set(CMAKE_CXX_FLAGS
			"${CMAKE_CXX_FLAGS} ${CMAKE_SHARED_LIBRARY_CXX_FLAGS}")
	endif()
endif()

set(SERVER_EXTRA_LIBS)
set(SERVER_LINK_FLAGS)

# Set up correct defines for Windows header compilation:
# This theoretically sets the lower-bound on operating system compatibility
# http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx
# NT4	0x0400
# Win2k	0x0500
# WinXP	0x0501
# WS2003SP1/WinXPSP2 	0x0502
# Vista 0x0600
# Win7  0x0601
# Win8  0x0602
# Win8.1 0x0603 (though some docs say 0x0602 with NTDDI_VERSION 0x06030000)
if(WIN32)
	set(WIN_MIN_VER 0x0500) # Default to Windows 2000
	if(MSVC AND MSVC_VERSION GREATER 1699)
		# VS2012: defaults to vista+, update 1 added XP support in additional toolset.
		if("${CMAKE_VS_PLATFORM_TOOLSET}" MATCHES "_xp")
			set(WIN_MIN_VER 0x0501) # WinXP
		else()
			set(WIN_MIN_VER 0x0600) # Vista
		endif()
	endif()
	add_definitions("-D_WIN32_WINNT=${WIN_MIN_VER}" "-DNTDDI_VERSION=${WIN_MIN_VER}0000")
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

	# Development tools
	if(MSVC_IDE AND MSVC_VERSION LESS 1600)
		# VS 2012 Express and newer has folder support...
		option(VRPN_BUILD_WITH_PROJECT_FOLDERS
			"Use project folders in build system - not compatible with Visual C++ Express editions!"
			OFF)
	else()
		set(VRPN_BUILD_WITH_PROJECT_FOLDERS ON)
	endif()

	set_property(GLOBAL
		PROPERTY
		USE_FOLDERS
		${VRPN_BUILD_WITH_PROJECT_FOLDERS})

	# Set a default build type
	set_default_build_type("RelWithDebInfo")

endif()

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
	find_package(Threads REQUIRED ${QUIET_IF_SUBPROJECT})
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
if(NOT SUBPROJECT)
	message(STATUS
		"Now searching for auto-configurable optional packages...\n")
endif()

###
# Submodules - bundled libraries/sources
###
add_subdirectory(submodules)

###
# SWIG and Python Libs (for python wrappers)
###
find_package(SWIG ${QUIET_IF_SUBPROJECT})

set(CurrentPythonSettings
	"${VRPN_BUILD_PYTHON}${VRPN_BUILD_PYTHON_HANDCODED_2X}${VRPN_BUILD_PYTHON_HANDCODED_3X}")
if(NOT CurrentPythonSettings STREQUAL _VRPN_PYTHON_SETTINGS)
	set(vrpn_python_versionsearch)
	set(vrpn_python_versionsearchargs)
	if(VRPN_BUILD_PYTHON)
		# Swig wants 2.x, I assume
		set(vrpn_python_versionsearch 2)
		set(Python_ADDITIONAL_VERSIONS "")
		if(VRPN_BUILD_PYTHON_HANDCODED_3X)
			message(FATAL_ERROR
				"Can't build both SWIG (2.x) and hand-coded 3.x Python bindings. Disable either VRPN_BUILD_PYTHON or VRPN_BUILD_PYTHON_HANDCODED_3X.")
		endif()
	elseif(VRPN_BUILD_PYTHON_HANDCODED_3X
		AND
		VRPN_BUILD_PYTHON_HANDCODED_2X)
		message(FATAL_ERROR
			"Can't build handcoded Python bindings for both 2.x and 3.x versions. Pick one, please.")
	elseif(VRPN_BUILD_PYTHON_HANDCODED_2X
		AND
		NOT
		VRPN_BUILD_PYTHON_HANDCODED_3X)
		set(vrpn_python_versionsearch 2)
		set(Python_ADDITIONAL_VERSIONS "")
	elseif(VRPN_BUILD_PYTHON_HANDCODED_3X
		AND
		NOT
		VRPN_BUILD_PYTHON_HANDCODED_2X)
		set(vrpn_python_versionsearch 3)
		set(Python_ADDITIONAL_VERSIONS 3.4)
	endif()
	unset(PYTHON_INCLUDE_DIR CACHE)
	unset(PYTHON_INCLUDE_DIR)
	unset(PYTHON_LIBRARY CACHE)
	unset(PYTHON_LIBRARY)

	set(_VRPN_PYTHON_VERSIONSEARCH
		${vrpn_python_versionsearch}
		CACHE
		INTERNAL
		""
		FORCE)
	set(_VRPN_PYTHON_SETTINGS
		${CurrentPythonSettings}
		CACHE
		INTERNAL
		""
		FORCE)
endif()

# Passing just the major version works with as desired multi-python-capable find module such
# as in latest CMake (2.8.9)
find_package(PythonLibs ${_VRPN_PYTHON_VERSIONSEARCH} ${QUIET_IF_SUBPROJECT})
if(PYTHONLIBS_FOUND)
	if(PYTHONLIBS_VERSION_STRING)
		string(SUBSTRING
			${PYTHONLIBS_VERSION_STRING}
			0
			1
			vrpn_python_majorver)
		set(PYTHON${vrpn_python_majorver}_FOUND ON)
	elseif(PYTHON_LIBRARY MATCHES "python([23])")
		set(PYTHON${CMAKE_MATCH_1}_FOUND ON)
	elseif(_VRPN_PYTHON_VERSIONSEARCH)
		set(PYTHON${_VRPN_PYTHON_VERSIONSEARCH}_FOUND ON)
	else()
		message(STATUS
			"Warning: found python but couldn't determine which version. Please set either VRPN_PYTHON_IS_3 or VRPN_PYTHON_IS_2")
		option(VRPN_PYTHON_IS_3 "Python found is version 3.x" OFF)
		option(VRPN_PYTHON_IS_2 "Python found is version 2.x" OFF)
		if(VRPN_PYTHON_IS_3 AND VRPN_PYTHON_IS_2)
			unset(VRPN_PYTHON_IS_2 CACHE)
			unset(VRPN_PYTHON_IS_3 CACHE)
			message(FATAL_ERROR
				"If needed, please set either VRPN_PYTHON_IS_3 or VRPN_PYTHON_IS_2, not both!")
		elseif(VRPN_PYTHON_IS_3)
			set(PYTHON3_FOUND ON)
		elseif(VRPN_PYTHON_IS_2)
			set(PYTHON2_FOUND ON)
		endif()
	endif()
endif()

# If MSVC, and we don't have a debug lib, default to off, or the default (debug runtime) build is broken.
if(MSVC AND NOT PYTHON_LIBRARY_DEBUG)
	set(PYTHON_DEFAULT_OFF OFF_BY_DEFAULT)
else()
	set(PYTHON_DEFAULT_OFF ${DEFAULT_OFF_IF_SUBPROJECT})
endif()
option_requires(VRPN_BUILD_PYTHON
	"Build VRPN Python 2.x SWIG-based bindings"
	${PYTHON_DEFAULT_OFF}
	SWIG_FOUND
	PYTHON2_FOUND)

option_requires(VRPN_BUILD_PYTHON_HANDCODED_2X
	"Build VRPN Python handcoded bindings for Python 2.x"
	${PYTHON_DEFAULT_OFF}
	PYTHON2_FOUND)

option_requires(VRPN_BUILD_PYTHON_HANDCODED_3X
	"Build VRPN Python handcoded bindings for Python 3.x"
	${PYTHON_DEFAULT_OFF}
	PYTHON3_FOUND)

###
# javac, jar, and javah (for java wrapper)
###
if(ANDROID AND DEFINED find_host_package)
	find_host_package(Java COMPONENTS Development)
	find_host_package(JNI)
	find_host_program(JAVAH_EXECUTABLE NAMES javah)
else()
	find_package(Java COMPONENTS Development ${QUIET_IF_SUBPROJECT})
	find_package(JNI ${QUIET_IF_SUBPROJECT})
	find_program(JAVAH_EXECUTABLE NAMES javah)
endif()
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
find_package(MPI ${QUIET_IF_SUBPROJECT})
# XXX Safe to enable by default if we find it?
option_requires(VRPN_USE_MPI
	"Build with MPI support"
	OFF_BY_DEFAULT
	MPI_FOUND)

if(VRPN_USE_MPI)
	# XXX what else needs to be done here?
	add_definitions(${MPI_COMPILE_FLAGS})
	include_directories(${MPI_INCLUDE_PATH})
	list(APPEND EXTRA_LIBS ${MPI_LIBRARIES})
endif()

###
# Modbus library
###
find_package(Modbus ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_MODBUS
	"Build with Modbus support"
	OFF_BY_DEFAULT
	MODBUS_FOUND)

if(VRPN_USE_MODBUS)
	# XXX what else needs to be done here?
	include_directories(${MODBUS_INCLUDE_DIR})
	list(APPEND EXTRA_LIBS ${MODBUS_LIBRARY})
endif()

###
# Libusb1
###
find_package(Libusb1 ${QUIET_IF_SUBPROJECT})
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
	find_package(HIDAPI ${QUIET_IF_SUBPROJECT})
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
	option(VRPN_BUILD_HID_GUI
		"Should we build a GUI for analyzing live HID streams?"
		OFF)
	option(VRPN_HID_DEBUGGING
		"Should verbose debugging messages be displayed during HID interactions?"
		off)
	if(VRPN_HID_DEBUGGING)
		add_definitions(-DVRPN_HID_DEBUGGING)
	endif()
else()
	# Clear this variable if they don't want HID after all.
	if (VRPN_USE_LOCAL_HIDAPI)
		message(STATUS
			"NOTE: You have VRPN_USE_LOCAL_HIDAPI enabled, but "
			"VRPN_USE_HID disabled: HIDAPI will only be built if you enable HID support for VRPN")
	endif()
	set(HIDAPI_SOURCES)
endif()

###
# Sensable "OpenHaptics" HDAPI/HLAPI
###
find_package(OpenHaptics ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_HDAPI
	"Allow SensAble Phantom support through HDAPI/HLAPI - VRPN_USE_PHANTOM_SERVER must still be set"
	${DEFAULT_OFF_IF_SUBPROJECT}
	OPENHAPTICS_FOUND)
if(VRPN_USE_HDAPI)
	set(PHANTOM_POSSIBLE ON)
endif()

###
# Sensable GHOST
###
if(NOT VRPN_USE_HDAPI)
	find_package(GHOST ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_USE_GHOST
		"Allow SensAble Phantom support through GHOST - VRPN_USE_PHANTOM_SERVER must still be set"
		${DEFAULT_OFF_IF_SUBPROJECT}
		GHOST_FOUND)
	if(VRPN_USE_GHOST)
		if(NOT ${GHOST_LIBRARIES} MATCHES ".*40.*")
			message(STATUS "GHOST pre-4.0 detected - calling it 3.1.")
			set(VRPN_USE_GHOST_31 ON)
		endif()
		set(PHANTOM_POSSIBLE ON)
	endif()
endif()

###
# Sensable PHANToM Support - Overall Option
###
option_requires(VRPN_USE_PHANTOM_SERVER
	"Build with SensAble Phantom support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	PHANTOM_POSSIBLE)

if(VRPN_USE_PHANTOM_SERVER)
	if(VRPN_USE_HDAPI)
		include_directories(${OPENHAPTICS_INCLUDE_DIRS})
		list(APPEND SERVER_EXTRA_LIBS ${OPENHAPTICS_LIBRARIES})
	else()
		# VRPN_USE_GHOST
		include_directories(${GHOST_INCLUDE_DIRS})
		list(APPEND SERVER_EXTRA_LIBS ${GHOST_LIBRARIES})
	endif()
endif()


###
# libi2c-dev
###
find_package(libi2c-dev ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_I2CDEV
	"Build devices requiring libi2c-dev"
	${DEFAULT_OFF_IF_SUBPROJECT}
	I2CDEV_FOUND)

if(VRPN_USE_I2CDEV)
	include_directories(AFTER ${I2CDEV_INCLUDE_DIRS})
endif()

###
# WiiUse
###
find_package(WiiUse ${QUIET_IF_SUBPROJECT})
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
##
## Setting up the local JSONCPP was handled above, in the submodules directory
if(NOT VRPN_USE_LOCAL_JSONCPP)
	find_package(JsonCpp ${QUIET_IF_SUBPROJECT})
endif()
option_requires(VRPN_USE_JSONNET
	"Build with JSONCPP (for Android widgets and other UDP JSON-based APIs)"
	${DEFAULT_OFF_IF_SUBPROJECT}
	JSONCPP_FOUND)

if(VRPN_USE_JSONNET)
	include_directories(BEFORE ${JSONCPP_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${JSONCPP_LIBRARY})
else()
	set(JSONCPP_SOURCES)
endif()

###
# libnifalcon
###
find_package(LibNifalcon ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_LIBNIFALCON
	"Build with libnifalcon support to access Novint Falcon devices"
	${DEFAULT_OFF_IF_SUBPROJECT}
	LIBNIFALCON_FOUND)

if(VRPN_USE_LIBNIFALCON)
	include_directories(${LIBNIFALCON_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${LIBNIFALCON_LIBRARIES})
endif()

###
# DirectInput and XInput
###
if(WIN32)
	find_package(DirectX ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_USE_DIRECTINPUT
		"Build with Microsoft DirectInput support"
		${DEFAULT_OFF_IF_SUBPROJECT}
		DIRECTX_DXGUID_LIBRARY
		DIRECTX_DINPUT_LIBRARY
		DIRECTX_DINPUT_INCLUDE_DIR
		DIRECTX_SDK_SUPPORTS_COMPILER)
	if(VRPN_USE_DIRECTINPUT)
		include_directories(${DIRECTX_INCLUDE_DIRS})
		list(APPEND
			SERVER_EXTRA_LIBS
			${DIRECTX_DXGUID_LIBRARY}
			${DIRECTX_DINPUT_LIBRARY})
		if(MSVC)
			# Delay-load DirectInput DLL
			# TODO is this always the right name
			# TODO how to do this with MinGW? Can we? Do we need to?
			list(APPEND SERVER_LINK_FLAGS "/DELAYLOAD:dinput8.dll")
			list(APPEND SERVER_EXTRA_LIBS "delayimp.lib")
		endif()
	endif()

	# XInput - enhanced API for XBOX 360 controllers and more.
	find_directx_include(DIRECTX_XINPUT_INCLUDE_DIR
		xinput.h)
	option_requires(VRPN_USE_WINDOWS_XINPUT
		"Build with Microsoft XInput support"
		${DEFAULT_OFF_IF_SUBPROJECT}
		DIRECTX_XINPUT_INCLUDE_DIR
		DIRECTX_XINPUT_LIBRARY
		DIRECTX_SDK_SUPPORTS_COMPILER) #TODO will this be a valid check here?
	if(VRPN_USE_WINDOWS_XINPUT)
		list(APPEND
			SERVER_EXTRA_LIBS
			${DIRECTX_XINPUT_LIBRARY})
		mark_as_advanced(DIRECTX_XINPUT_INCLUDE_DIR)

		# Get name of DLL to delay-load.
		include(GetDefineString)
		get_define_string(NAME XINPUT_DLL
			INCLUDES windows.h xinput.h
			DEFINES "-D_WIN32_WINNT=${WIN_MIN_VER}"
			INCLUDE_DIRS ${DIRECTX_DINPUT_INCLUDE_DIR} ${DIRECTX_XINPUT_INCLUDE_DIR}
			RESULT VRPN_XINPUT_DLL)
		if(VRPN_XINPUT_DLL AND MSVC)
			# Delay-load XInput DLL
			# TODO how to do this with MinGW? Can we? Do we need to?
			list(APPEND SERVER_LINK_FLAGS "/DELAYLOAD:${VRPN_XINPUT_DLL}")
			list(APPEND SERVER_EXTRA_LIBS "delayimp.lib")
		endif()
	endif()

	# ATL is used for the vrpn_Tracker_zSight (which is based on DirectInput)
	# to provide a smart pointer, but ATL isn't always available.
	include(CheckIncludeFileCXX)
	check_include_file_cxx(atlbase.h VRPN_HAVE_ATLBASE)
endif()


###
# DirectShow
###
# Note that header removal makes this harder for VS10 and later - you need an earlier
# version of MSVC also installed or an older Windows/Platform SDK with qedit.h in it.
if(MSVC)
	if(MSVC_VERSION GREATER 1500)
		set(DIRECTSHOW_OFF_BY_DEFAULT OFF_BY_DEFAULT)
	else()
		set(DIRECTSHOW_OFF_BY_DEFAULT)
	endif()
	find_package(DirectShow ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_USE_DIRECTSHOW
		"Build with Microsoft DirectShow support"
		${DIRECTSHOW_OFF_BY_DEFAULT}
		DIRECTSHOW_FOUND)
	option_requires(VRPN_BUILD_DIRECTSHOW_VIDEO_SERVER
		"Enable to build DirectShow Video Server (Windows)"
		${DEFAULT_OFF_IF_SUBPROJECT}
		VRPN_BUILD_DIRECTSHOW_VIDEO_SERVER
		DIRECTSHOW_FOUND)
endif()

###
# GPM
###
if(UNIX)
	find_package(GPM ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_USE_GPM_MOUSE
		"Build with GPM Linux mouse interface support (makes servers GPL)"
		${DEFAULT_OFF_IF_SUBPROJECT}
		GPM_FOUND)
endif()

if(VRPN_USE_GPM_MOUSE)
	list(APPEND SERVER_EXTRA_LIBS ${GPM_LIBRARIES})
endif()

###
# INTERSENSE
###
find_package(InterSense ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_INCLUDE_INTERSENSE
	"Build with InterSense support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	INTERSENSE_FOUND)

if(VRPN_INCLUDE_INTERSENSE)
	include_directories(${INTERSENSE_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${INTERSENSE_LIBRARIES})
endif()

###
# THALMICLABSMYO
###
if(WIN32)
	find_package(ThalmicLabsMyo ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_INCLUDE_THALMICLABSMYO
		"Build with Thalmic Labs' Myo support"
		${DEFAULT_OFF_IF_SUBPROJECT}
		THALMICLABSMYO_FOUND)
	if(VRPN_INCLUDE_THALMICLABSMYO)
		include_directories(${THALMICLABSMYO_INCLUDE_DIRS})
		list(APPEND SERVER_EXTRA_LIBS ${THALMICLABSMYO_LIBRARIES})
		add_definitions(-DVRPN_INCLUDE_THALMICLABSMYO)
	endif()
endif()

###
# NIDAQMX
###
find_package(NIDAQmx ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_NATIONAL_INSTRUMENTS_MX
	"Build with National Instruments NIDAQMX support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	NIDAQMX_FOUND)

if(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
	include_directories(${NIDAQMX_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${NIDAQMX_LIBRARIES})
endif()

###
# Arrington Research ViewPoint EyeTracker
###
find_package(ViewPoint ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_VIEWPOINT
	"Build with support for ViewPoint EyeTracker"
	${DEFAULT_OFF_IF_SUBPROJECT}
	VIEWPOINT_FOUND)

if(VRPN_USE_VIEWPOINT)
	include_directories(${VIEWPOINT_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${VIEWPOINT_LIBRARIES})

	# Needed for the config file, apparently - it was added to the non-CMake one.
	get_filename_component(VRPN_VIEWPOINT_LIB_PATH
		"${VIEWPOINT_LIBRARY}"
		PATH)
	file(TO_CMAKE_PATH
		"${VRPN_VIEWPOINT_LIB_PATH}"
		VRPN_VIEWPOINT_LIB_PATH)
endif()

###
# Adrienne timecode boards
###
if(WIN32 OR CYGWIN)
	find_package(Adrienne ${QUIET_IF_SUBPROJECT})
	option_requires(VRPN_INCLUDE_TIMECODE_SERVER
		"Build with support for Adrienne timecode server"
		${DEFAULT_OFF_IF_SUBPROJECT}
		ADRIENNE_FOUND)
else()
	set(VRPN_INCLUDEE_TIMECODE_SERVER OFF)
endif()

if(VRPN_INCLUDE_TIMECODE_SERVER)
	set(VRPN_ADRIENNE_INCLUDE_FILENAME "${ADRIENNE_INCLUDE_FILENAME}")
	set(VRPN_ADRIENNE_INCLUDE_HAS_EXTERN_C
		${ADRIENNE_INCLUDE_HAS_EXTERN_C})
endif()

###
# Linux kernel joystick interface
###
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	check_include_file_cxx(linux/joystick.h HAVE_LINUX_JOYSTICK_H)
	option_requires(VRPN_USE_JOYLIN
		"Build with support for Linux kernel joystick interface (Uses kernel header - may make servers GPL)"
		${DEFAULT_OFF_IF_SUBPROJECT}
		HAVE_LINUX_JOYSTICK_H)
else()
	set(VRPN_USE_JOYLIN OFF)
endif()

###
# /dev/input kernel joystick interface
###
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	check_include_file_cxx(linux/input.h HAVE_LINUX_INPUT_H)
	option_requires(VRPN_USE_DEV_INPUT
		"Build with flags to enable the use of DevInput. (Uses kernel header - may make servers GPL)"
		${DEFAULT_OFF_IF_SUBPROJECT}
		HAVE_LINUX_INPUT_H)
else()
	set(VRPN_USE_DEV_INPUT OFF)
endif()

###
# Perl, for vrpn_rpc_gen
###
if(NOT WIN32)
	# TODO - a bit of a compatibility hack
	# MiKTeX 2.9's perl is broken - missing libgcc-s-sjlj-1.dll -
	# and can't seem to work around it without popping up dialogs
	# which is no fun for automated builds.
	find_package(Perl ${QUIET_IF_SUBPROJECT})
	find_package(PerlModules ${QUIET_IF_SUBPROJECT} COMPONENTS Parse::RecDescent)
endif()

option_requires(VRPN_BUILD_TEST_RPC_GENERATION
	"Build VRPN RPC generation"
	${DEFAULT_OFF_IF_SUBPROJECT}
	PERL_FOUND
	PERLMODULES_FOUND)

###
# Polhemus PDI library
###
# TODO Generalize this to use a FindPDI.cmake - someone with access
# to this library must do it.
# Make this also work with debug (use PDID)
option(VRPN_USE_PDI
	"Build with flags to enable the use of Polhemus DVI library."
	OFF)
if(VRPN_USE_PDI)
	find_path(POLHEMUS_PDI_DIR NAMES "Inc" "Lib" PATH_SUFFIXES "PDI_140" "PDI_110" "PDI_90" PATHS "C:/Polhemus/PDI" "C:/Program Files (x86)/Polhemus/PDI" NO_DEFAULT_PATH)
	include_directories(${POLHEMUS_PDI_DIR}/Inc)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		list(APPEND SERVER_EXTRA_LIBS "${POLHEMUS_PDI_DIR}/Lib/x64/PDI.lib")
	else()
		list(APPEND SERVER_EXTRA_LIBS "${POLHEMUS_PDI_DIR}/Lib/Win32/PDI.lib")
	endif()
endif()

###
# PhaseSpace API
###
# TODO Actually do a proper search for the PhaseSpace API headers and libraries
# and not use link_directories.
option(VRPN_INCLUDE_PHASESPACE
	"Build with PhaseSpace library support"
	OFF)
set(PHASESPACE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/phasespace" CACHE PATH "location of PhaseSpace headers")
set(PHASESPACE_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/phasespace" CACHE PATH "location of PhaseSpace libraries (libowlsock.so, etc")
if(VRPN_INCLUDE_PHASESPACE)
	include_directories(${PHASESPACE_INCLUDE_DIR})
	link_directories(${PHASESPACE_LIBRARY_DIR})
	if(WIN32)
		list(APPEND SERVER_EXTRA_LIBS libowlsock)
	else()
		list(APPEND SERVER_EXTRA_LIBS owlsock)
	endif()
endif()

###
# Hillcrest Labs' Freespace
###
find_package(LibFreespace ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_FREESPACE
	"Build with Hillcrest Labs' Freespace devices support"
	${DEFAULT_OFF_IF_SUBPROJECT}
	LIBFREESPACE_FOUND)

if(VRPN_USE_FREESPACE)
	include_directories(${LIBFREESPACE_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${LIBFREESPACE_LIBRARIES})
endif()

###
# MotionNode tracker support. Loads shared library dynamically if available
###
option(VRPN_USE_MOTIONNODE
	"Build with MotionNode tracker support"
	OFF)
if(VRPN_USE_MOTIONNODE)
	list(APPEND SERVER_EXTRA_LIBS ${CMAKE_DL_LIBS})
endif()

###
# Trivisio ColibriAPI
###
find_package(ColibriApi ${QUIET_IF_SUBPROJECT})
option_requires(VRPN_USE_COLIBRIAPI
	"Build with Trivisio ColibriAPI support"
	OFF_BY_DEFAULT
	COLIBRIAPI_FOUND)
if(VRPN_USE_COLIBRIAPI)
	include_directories(${COLIBRIAPI_INCLUDE_DIR})
	list(APPEND SERVER_EXTRA_LIBS ${COLIBRIAPI_LIBRARIES})
endif()

###
# XXX Other libraries needing detection and handling (TODO)
###
# National Instruments Nidaq traditional
# US Digital SEI/A2
# microscribe3D library
#
# All include paths should be moved out of at least vrpn_Configure.h.cmake_in
# as well as all #pragma comment (lib, "" ) lines, since cmake replaces
# them more flexibly (include_directories and target_link_libraries)


# Configuration options controlling what gets included in the build.
# These are the default options for libraries we can't yet detect -
# if a library can be detected above it will be pre-set to an appropriate
# value by default.
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
option(VRPN_USE_TRIVISIOCOLIBRI
	"(OBSOLETE) Build with support for TrivisioColibri tracker"
	OFF)
if(WIN32)
	option(VRPN_USE_WINSOCK2
		"Use Winsock2 library, rather than Winsock."
		OFF)
	option(VRPN_USE_SHARED_LIBRARY
		"Enable to use DLLs on Windows (see vrpn_Configure.h for more info)"
		OFF)
endif()

if(UNIX)
	option(VRPN_BUILD_PROFILING_SUPPORT
		"Build with flags to enable profiling."
		OFF)
	if(VRPN_BUILD_PROFILING_SUPPORT)
		include(EnableProfiling)
		globally_enable_profiling()
	endif()

endif()

if(WIN32 AND NOT CYGWIN)
	# Only intended for Windows, known not needed on Cygwin,
	# but potentially hazardous on other Windows build environments.
	option(VRPN_EXPORT_GETTIMEOFDAY "Use a #define to make gettimeofday a synonym for vrpn_gettimeofday." OFF)
else()
	set(VRPN_EXPORT_GETTIMEOFDAY OFF)
endif()

# This will only work on compilers that support C++-11.
# We could check for this and turn it on by default.
set(VRPN_CHRONO_DEFAULT OFF)
if(MSVC AND MSVC_VERSION GREATER 1800)
	# On MSVC 2015 and newer, std::chrono provides the best time implementation.
	set(VRPN_CHRONO_DEFAULT ON)
endif()

option(VRPN_USE_STD_CHRONO "Use the std::chrono class for time" ${VRPN_CHRONO_DEFAULT})

option(VRPN_USE_STATIC_ASSERTIONS "Use some compile-time assertions" ON)
option(VRPN_BUILD_EXTRA_COMPILER_WARNINGS
	"Build with flags to enable extra warnings."
	OFF)
if(VRPN_BUILD_EXTRA_COMPILER_WARNINGS)
	include(EnableExtraCompilerWarnings)
	globally_enable_extra_compiler_warnings()
	if(NOT MSVC)
		# shadowing virtual functions is pretty much unavoidable with the
		# multiple inheritance design and methods like "report_changes"
		check_cxx_compiler_flag(-Wno-overloaded-virtual SUPPORTS_WNO_OVERLOADED_VIRTUAL_FLAG)
		if(SUPPORTS_WNO_OVERLOADED_VIRTUAL_FLAG)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
		endif()
	endif()
endif()

set(VRPN_CLIENT_ONLY)
if(VRPN_BUILD_CLIENT_LIBRARY AND NOT VRPN_BUILD_SERVER_LIBRARY)
	# We can define VRPN_CLIENT_ONLY in the header in this case!
	set(VRPN_CLIENT_ONLY ON)
endif()

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h")
set(VRPN_PATH_TO_CMAKE_CONFIG "${PROJECT_BINARY_DIR}/vrpn_Configure.h")
add_definitions("-DVRPN_USING_CMAKE=\"${VRPN_PATH_TO_CMAKE_CONFIG}\"")

if(SUBPROJECT)
	set(BUILD_TESTING FALSE)
endif()

if(APPLE)
	# XXX Is this still needed?
	if(NOT CMAKE_INSTALL_NAME_DIR)
		set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
	endif()
endif()

#-----------------------------------------------------------------------------
# Build the library itself and declare what bits need to be installed

set(VRPN_CLIENT_SOURCES
	vrpn_Analog.C
	vrpn_Analog_Output.C
	vrpn_Assert.C
	vrpn_Auxiliary_Logger.C
	vrpn_BaseClass.C
	vrpn_Button.C
	vrpn_Connection.C
	vrpn_Dial.C
	vrpn_EndpointContainer.C
	vrpn_FileConnection.C
	vrpn_FileController.C
	vrpn_ForceDevice.C
	vrpn_Forwarder.C
	vrpn_ForwarderController.C
	vrpn_FunctionGenerator.C
	vrpn_Imager.C
	vrpn_LamportClock.C
	vrpn_Mutex.C
	vrpn_Poser.C
	vrpn_RedundantTransmission.C
	vrpn_Serial.C
	vrpn_SerialPort.C
	vrpn_Shared.C
	vrpn_SharedObject.C
	vrpn_Sound.C
	vrpn_Text.C
	vrpn_Thread.C
	vrpn_Tracker.C)

set(VRPN_CLIENT_PUBLIC_HEADERS
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h"
	vrpn_Analog.h
	vrpn_Analog_Output.h
	vrpn_Assert.h
	vrpn_Auxiliary_Logger.h
	vrpn_BaseClass.h
	vrpn_Button.h
	vrpn_Connection.h
	vrpn_ConnectionPtr.h
	vrpn_Dial.h
	vrpn_EndpointContainer.h
	vrpn_FileConnection.h
	vrpn_FileController.h
	vrpn_ForceDevice.h
	vrpn_ForwarderController.h
	vrpn_Forwarder.h
	vrpn_FunctionGenerator.h
	vrpn_Imager.h
	vrpn_LamportClock.h
	vrpn_Log.h
	vrpn_MainloopContainer.h
	vrpn_MainloopObject.h
	vrpn_Mutex.h
	vrpn_OwningPtr.h
	vrpn_RedundantTransmission.h
	vrpn_SendTextMessageStreamProxy.h
	vrpn_Serial.h
	vrpn_SerialPort.h
	vrpn_Shared.h
	vrpn_SharedObject.h
	vrpn_Sound.h
	vrpn_Text.h
	vrpn_Thread.h
	vrpn_Tracker.h
	vrpn_Types.h
	vrpn_WindowsH.h)

set(VRPN_SERVER_SOURCES
	${VRPN_CLIENT_SOURCES}
	vrpn_3DConnexion.C
	vrpn_3DMicroscribe.C
	vrpn_3Space.C
	vrpn_5DT16.C
	vrpn_Adafruit.C
	vrpn_ADBox.C
	vrpn_Analog_5dt.C
	vrpn_Analog_5dtUSB.C
	vrpn_Analog_Radamec_SPI.C
	vrpn_Analog_USDigital_A2.C
	vrpn_Atmel.C
	vrpn_BiosciencesTools.C
	vrpn_Button_NI_DIO24.C
	vrpn_Button_USB.cpp
	vrpn_CerealBox.C
	vrpn_CHProducts_Controller_Raw.C
	vrpn_Contour.C
	vrpn_DevInput.C
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
	vrpn_Futaba.C
	vrpn_GlobalHapticsOrb.C
	vrpn_Griffin.C
	vrpn_HumanInterface.C
	vrpn_IDEA.C
	vrpn_Imager_Stream_Buffer.C
	vrpn_ImmersionBox.C
	vrpn_inertiamouse.C
	vrpn_JoyFly.C
	vrpn_Joylin.C
	vrpn_Joywin32.C
	vrpn_Keyboard.C
	vrpn_Laputa.C
	vrpn_LUDL.C
	vrpn_Logitech_Controller_Raw.C
	vrpn_Magellan.C
	vrpn_Microsoft_Controller_Raw.C
	vrpn_Mouse.C
	vrpn_NationalInstruments.C
	vrpn_Nidaq.C
	vrpn_nikon_controls.C
	vrpn_nVidia_shield_controller.C
	vrpn_Oculus.C
	vrpn_OzzMaker.C
	vrpn_OmegaTemperature.C
	vrpn_Poser_Analog.C
	vrpn_Poser_Tek4662.C
	vrpn_raw_sgibox.C
	vrpn_Retrolink.C
	vrpn_Saitek_Controller_Raw.C
	vrpn_sgibox.C
	vrpn_Spaceball.C
	vrpn_Streaming_Arduino.C
	vrpn_Tng3.C
	vrpn_Tracker_3DMouse.C
	vrpn_Tracker_AnalogFly.C
	vrpn_Tracker_ButtonFly.C
	vrpn_Tracker_Crossbow.C
	vrpn_Tracker_DTrack.C
	vrpn_Tracker_Fastrak.C
	vrpn_Tracker_Filter.C
	vrpn_Tracker_GameTrak.C
	vrpn_Tracker_GPS.C
	vrpn_Tracker_IMU.C
	vrpn_Tracker_isense.C
	vrpn_Tracker_Isotrak.C
	vrpn_Tracker_JsonNet.C
	vrpn_Tracker_Liberty.C
	vrpn_Tracker_MotionNode.C
	vrpn_Tracker_NDI_Polaris.C
	vrpn_Tracker_NovintFalcon.C
	vrpn_Tracker_OSVRHackerDevKit.C
	vrpn_Tracker_PDI.C
	vrpn_Tracker_PhaseSpace.C
	vrpn_Tracker_RazerHydra.C
	vrpn_Tracker_ThalmicLabsMyo.C
	vrpn_Tracker_SpacePoint.C
	vrpn_Tracker_Wintracker.C
	vrpn_Tracker_Colibri.C
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
	vrpn_Tracker_LibertyHS.C
	vrpn_Vality.C
	vrpn_YEI_3Space.C
	vrpn_Zaber.C
	server_src/vrpn_Generic_server_object.C
	${HIDAPI_SOURCES}
	${JSONCPP_SOURCES}
	${PDI_SOURCES})

set(VRPN_SERVER_PUBLIC_HEADERS
	${VRPN_CLIENT_PUBLIC_HEADERS}
	vrpn_3DConnexion.h
	vrpn_3DMicroscribe.h
	vrpn_3Space.h
	vrpn_5DT16.h
	vrpn_Adafruit.h
	vrpn_ADBox.h
	vrpn_Analog_5dt.h
	vrpn_Analog_5dtUSB.h
	vrpn_Analog_Radamec_SPI.h
	vrpn_Analog_USDigital_A2.h
	vrpn_Atmel.h
	vrpn_BiosciencesTools.h
	vrpn_Button_NI_DIO24.h
	vrpn_Button_USB.h
	vrpn_CerealBox.h
	vrpn_CHProducts_Controller_Raw.h
	vrpn_Contour.h
	vrpn_DevInput.h
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
	vrpn_Futaba.h
	vrpn_GlobalHapticsOrb.h
	vrpn_Griffin.h
	vrpn_HashST.h
	vrpn_HumanInterface.h
	vrpn_IDEA.h
	vrpn_Imager_Stream_Buffer.h
	vrpn_ImmersionBox.h
	vrpn_inertiamouse.h
	vrpn_JoyFly.h
	vrpn_Joylin.h
	vrpn_Joywin32.h
	vrpn_Keyboard.h
	vrpn_Logitech_Controller_Raw.h
	vrpn_Laputa.h
	vrpn_LUDL.h
	vrpn_Magellan.h
	vrpn_MessageMacros.h
	vrpn_Microsoft_Controller_Raw.h
	vrpn_Mouse.h
	vrpn_NationalInstruments.h
	vrpn_Nidaq.h
	vrpn_nikon_controls.h
	vrpn_nVidia_shield_controller.h
	vrpn_Oculus.h
	vrpn_OmegaTemperature.h
	vrpn_OneEuroFilter.h
	vrpn_OzzMaker.h
	vrpn_Poser_Analog.h
	vrpn_Poser.h
	vrpn_Poser_Tek4662.h
	vrpn_raw_sgibox.h
	vrpn_Retrolink.h
	vrpn_Saitek_Controller_Raw.h
	vrpn_sgibox.h
	vrpn_Spaceball.h
	vrpn_Streaming_Arduino.h
	vrpn_Tng3.h
	vrpn_Tracker_3DMouse.h
	vrpn_Tracker_AnalogFly.h
	vrpn_Tracker_ButtonFly.h
	vrpn_Tracker_Crossbow.h
	vrpn_Tracker_DTrack.h
	vrpn_Tracker_Fastrak.h
	vrpn_Tracker_Filter.h
	vrpn_Tracker_GameTrak.h
	vrpn_Tracker_GPS.h
	vrpn_Tracker_IMU.h
	vrpn_Tracker_isense.h
	vrpn_Tracker_Isotrak.h
	vrpn_Tracker_JsonNet.h
	vrpn_Tracker_Liberty.h
	vrpn_Tracker_MotionNode.h
	vrpn_Tracker_NDI_Polaris.h
	vrpn_Tracker_NovintFalcon.h
	vrpn_Tracker_OSVRHackerDevKit.h
	vrpn_Tracker_PDI.h
	vrpn_Tracker_PhaseSpace.h
	vrpn_Tracker_RazerHydra.h
	vrpn_Tracker_ThalmicLabsMyo.h
	vrpn_Tracker_SpacePoint.h
	vrpn_Tracker_Wintracker.h
	vrpn_Tracker_Colibri.h
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
	vrpn_Tracker_LibertyHS.h
	vrpn_Vality.h
	vrpn_YEI_3Space.h
	vrpn_Zaber.h
	server_src/vrpn_Generic_server_object.h)

set(VRPN_SERVER_LIBRARY)
set(VRPN_CLIENT_LIBRARY)

if(VRPN_BUILD_SERVER_LIBRARY)
	add_library(vrpnserver
		${VRPN_SERVER_SOURCES}
		${VRPN_SERVER_PUBLIC_HEADERS})
	target_link_libraries(vrpnserver ${EXTRA_LIBS} ${SERVER_EXTRA_LIBS})
	set(VRPN_CLIENT_LIBRARY vrpnserver)
	set(VRPN_SERVER_LIBRARY vrpnserver)
	set_property(TARGET
		vrpnserver
		PROPERTY
		PUBLIC_HEADER
		${VRPN_SERVER_PUBLIC_HEADERS})
	set_property(TARGET
		vrpnserver
		PROPERTY
		SOVERSION
		0)
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
	string(REPLACE ";" " " SERVER_LINK_FLAGS "${SERVER_LINK_FLAGS}")
	set_property(TARGET
		vrpnserver
		PROPERTY
		LINK_FLAGS
		${SERVER_LINK_FLAGS})

	if(UNIX)
		add_subdirectory(atmellib)
		target_link_libraries(vrpnserver vrpn_atmel)
	endif()

	add_subdirectory(gpsnmealib)
	target_link_libraries(vrpnserver gpsnmea)
	if(VRPN_INSTALL)
		install(TARGETS
			vrpnserver
			RUNTIME DESTINATION bin COMPONENT serversdk
			LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
			ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT serversdk
			PUBLIC_HEADER DESTINATION include COMPONENT serversdk)
	endif()
	add_cppcheck(vrpnserver STYLE UNUSED_FUNCTIONS)
endif()


if(VRPN_BUILD_CLIENT_LIBRARY)
	add_library(vrpn ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	target_link_libraries(vrpn ${EXTRA_LIBS})
	set(VRPN_CLIENT_LIBRARY vrpn)

	set_property(TARGET
		vrpn
		PROPERTY
		PUBLIC_HEADER
		${VRPN_CLIENT_PUBLIC_HEADERS})
	set_property(TARGET
		vrpn
		PROPERTY
		SOVERSION
		0)

	if(NOT VRPN_CLIENT_ONLY)
		set_property(TARGET
			vrpn
			PROPERTY
			COMPILE_DEFINITIONS
			"VRPN_CLIENT_ONLY")
	endif()
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

	if(VRPN_INSTALL)
		install(TARGETS
			vrpn
			RUNTIME DESTINATION bin COMPONENT clientsdk
			LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT clientsdk
			ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT clientsdk
			PUBLIC_HEADER DESTINATION include COMPONENT clientsdk)
	endif()
	add_cppcheck(vrpn STYLE UNUSED_FUNCTIONS)
endif()

add_subdirectory(client_src)

#-----------------------------------------------------------------------------
# Set some internal cache variables to make life easier if we're a subproject

set(VRPN_DEFINITIONS "-DVRPN_USING_CMAKE=\"${CMAKE_CURRENT_BINARY_DIR}/vrpn_Configure.h\""
	CACHE INTERNAL "Definition to add if using as a subproject" FORCE)
set(VRPN_CLIENT_LIBRARY ${VRPN_CLIENT_LIBRARY} CACHE INTERNAL "" FORCE)
set(VRPN_SERVER_LIBRARY ${VRPN_SERVER_LIBRARY} CACHE INTERNAL "" FORCE)
set(VRPN_SERVER_LINK_FLAGS ${SERVER_LINK_FLAGS} CACHE INTERNAL "" FORCE)
set(VRPN_INCLUDE_DIRS
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${VRPN_SOURCE_DIR}"
	"${VRPN_SOURCE_DIR}/atmellib"
	"${VRPN_SOURCE_DIR}/quat"
	CACHE INTERNAL "" FORCE)

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

if(VRPN_BUILD_CLIENTS)
	add_subdirectory(util/printStream)
endif()

#-----------------------------------------------------------------------------
# Create documentation
if(NOT SUBPROJECT)
	add_subdirectory(doxygen)
endif()

#-----------------------------------------------------------------------------
# Python wrappers
add_subdirectory(python_vrpn)
add_subdirectory(python)

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
if(BUILD_TESTING)
	foreach(SOURCE time_test.cpp)
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} vrpnserver)
		set_target_properties(${APP} PROPERTIES FOLDER Tests)
		if(VRPN_INSTALL)
			install(TARGETS ${APP} RUNTIME DESTINATION bin COMPONENT tests)
		endif()
	endforeach()
endif()

#-----------------------------------------------------------------------------
# Do a little check for GPL and GPL-incompatible libraries

# What flags cause us to link against GPL libraries?

# TODO: Figure out about the kernel headers we use.
# Conflicting stories:
# ML post by Linus: https://lkml.org/lkml/2003/12/5/13
# Interview of Linus: http://www.itworld.com/open-source/140916/android-sued-microsoft-not-linux

set(ALL_GPL_SERVER_FLAGS
	#VRPN_USE_DEV_INPUT # Kernel header: GPL2
	#VRPN_USE_JOYLIN # Kernel header: GPL2
	VRPN_USE_GPM_MOUSE # GPL2+
	VRPN_USE_WIIUSE # GPL3
)

# What flags cause us to link against GPL-incompatible libraries?
set(ALL_GPLINCOMPAT_SERVER_FLAGS
	VRPN_INCLUDE_INTERSENSE
	VRPN_INCLUDE_TIMECODE_SERVER
	VRPN_USE_PHANTOM_SERVER)

set(GPL_SERVER_FLAGS)
foreach(POSSIBLE_GPL_FLAG ${ALL_GPL_SERVER_FLAGS})
	if(${POSSIBLE_GPL_FLAG})
		list(APPEND GPL_SERVER_FLAGS "${POSSIBLE_GPL_FLAG}")
	endif()
endforeach()

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
if(NOT SUBPROJECT)
	create_dashboard_scripts("${CMAKE_CURRENT_SOURCE_DIR}/DashboardBuildInitialCache.cmake.in")
endif()

#-----------------------------------------------------------------------------
# If we succeeded, we can go on and include packaging!
if(VRPN_INSTALL)
	include(CPack)
	cpack_add_component(serversdk
		DISPLAY_NAME
		"VRPN Server Library and C++ Headers")
	cpack_add_component(clientsdk
		DISPLAY_NAME
		"VRPN Client Library and C++ Headers")
	cpack_add_component(tests DISPLAY_NAME "Test applications")
	cpack_add_component(clients DISPLAY_NAME "Client applications")
	cpack_add_component(servers DISPLAY_NAME "Server applications")
	cpack_add_component(mainserver
		DISPLAY_NAME
		"VRPN main server application")
	cpack_add_component(python DISPLAY_NAME "Python bindings")
	cpack_add_component(java DISPLAY_NAME "Java bindings")
	cpack_add_component(doc DISPLAY_NAME "C++ API Documentation")
endif()
