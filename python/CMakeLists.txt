
if(PYTHONLIBS_FOUND AND (VRPN_BUILD_PYTHON_HANDCODED_2X OR VRPN_BUILD_PYTHON_HANDCODED_3X))
	include_directories(${PYTHON_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}")

	set(PYTHONVERSIONDIR python)
	if(PYTHON_LIBRARY MATCHES "python([23].[0-9])")
		set(PYTHONVERSIONDIR "python${CMAKE_MATCH_1}/")
	endif()

	if(WIN32)
		add_definitions(-DCALLBACK_CALL=__stdcall)
	else()
		add_definitions(-DCALLBACK_CALL=)
	endif()
	python_add_module(vrpn-python
		Base.cpp
		Connection.cpp
		Device.cpp
		callback.cpp
		include/Base.hpp
		include/Connection.hpp
		include/Device.hpp
		include/callback.hpp
		include/definition.hpp
		include/exceptions.hpp
		include/handlers.hpp
		include/interface.hpp
		include/tools.hpp
		interface.cpp
		quaternion/quaternion.cpp
		receiver/Analog.cpp
		receiver/Button.cpp
		receiver/Dial.cpp
		receiver/Text_Receiver.cpp
		receiver/Tracker.cpp
		receiver/include/Analog.hpp
		receiver/include/Button.hpp
		receiver/include/Dial.hpp
		receiver/include/Text_Receiver.hpp
		receiver/include/Tracker.hpp
		receiver/receiver.cpp
		sender/Poser.cpp
		sender/Text_Sender.cpp
		sender/include/Poser.hpp
		sender/include/Text_Sender.hpp
		sender/sender.cpp
		tools.cpp)
	target_link_libraries(vrpn-python ${PYTHON_LIBRARIES} vrpnserver)
	set_target_properties(vrpn-python
		PROPERTIES
		FOLDER "Python Bindings"
		OUTPUT_NAME vrpn)

	if(VRPN_INSTALL)
		if(WIN32)
#			install(TARGETS vrpn-python
#				RUNTIME DESTINATION lib/${PYTHONVERSIONDIR}dist-packages/ COMPONENT python
#				LIBRARY DESTINATION lib/${PYTHONVERSIONDIR}dist-packages/ COMPONENT python)
		else()
			install(TARGETS vrpn-python
				RUNTIME DESTINATION lib/${PYTHONVERSIONDIR}dist-packages/ COMPONENT python
				LIBRARY DESTINATION lib/${PYTHONVERSIONDIR}dist-packages/ COMPONENT python)
		endif()
	endif()
endif()
