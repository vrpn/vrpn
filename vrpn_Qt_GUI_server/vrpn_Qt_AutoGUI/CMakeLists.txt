cmake_minimum_required(VERSION 2.6)

project(vrpn_Qt_AutoGUI)

set(EXECUTABLE_OUTPUT_PATH "${vrpn_Qt_BINARY_DIR}/bin")


#######################################
# Include Qt
#######################################

# Set up variables for moc
set(QT_HEADER QAutoGUI.h)
set(QT_SRC vrpn_Qt_AutoGUI.cpp QAutoGUI.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR})

# Do moc stuff
set(QT_MOC_SRC)
qt4_wrap_cpp(QT_MOC_SRC ${QT_HEADER})
add_definitions(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)


#######################################
# Build setup
#######################################

set(SRC QXmlAutoGUIHandler.h QXmlAutoGUIHandler.cpp vrpn_Qt_AutoGUI.xml)

add_executable(vrpn_Qt_AutoGUI
	${QT_HEADER}
	${QT_SRC}
	${QT_MOC_SRC}
	${SRC})
add_dependencies(vrpn_Qt_AutoGUI vrpn_Qt)
target_link_libraries(vrpn_Qt_AutoGUI vrpn_Qt)

add_custom_command(TARGET
	vrpn_Qt_AutoGUI
	PRE_BUILD
	COMMAND
	${CMAKE_COMMAND}
	-E
	copy
	${vrpn_Qt_AutoGUI_SOURCE_DIR}/vrpn_Qt_AutoGUI.xml
	${EXECUTABLE_OUTPUT_PATH}/Release)

add_custom_command(TARGET
	vrpn_Qt_AutoGUI
	PRE_BUILD
	COMMAND
	${CMAKE_COMMAND}
	-E
	copy
	${vrpn_Qt_AutoGUI_SOURCE_DIR}/vrpn_Qt_AutoGUI.xml
	${EXECUTABLE_OUTPUT_PATH}/Debug)

