cmake_minimum_required(VERSION 2.6)

project(vrpn_Qt)

set(EXECUTABLE_OUTPUT_PATH "${vrpn_Qt_BINARY_DIR}/bin")

#-----------------------------------------------------------------------------
# Local CMake Modules
list(APPEND
	CMAKE_MODULE_PATH
	${vrpn_Qt_SOURCE_DIR}
	${vrpn_Qt_SOURCE_DIR}/cmake)


#######################################
# Build shared library or not
#######################################

option(BUILD_SHARED_LIBS 0)


#######################################
# Include Qt
#######################################

# Need XML for QAutoGUI
set(QT_USE_QTXML TRUE)

#Use Qt
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

include_directories(${QT_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

# Set up variables for moc
set(QT_HEADER vrpn_Qt.h)
set(QT_SRC vrpn_Qt.C)

# Do moc stuff
qt4_wrap_cpp(QT_MOC_SRC ${QT_HEADER})


#######################################
# Include vrpn
#######################################

find_package(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
find_package(VRPN REQUIRED)

include_directories(${VRPN_INCLUDE_DIR})


#######################################
# Build setup
#######################################

add_library(vrpn_Qt ${QT_HEADER} ${QT_SRC} ${QT_MOC_SRC} ${SRC})
target_link_libraries(vrpn_Qt ${QT_LIBRARIES} ${VRPN_LIBRARY})
if(WIN32)	# MS-Windows, both 32 and 64 bits
	target_link_libraries(vrpn_Qt wsock32)
elseif(APPLE)	# Apple
	target_link_libraries(vrpn_Qt m)	# Math library
elseif(UNIX)	# other UNIXes
	target_link_libraries(vrpn_Qt m)	# Math library
endif()


#######################################
# Include applications
#######################################

add_subdirectory(vrpn_Qt_AutoGUI)
add_subdirectory(vrpn_Qt_AppExample)

