#include "vrpn_Tracker_JsonNet.h"

#if defined(VRPN_USE_JSONNET)

#ifdef _WIN32
  #ifdef VRPN_USE_WINSOCK2
    #include <winsock2.h>    // struct timeval is defined here
  #else
    #include <winsock.h>    // struct timeval is defined here
  #endif
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#define INVALID_SOCKET -1
#endif

#include "json/json.h"

#include "quat.h"

#include "vrpn_SendTextMessageStreamProxy.h"

#include <stdlib.h> // for exit

// These must match definitions in eu.ensam.ii.vrpn.Vrpn
static const char* const MSG_KEY_TYPE =				"type";
//static const char* const MSG_KEY_SEQUENCE_NUMBER =	"sn";
//static const char* const MSG_KEY_TIMESTAMP =		"ts";
	
static const char* const MSG_KEY_TRACKER_ID =		"id";
static const char* const MSG_KEY_TRACKER_QUAT =		"quat";
static const char* const MSG_KEY_TRACKER_POS =		"pos";

static const char* const MSG_KEY_BUTTON_ID =		"button";
static const char* const MSG_KEY_BUTTON_STATUS =	"state";
	
static const char* const MSG_KEY_ANALOG_CHANNEL =	"num";
static const char* const MSG_KEY_ANALOG_DATA =		"data";

static const char* const MSG_KEY_TEXT_DATA =		"data";

// Message types (values for MSG_KEY_TYPE)
static const int MSG_TYPE_TRACKER = 1;
static const int MSG_TYPE_BUTTON = 2;
static const int MSG_TYPE_ANALOG = 3;
static const int MSG_TYPE_TEXT = 4;

vrpn_Tracker_JsonNet::vrpn_Tracker_JsonNet(const char* name,vrpn_Connection* c,int udp_port) :
	vrpn_Tracker(name, c),
	vrpn_Button_Filter(name, c),
	vrpn_Analog(name, c),
	vrpn_Text_Sender(name, c),
	_socket(INVALID_SOCKET),
	_do_tracker_report(false),
	_pJsonReader(0)
{
	fprintf(stderr, "vrpn_Tracker_JsonNet : Device %s listen on port udp port %d\n", name, udp_port);
	if (! _network_init(udp_port)) {
		exit(EXIT_FAILURE);
	}

	// Buttons part

	num_buttons = vrpn_BUTTON_MAX_BUTTONS;
	num_channel = vrpn_CHANNEL_MAX;

	_pJsonReader = new Json::Reader();
}

vrpn_Tracker_JsonNet::~vrpn_Tracker_JsonNet(void)
{
	if (_pJsonReader != 0) {
                try {
                  delete _pJsonReader;
                } catch (...) {
                  fprintf(stderr, "vrpn_Tracker_JsonNet::~vrpn_Tracker_JsonNet(): delete failed\n");
                  return;
                }
		_pJsonReader = 0;
	}
	_network_release();
}


