// vrpn_Analog_5dtUSB.C: VRPN driver for 5dt datagloves over USB (HID)


#include "vrpn_Analog_5dtUSB.h"

#include <iostream>

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


static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRED = 0x0000;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRED = 0x0001;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRELESS_PORTA = 0x0002;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRELESS_PORTA = 0x0003;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_RIGHT_WIRELESS_PORTB = 0x0006;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE14_LEFT_WIRELESS_PORTB = 0x0007;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRED = 0x0010;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRED = 0x0011;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRELESS_PORTA = 0x0012;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRELESS_PORTA = 0x0013;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_RIGHT_WIRELESS_PORTB = 0x0016;
static const vrpn_uint16 vrpn_5DT_DATAGLOVE5_LEFT_WIRELESS_PORTB = 0x0017;


vrpn_Analog_5dtUSB::vrpn_Analog_5dtUSB(vrpn_HidAcceptor *filter,
                                       int num_sensors,
                                       bool isLeftHand,
                                       const char *name,
                                       vrpn_Connection *c) :
	vrpn_Analog(name, c),
	vrpn_HidInterface(filter),
	_filter(filter),
	_isLeftHand(isLeftHand) {
	vrpn_Analog::num_channel = num_sensors;

	// Initialize the state of all the analogs
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

vrpn_Analog_5dtUSB::~vrpn_Analog_5dtUSB() {
	delete _filter;
}

void vrpn_Analog_5dtUSB::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	if (bytes != 64) {
		std::cerr << "Report too short: " << bytes << std::endl;
		return;
	}
	// Decode all full reports.
	const float scale = static_cast<float>(1.0 / 4096.0);
	for (size_t i = 0; i < 16; i++) {
		const char *report = static_cast<char *>(static_cast<void*>(buffer + (i * 2)));
		vrpn_int16 temp;
		vrpn_unbuffer(&report, &temp);
		_rawVals[i] = temp * scale;
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

