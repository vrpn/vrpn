
if(NOT VRPN_BUILD_SERVER_LIBRARY OR NOT VRPN_USE_HID)
	message(FATAL_ERROR
		"Cannot compile HID gui without the full (server) library and HID support.  Enable it and try again.")
endif()

set(VRPN_SERVER_LIBRARY vrpnserver)

if(NOT WIN32)
	set(VRPN_ATMEL_LIBRARY vrpn_atmel)
endif()

set(SOURCES
	HIDDevice.cpp
	Inspector.cpp
	main.cpp
	MainWindow.cpp
	QuickChart.cpp)

set(HEADERS)

set(MOCS
	HIDDevice.h
	Inspector.h
	MainWindow.h
	QuickChart.h)

set(UIS
	mainwindow.ui
	plot.ui)

find_package(Qt5 COMPONENTS Gui Widgets)
if(NOT TARGET Qt5::Core)
	message("No Qt5")
	find_package(Qt4 4.7)
endif()

if(TARGET Qt5::Core)
	qt5_wrap_ui(GENERATED_SOURCES ${UIS})
	qt5_wrap_cpp(GENERATED_SOURCES ${MOCS})
elseif(QT_FOUND)
	include(${QT_USE_FILE})
	qt4_wrap_ui(GENERATED_SOURCES ${UIS})
	qt4_wrap_cpp(GENERATED_SOURCES ${MOCS})
endif()

if(TARGET Qt5::Core OR QT_FOUND)
	source_group("Generated Sources - Do Not Edit"
		FILES
		${GENERATED_SOURCES})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	# Build and link the app!
	add_executable(vrpn_hid_gui
		MACOSX_BUNDLE
		${SOURCES}
		${GENERATED_SOURCES}
		${HEADERS}
		${UIS}
		${MOCS}
		${RESOURCES})
	target_link_libraries(vrpn_hid_gui ${QT_LIBRARIES} vrpnserver)
endif()
if(TARGET Qt5::Core)
	qt5_wrap_ui(GENERATED_SOURCES ${UIS})
	qt5_wrap_cpp(GENERATED_SOURCES ${MOCS})
	qt5_use_modules(vrpn_hid_gui Widgets)
elseif(QT_FOUND)
	target_link_libraries(vrpn_hid_gui ${QT_LIBRARIES})
endif()