void vrpn_Tracker_JsonNet::mainloop()
{
	server_mainloop();
	/*
	 * The original Dtrack code uses blocking call to select() in _network_receive with 
	 * a 1 sec timeout. In Dtrack, the data is supposed to be continuously flowing (app. 60 Hz), 
	 * so the timeout is unlikely to happen. However, the data from the Android device flow at a lower
	 * frequency and may not flow at all if the tilt tracker is disabled. 
	 * Thus a 1 sec timeout here causes latency and jerky movements in Dtrack 
	 */
	const int timeout_us = 10 * 1000;
	int received_length = _network_receive(_network_buffer, _NETWORK_BUFFER_SIZE, timeout_us);

	if (received_length < 0) {
		//fprintf(stderr, "vrpn_Tracker_JsonNet : receive error %d\n", received_length);
		return;
	} 
	_network_buffer[received_length] = '\0';
	//fprintf(stderr, "got data : %.*s\n", received_length, _network_buffer);
	if (!_parse(_network_buffer, received_length)) {
		// whatever error

		return;
	}

	// report trackerchanges
	// TODO really use timestamps
	struct timeval ts ;
	vrpn_gettimeofday(&ts, NULL);
	// from vrpn_Tracker_DTrack::dtrack2vrpnbody
	if (d_connection && _do_tracker_report) {
		char msgbuf[1000];
		// Encode pos and d_quat
		int len = vrpn_Tracker::encode_to(msgbuf);
		if (d_connection->pack_message(len, ts, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			// error
		}
		_do_tracker_report = false;
		//fprintf(stderr, "Packed and sent\n");
	}

	vrpn_Button::report_changes();
	vrpn_Analog::report_changes();
}

bool vrpn_Tracker_JsonNet::_parse(const char* buffer, int /*length*/)
{
	Json::Value root;							// will contains the root value after parsing.
	// Beware collectcomment = true crashes
	bool parsingSuccessful = _pJsonReader->parse( buffer, root , false);
	if ( !parsingSuccessful ) {
		// report to the user the failure and their locations in the document.
		fprintf(stderr, "vrpn_Tracker_JsonNet parse error :%s\n",
						 _pJsonReader->getFormatedErrorMessages().c_str());
		fprintf(stderr, "%s\n",buffer);
		return false;
	}

	const Json::Value& constRoot = root;
	// Find MessageType
	const Json::Value& type = constRoot[MSG_KEY_TYPE]; 
	int messageType;
	if (!type.empty() && type.isConvertibleTo(Json::intValue)) {
		messageType = type.asInt();
		// HACK
	} else {
		fprintf(stderr, "vrpn_Tracker_JsonNet parse error : missing message type\n");
		return false;
	}
	switch (messageType) {
		// TODO cleanup
		case MSG_TYPE_TRACKER:
			return _parse_tracker_data(root);
			break;
		case MSG_TYPE_BUTTON:
			return _parse_button(root);
			break;
		case MSG_TYPE_ANALOG:
			return _parse_analog(root);
			break;
		case MSG_TYPE_TEXT:
			return _parse_text(root);
			break;
		default:
			break;
	}
	return false;
}

/**
 * Parse a tracker update mesage.
 * 
 * If the message can be parsed and the tracker Id is valid, the tracker data is updated.
 *
 * @param root the JSON message
 * @returns false if any error, true otherwise.
 */
bool vrpn_Tracker_JsonNet::_parse_tracker_data(const Json::Value& root)
{
	const Json::Value& constRoot = root;

	// Id of the current tracker
	const Json::Value& sensorId = constRoot[MSG_KEY_TRACKER_ID];
	if (!sensorId.empty() && sensorId.isConvertibleTo(Json::intValue)){
		this->d_sensor = sensorId.asInt();
	} else {
		return false;
	}

	/*
	 * mainloop calls vrpn_Tracker::encode_to, that will send d_sensor, d_pos and d_quat.
	 * velocity and acceleration are curretly not handled
	 */
	 
	const Json::Value& quatData = constRoot[MSG_KEY_TRACKER_QUAT];
	if (!quatData.empty() && quatData.isArray() && quatData.size() == 4) {
		this->d_quat[0] = quatData[0u].asDouble();
		this->d_quat[1] = quatData[1].asDouble();
		this->d_quat[2] = quatData[2].asDouble();
		this->d_quat[3] = quatData[3].asDouble();
		//q_vec_type ypr;
		//q_to_euler(ypr, d_quat);
		//fprintf(stderr, "yaw-rY %.3f pitch-rX %.3f roll-rZ %.3f \n", ypr[0], ypr[1], ypr[2]);
	} 

	/*
	 * Look for a position 
	 */
	const Json::Value& posData = constRoot[MSG_KEY_TRACKER_POS];
	if (!posData.empty() && posData.isArray() &&  posData.size() == 3) {
		this->pos[0] = posData[0u].asDouble();
		this->pos[1]= posData[1].asDouble();
		this->pos[2]= posData[2].asDouble();
	} 

	_do_tracker_report = true;
	return true;
}

/**
 * Parse a text update mesage.
 *
 * If the message can be parsed the message data is sent.
 *
 * @param root the JSON message
 * @returns false if any error, true otherwise.
 */
bool vrpn_Tracker_JsonNet::_parse_text(const Json::Value& root)
{
	const Json::Value& valueTextStatus = root[MSG_KEY_TEXT_DATA];
	if (!valueTextStatus.empty() && valueTextStatus.isConvertibleTo(Json::stringValue)) {
		send_text_message(vrpn_TEXT_NORMAL) << valueTextStatus.asString();
		return true;
	}
	fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_text parse error : missing text");
	return false;
}
/**
 * Parse a button update mesage.
 * 
 * If the message can be parses and the button Id is valid, the button data is updated.
 *
 * @param root the JSON message
 * @returns false if any error, true otherwise.
 */
bool vrpn_Tracker_JsonNet::_parse_button(const Json::Value& root)
{
	const Json::Value& valueButtonStatus = root[MSG_KEY_BUTTON_STATUS]; 
	bool buttonStatus;
	if (!valueButtonStatus.empty() && valueButtonStatus.isConvertibleTo(Json::booleanValue)) {
		buttonStatus = valueButtonStatus.asBool();
	} else {
		fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_button parse error : missing status");
		return false;
	}
	const Json::Value& valueButtonId = root[MSG_KEY_BUTTON_ID]; 
	int buttonId;		// buttonId embedded in the message. 
	if (!valueButtonId.empty() && valueButtonId.isConvertibleTo(Json::intValue)) {
		buttonId = valueButtonId.asInt();
	} else {
		fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_button parse error : missing id\n");
		return false;
	}

	if (buttonId < 0 || buttonId > num_buttons) {
		fprintf(stderr, "invalid button Id %d (max : %d)\n", buttonId, num_buttons);
	} else {
		buttons[buttonId] = (int)buttonStatus;
	}

	return true;
}

/**
 * Parse an analog update mesage.
 * 
 * If the message can be parsed and the analog Id is valid, the analog data is updated.
 *
 * @param root the JSON message
 * @returns false if any error, true otherwise.
 */

bool vrpn_Tracker_JsonNet::_parse_analog(const Json::Value& root)
{
	const Json::Value& valueData = root[MSG_KEY_ANALOG_DATA]; 
	double data;
	if (!valueData.empty() && valueData.isConvertibleTo(Json::realValue)) {
		data = valueData.asDouble();
	} else {
		fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_analog parse error : missing status");
		return false;
	}

	const Json::Value& channelNumberId = root[MSG_KEY_ANALOG_CHANNEL]; 
	int channelNumber;
	if (!channelNumberId.empty() && channelNumberId.isConvertibleTo(Json::intValue)) {
		channelNumber = channelNumberId.asInt();
	} else {
		fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_analog parse error : missing id\n");
		return false;
	}
	
	if (channelNumber < 0 || channelNumber >= num_channel) {
		fprintf(stderr, "vrpn_Tracker_JsonNet::_parse_analog id out of bounds %d/%d\n", channelNumber, num_channel);
	} else {
		channel[channelNumber] = data;
	}

	return true;
}

/**
 * Initialises the listening socket
 * @param udp_port the UDP listening port  
 */
bool vrpn_Tracker_JsonNet::_network_init(int udp_port)
{
	int iResult;
#ifdef _WIN32
	{
		// Initialize Winsock
		WORD versionRequested =  MAKEWORD(2,2);
		WSADATA wsaData;

		iResult = WSAStartup(versionRequested, &wsaData);
		if (iResult != 0) {
		    printf("WSAStartup failed with error: %d\n", iResult);
		    return false;
		}
    }
#endif

#ifdef _WIN32
	{
		// Create a SOCKET for connecting to server
		_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (_socket == INVALID_SOCKET) {
		    printf("socket failed with error: %ld\n", WSAGetLastError());
		    //freeaddrinfo(result);
		    WSACleanup();
		    return false;
		}
	}
#else
	{
		int usock;
		
		usock = socket(PF_INET, SOCK_DGRAM, 0);
	
		if (usock < 0){
			return false;
		}
		_socket = usock;
	}
#endif
	struct sockaddr_in localSocketAddress;
        memset((void *)&localSocketAddress, 0, sizeof(localSocketAddress));
	localSocketAddress.sin_family = AF_INET;
	localSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localSocketAddress.sin_port = htons(udp_port);

    // Setup the listening socket
	iResult = bind( _socket, (struct sockaddr*)&localSocketAddress, sizeof(localSocketAddress));
    if (iResult < 0) {
#ifdef _WIN32
        printf("bind failed with error: %d\n", WSAGetLastError());
#else
        printf("bind failed.");
#endif
        //freeaddrinfo(result);
		_network_release();
        return false;
    }

    //freeaddrinfo(result);
	return true;

}
 
/**
 * Read a network message.
 *
 * Same as vrpn_tracker_DTrack::udp_receive
 *
 * @param buffer the address of the data buffer to be updated
 * @param maxlen the length of the buffer
 * @param tout_us the read timeout in microseconds
 */
int vrpn_Tracker_JsonNet::_network_receive(void *buffer, int maxlen, int tout_us)
{
	int nbytes, err;
	fd_set set;
	struct timeval tout;

	// waiting for data:

	FD_ZERO(&set);
	FD_SET(_socket, &set);

	tout.tv_sec = tout_us / 1000000;
	tout.tv_usec = tout_us % 1000000;

	switch((err = select(FD_SETSIZE, &set, NULL, NULL, &tout))){
		case 1:
			break;        // data available
		case 0:
			//fprintf(stderr, "net_receive: select timeout (err = 0)\n");
			return -1;    // timeout
			break;
		default:
			//fprintf(stderr, "net_receive: select error %d\n", err);
			return -2;    // error

	}

	// receiving packet:
	while(1){

		// receive one packet:
		nbytes = recv(_socket, (char *)buffer, maxlen, 0);
		if(nbytes < 0){  // receive error
			//fprintf(stderr, "recv_receive: select error %d\n", err);
			return -3;
		}

		// check, if more data available: if so, receive another packet
		FD_ZERO(&set);
		FD_SET(_socket, &set);

		tout.tv_sec = 0;   // timeout with value of zero, thus no waiting
		tout.tv_usec = 0;

		if(select(FD_SETSIZE, &set, NULL, NULL, &tout) != 1){
			// no more data available: check length of received packet and return
			if(nbytes >= maxlen){   // buffer overflow
      			return -4;
		   }
		return nbytes;
		}
	}
}

/**
 * Cleanup the network resources
 */
void vrpn_Tracker_JsonNet::_network_release()
{
#ifdef _WIN32
	closesocket(_socket);
	WSACleanup();
#else
	close(_socket);
#endif
}

#endif // defined VRPN_USE_JSONNET

