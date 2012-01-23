CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( vrpn_Qt )

SET( EXECUTABLE_OUTPUT_PATH "${vrpn_Qt_BINARY_DIR}/bin" )

#-----------------------------------------------------------------------------
# Local CMake Modules
list(APPEND CMAKE_MODULE_PATH ${vrpn_Qt_SOURCE_DIR} ${vrpn_Qt_SOURCE_DIR}/cmake)


#######################################
# Build shared library or not
#######################################

OPTION( BUILD_SHARED_LIBS 0 )


#######################################
# Include Qt
#######################################

# Need XML for QAutoGUI
SET(QT_USE_QTXML TRUE)

#Use Qt
FIND_PACKAGE( Qt4 REQUIRED )
INCLUDE( ${QT_USE_FILE} )

INCLUDE_DIRECTORIES( ${QT_INCLUDE_DIR}
                     ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR} )

# Set up variables for moc
SET( QT_HEADER vrpn_Qt.h )
SET( QT_SRC vrpn_Qt.C )

# Do moc stuff
QT4_WRAP_CPP( QT_MOC_SRC ${QT_HEADER} )


#######################################
# Include vrpn
#######################################

FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
FIND_PACKAGE(VRPN REQUIRED)

INCLUDE_DIRECTORIES( ${VRPN_INCLUDE_DIR} )


#######################################
# Build setup 
#######################################

ADD_LIBRARY( vrpn_Qt ${QT_HEADER} ${QT_SRC} ${QT_MOC_SRC} ${SRC} )
TARGET_LINK_LIBRARIES( vrpn_Qt 
                       ${QT_LIBRARIES}
			           ${VRPN_LIBRARY}
)
if(WIN32)	# MS-Windows, both 32 and 64 bits
	target_link_libraries(vrpn_Qt  wsock32)
elseif(APPLE)	# Apple
	target_link_libraries(vrpn_Qt  m)	# Math library
elseif(UNIX)	# other UNIXes
	target_link_libraries(vrpn_Qt  m)	# Math library
endif()


#######################################
# Include applications 
#######################################

ADD_SUBDIRECTORY( vrpn_Qt_AutoGUI )
ADD_SUBDIRECTORY( vrpn_Qt_AppExample )

