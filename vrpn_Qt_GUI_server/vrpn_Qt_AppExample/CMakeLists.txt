cmake_minimum_required(VERSION 2.6)

project(vrpn_Qt_AppExample)

set(EXECUTABLE_OUTPUT_PATH "${vrpn_Qt_BINARY_DIR}/bin")


#######################################
# Qt moc
#######################################

# Set up variables for moc
set(QT_UI MainWindow.ui)
set(QT_HEADER MainWindow.h)
set(QT_SRC vrpn_Qt_AppExample.cpp MainWindow.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

# Do moc stuff
set(QT_UI_HEADER)
set(QT_MOC_SRC)
qt4_wrap_ui(QT_UI_HEADER ${QT_UI})
qt4_wrap_cpp(QT_MOC_SRC ${QT_HEADER})
set_source_files_properties(${QT_SRC}
	PROPERTIES
	OBJECT_DEPENDS
	"${QT_UI_HEADER}")


#######################################
# Build setup
#######################################

add_executable(vrpn_Qt_AppExample ${QT_HEADER} ${QT_SRC} ${QT_MOC_SRC})
add_dependencies(vrpn_Qt_AppExample vrpn_Qt)
target_link_libraries(vrpn_Qt_AppExample vrpn_Qt)

