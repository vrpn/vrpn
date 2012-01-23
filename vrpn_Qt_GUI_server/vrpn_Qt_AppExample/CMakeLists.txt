CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( vrpn_Qt_AppExample )

SET( EXECUTABLE_OUTPUT_PATH "${vrpn_Qt_BINARY_DIR}/bin" )


#######################################
# Qt moc
#######################################

# Set up variables for moc
SET( QT_UI MainWindow.ui )
SET( QT_HEADER MainWindow.h )
SET( QT_SRC vrpn_Qt_AppExample.cpp MainWindow.cpp )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR} )

# Do moc stuff
SET( QT_UI_HEADER )
SET( QT_MOC_SRC )
QT4_WRAP_UI( QT_UI_HEADER ${QT_UI} )
QT4_WRAP_CPP( QT_MOC_SRC ${QT_HEADER} )
SET_SOURCE_FILES_PROPERTIES( ${QT_SRC} PROPERTIES OBJECT_DEPENDS "${QT_UI_HEADER}" )


#######################################
# Build setup 
#######################################

ADD_EXECUTABLE( vrpn_Qt_AppExample ${QT_HEADER} ${QT_SRC} ${QT_MOC_SRC} )
ADD_DEPENDENCIES( vrpn_Qt_AppExample vrpn_Qt )
TARGET_LINK_LIBRARIES( vrpn_Qt_AppExample vrpn_Qt )	

