
add_executable(vrpn_streamPrint vrpn_streamPrint.cpp)
target_link_libraries(vrpn_streamPrint ${VRPN_CLIENT_LIBRARY})

set_target_properties(vrpn_streamPrint PROPERTIES FOLDER Clients)

if(VRPN_INSTALL)
	install(TARGETS vrpn_streamPrint RUNTIME DESTINATION bin COMPONENT clients)
endif()
