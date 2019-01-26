/** @file	vrpn_Analog_5dtUSB.C
	@brief	Implemendation of 5DT USB (HID) dataglove driver

	@date	2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

#include <string.h>                     // for memset
#include <iostream>                     // for operator<<, ostringstream, etc
#include <sstream>
#include <stdexcept>                    // for logic_error

#include "vrpn_Analog_5dtUSB.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_NORMAL, etc

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_5DT_VENDOR = 0x5d70;

static const vrpn_uint16 vrpn_5DT_LEFT_MASK = 0x0001;
static const vrpn_uint16 vrpn_5DT_RIGHT = 0x0000;
static const vrpn_uint16 vrpn_5DT_LEFT = 0x0001;
static const vrpn_uint16 vrpn_5DT_WIRELESS_PORTA_MASK = 0x0002;
static const vrpn_uint16 vrpn_5DT_WIRELESS_PORTB_MASK = 0x0006;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_MASK = 0x0010;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5 = 0x0010;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14 = 0x0000;


//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRED = 0x0000;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRED = 0x0001;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRELESS_PORTA = 0x0002;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRELESS_PORTA = 0x0003;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRELESS_PORTB = 0x0006;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRELESS_PORTB = 0x0007;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRED = 0x0010;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRED = 0x0011;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRELESS_PORTA = 0x0012;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRELESS_PORTA = 0x0013;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRELESS_PORTB = 0x0016;
//static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRELESS_PORTB = 0x0017;


vrpn_Analog_5dtUSB::vrpn_Analog_5dtUSB(vrpn_HidAcceptor *filter,
                                       int num_sensors,
                                       bool isLeftHand,
                                       const char *name,
                                       vrpn_Connection *c) :
	vrpn_Analog(name, c),
	vrpn_HidInterface(filter),
	_isLeftHand(isLeftHand),
	_wasConnected(false) {

	if (num_sensors != 5 && num_sensors != 14) {
		throw std::logic_error("The vrpn_Analog_5dtUSB driver only supports 5 or 14 sensors, and a different number was passed!");
	}

	vrpn_Analog::num_channel = num_sensors;

	// Initialize the state of all the analogs
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));

	// Set the timestamp
	vrpn_gettimeofday(&_timestamp, NULL);
}

vrpn_Analog_5dtUSB::~vrpn_Analog_5dtUSB() {
  try {
    delete m_acceptor;
  } catch (...) {
    fprintf(stderr, "vrpn_Analog_5dtUSB::~vrpn_Analog_5dtUSB(): delete failed\n");
    return;
  }
}
std::string vrpn_Analog_5dtUSB::get_description() const {
	std::ostringstream ss;
	if (product() & vrpn_5DT_LEFT_MASK) {
		ss << "left";
	} else {
		ss << "right";
	}

	ss << " hand, ";
	if (product() & vrpn_5DT_DATAGLOVE5_MASK) {
		ss << "5";
	} else {
		ss << "14";
	}

	ss << " sensors, ";
	if (product() & vrpn_5DT_WIRELESS_PORTA_MASK) {
		ss << "wireless port A";
	} else if (product() & vrpn_5DT_WIRELESS_PORTB_MASK) {
		ss << "wireless port B";
	} else {
		ss << "wired USB";
	}
	return ss.str();
}

void vrpn_Analog_5dtUSB::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	if (bytes != 64) {
		std::ostringstream ss;
		ss << "Received a too-short report: " << bytes;
		struct timeval ts;
		vrpn_gettimeofday(&ts, NULL);
		send_text_message(ss.str().c_str(), ts, vrpn_TEXT_WARNING);
		return;
	}

	// Decode all full reports.
	const float scale = 1.0f / 4096.0f;
	vrpn_uint8 * bufptr = buffer;
	for (size_t i = 0; i < 16; i++) {
		_rawVals[i] = vrpn_unbuffer<vrpn_int16>(bufptr) * scale;
	}

	switch (vrpn_Analog::num_channel) {
		case 5:
			for (size_t i = 0; i < 5; ++i) {
				channel[i] = _rawVals[i * 3];
				// Report this event before parsing the next.
				report_changes();
			}
			break;

		case 14:
			for (size_t i = 0; i < 14; ++i) {
				channel[i] = _rawVals[i];
				// Report this event before parsing the next.
				report_changes();
			}
			break;
		default:
			std::cerr << "Internal error - should not happen: Unrecognized number of channels!" << std::endl;
	}
}

void vrpn_Analog_5dtUSB::mainloop() {
	vrpn_gettimeofday(&_timestamp, NULL);

	update();

	if (connected() && !_wasConnected) {
		std::ostringstream ss;
		ss << "Successfully connected to 5DT glove, " << get_description();
        send_text_message(ss.str().c_str(), _timestamp, vrpn_TEXT_NORMAL);
	}
	_wasConnected = connected();

	server_mainloop();
}

void vrpn_Analog_5dtUSB::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
}

void vrpn_Analog_5dtUSB::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
}


vrpn_Analog_5dtUSB_Glove5Left::vrpn_Analog_5dtUSB_Glove5Left(const char *name, vrpn_Connection *c) :
	vrpn_Analog_5dtUSB(new vrpn_HidProductMaskAcceptor(vrpn_5DT_VENDOR, vrpn_5DT_LEFT_MASK | vrpn_5DT_DATAGLOVE5_MASK, vrpn_5DT_LEFT | vrpn_5DT_DATAGLOVE5),
	                   5,
	                   true,
	                   name,
	                   c) { }

vrpn_Analog_5dtUSB_Glove5Right::vrpn_Analog_5dtUSB_Glove5Right(const char *name, vrpn_Connection *c) :
	vrpn_Analog_5dtUSB(new vrpn_HidProductMaskAcceptor(vrpn_5DT_VENDOR, vrpn_5DT_LEFT_MASK | vrpn_5DT_DATAGLOVE5_MASK, vrpn_5DT_RIGHT | vrpn_5DT_DATAGLOVE5),
	                   5,
	                   false,
	                   name,
	                   c) { }

vrpn_Analog_5dtUSB_Glove14Left::vrpn_Analog_5dtUSB_Glove14Left(const char *name, vrpn_Connection *c) :
	vrpn_Analog_5dtUSB(new vrpn_HidProductMaskAcceptor(vrpn_5DT_VENDOR, vrpn_5DT_LEFT_MASK | vrpn_5DT_DATAGLOVE5_MASK, vrpn_5DT_LEFT | vrpn_5DT_DATAGLOVE14),
	                   14,
	                   true,
	                   name,
	                   c) { }

vrpn_Analog_5dtUSB_Glove14Right::vrpn_Analog_5dtUSB_Glove14Right(const char *name, vrpn_Connection *c) :
	vrpn_Analog_5dtUSB(new vrpn_HidProductMaskAcceptor(vrpn_5DT_VENDOR, vrpn_5DT_LEFT_MASK | vrpn_5DT_DATAGLOVE5_MASK, vrpn_5DT_RIGHT | vrpn_5DT_DATAGLOVE14),
	                   14,
	                   false,
	                   name,
	                   c) { }

#endif

