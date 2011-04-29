#ifndef VRPN_TRACKER_JSONNET
#define VRPN_TRACKER_JSONNET

#include "vrpn_Configure.h"
#if defined(VRPN_USE_JSONNET)

#include "vrpn_tracker.h"
#include "vrpn_button.h"
#include "vrpn_analog.h"

namespace Json {
	class Reader;
	class Value;
}

/**
 * A tracker class that accepts network updates in JSON format.
 *
 * This tracker is used by the Vrpn Android widgets. 
 * Any other application that can send USD packets with a JSON payload 
 * and feed this tracker.
 * 
 * @Author Philippe Crassous / ENSAM ParisTech-Institut Image
 */
class vrpn_Tracker_JsonNet :
	public vrpn_Tracker, public vrpn_Button, public vrpn_Analog
{
public:
	vrpn_Tracker_JsonNet(
		const char* name,
		vrpn_Connection* c,
		int udpPort
		);
	~vrpn_Tracker_JsonNet(void);

	void mainloop();

	enum {
		TILT_TRACKER_ID = 0,
	};

	
private:
	/*
	 * Network part
	 */
	bool _network_init(int udp_port);
	int _network_receive(void *buffer, int maxlen, int tout_us);
	void _network_release();
	SOCKET _socket;
	enum {
		_NETWORK_BUFFER_SIZE = 2000,

	};
	char _network_buffer[_NETWORK_BUFFER_SIZE];

	/*
	 * Json part
	 */
	bool _parse(const char* buffer, int length);
	bool _parse_tracker_data(const Json::Value& root);
	bool _parse_analog(const Json::Value& root);
	bool _parse_button(const Json::Value& root);
	Json::Reader* _pJsonReader;
};

#endif // ifdef JSONNET
#endif