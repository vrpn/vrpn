/**
	@file
	@brief Implementation

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "vrpn_Tracker_RazerHydra.h"
#include "quat.h"
#include "vrpn_BufferUtils.h"
#include "vrpn_SendTextMessageStreamProxy.h"

// Library/third-party includes
// - none

// Standard includes
#ifdef sgi
#include <assert.h>
#else
#include <cassert>
#include <sstream>
#endif

#ifdef VRPN_USE_HID

const unsigned int HYDRA_VENDOR = 0x1532;
const unsigned int HYDRA_PRODUCT = 0x0300;
const unsigned int HYDRA_INTERFACE = 0x0;

/// Feature report 0 to set to enter motion controller mode
static const vrpn_uint8 HYDRA_FEATURE_REPORT[] = {
	0x00, // first byte must be report type
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x06, 0x00
};
static const int HYDRA_FEATURE_REPORT_LEN = 91;

/// Feature report 0 to set to enter gamepad mode
static const vrpn_uint8 HYDRA_GAMEPAD_COMMAND[] = {
	0x00, // first byte must be report type
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x00
};
static const int HYDRA_GAMEPAD_COMMAND_LEN = 91;

/// 1 second is as long as we give it to send a first report if it's already
/// in motion-controller mode
static const unsigned long MAXIMUM_INITIAL_WAIT_USEC = 1000000L;

/// 5 seconds is as long as we give it to switch into motion-controller mode
/// after we tell it to.
static const unsigned long MAXIMUM_WAIT_USEC = 5000000L;


static inline unsigned long duration(struct timeval t1, struct timeval t2) {
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

vrpn_Tracker_RazerHydra::vrpn_Tracker_RazerHydra(const char * name, vrpn_Connection * con)
	: vrpn_Analog(name, con)
	, vrpn_Button_Filter(name, con)
	, vrpn_Tracker(name, con)
	, vrpn_HidInterface(new vrpn_HidBooleanAndAcceptor(
	                        new vrpn_HidInterfaceNumberAcceptor(HYDRA_INTERFACE),
	                        new vrpn_HidProductAcceptor(HYDRA_VENDOR, HYDRA_PRODUCT)))
	, status(HYDRA_WAITING_FOR_CONNECT)
	, _wasInGamepadMode(false) /// assume not - if we have to send a command, then set to true
	, _attempt(0) {

	/// Set up sensor counts
	vrpn_Analog::num_channel = 6; /// 3 analog channels from each controller
	vrpn_Button::num_buttons = 16; /// 7 for each controller, starting at a nice number for each
	vrpn_Tracker::num_sensors = 2;

	/// Initialize all data
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

vrpn_Tracker_RazerHydra::~vrpn_Tracker_RazerHydra() {
	if (status == HYDRA_REPORTING && _wasInGamepadMode) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Hydra was in gamepad mode when we started: switching back to gamepad mode.";
		send_feature_report(HYDRA_GAMEPAD_COMMAND_LEN, HYDRA_GAMEPAD_COMMAND);

		send_text_message() << "Waiting 2 seconds for mode change to complete.";
		vrpn_SleepMsecs(2000);
	}
}

void vrpn_Tracker_RazerHydra::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	if (bytes != 52) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Got input report of " << bytes << " bytes, expected 52! Discarding.";
		return;
	}

	if (status != HYDRA_REPORTING) {

		send_text_message(vrpn_TEXT_WARNING)
		        << "Got first motion controller report! This means everything is working properly now. "
		        << "(Took " << _attempt << " attempt" << (_attempt > 1 ? "s" : "") << " to change modes.)";
		status = HYDRA_REPORTING;
	}

	vrpn_gettimeofday(&_timestamp, NULL);
	vrpn_Button::timestamp = _timestamp;
	vrpn_Tracker::timestamp = _timestamp;

	_report_for_sensor(0, buffer + 8);
	_report_for_sensor(1, buffer + 30);

	vrpn_Analog::report_changes(vrpn_CONNECTION_LOW_LATENCY, _timestamp);
	vrpn_Button::report_changes();
}

void vrpn_Tracker_RazerHydra::mainloop() {
	// server update
	vrpn_Analog::server_mainloop();
	vrpn_Button::server_mainloop();
	vrpn_Tracker::server_mainloop();

	if (connected()) {
		// Update state
		if (status == HYDRA_WAITING_FOR_CONNECT) {
			_waiting_for_connect();
		}

		// device update
		update();

		// Check/update listening state during connection/handshaking
		if (status == HYDRA_LISTENING_AFTER_CONNECT) {
			_listening_after_connect();
		} else if (status == HYDRA_LISTENING_AFTER_SET_FEATURE) {
			_listening_after_set_feature();
		}
	}
}

void vrpn_Tracker_RazerHydra::reconnect() {
	status = HYDRA_WAITING_FOR_CONNECT;
	vrpn_HidInterface::reconnect();
}

void vrpn_Tracker_RazerHydra::_waiting_for_connect() {
	assert(status == HYDRA_WAITING_FOR_CONNECT);
	if (connected()) {
		status = HYDRA_LISTENING_AFTER_CONNECT;
		vrpn_gettimeofday(&_connected, NULL);
		send_text_message() << "Listening to see if device is in motion controller mode.";

		/// Reset the mode-change-attempt counter
		_attempt = 0;
		/// We'll assume not in gamepad mode unless we have to tell it to switch
		_wasInGamepadMode = false;
	}
}

void vrpn_Tracker_RazerHydra::_listening_after_connect() {
	assert(status == HYDRA_LISTENING_AFTER_CONNECT);
	assert(connected());
	struct timeval now;
	vrpn_gettimeofday(&now, NULL);
	if (duration(now, _connected) > MAXIMUM_INITIAL_WAIT_USEC) {
		_enter_motion_controller_mode();
	}
}
void vrpn_Tracker_RazerHydra::_listening_after_set_feature() {
	assert(status == HYDRA_LISTENING_AFTER_SET_FEATURE);
	assert(connected());
	struct timeval now;
	vrpn_gettimeofday(&now, NULL);
	if (duration(now, _set_feature) > MAXIMUM_WAIT_USEC) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Really sleepy device - won't start motion controller reports despite our earlier "
		        << _attempt << " attempt" << (_attempt > 1 ? ". " : "s. ")
		        << " Will give it another try.";
#ifdef _WIN32
		send_text_message(vrpn_TEXT_WARNING)
		        << "IMPORTANT (Windows-only): you need the Hydra driver from http://www.razersupport.com/ "
		        << "installed for us to be able to control the device mode. If you haven't installed it, "
		        << "that's why you're getting these errors. Quit this server and install the driver, "
		        << "then try again.";
#else
		send_text_message(vrpn_TEXT_WARNING)
		        << "If this doesn't work, unplug and replug device and restart the VRPN server.";
#endif
		_enter_motion_controller_mode();
	}
}

void vrpn_Tracker_RazerHydra::_enter_motion_controller_mode() {
	assert(status == HYDRA_LISTENING_AFTER_CONNECT || status == HYDRA_LISTENING_AFTER_SET_FEATURE);
	assert(connected());

	_attempt++;
	_wasInGamepadMode = true;

	/** @todo get a feature report as a way of determining current mode

	buf[0] = 0;
	int bytes = get_feature_report(91, buf);
	printf("feature report 0:\n");
	dumpReport(buf, bytes);
	*/

	send_text_message(vrpn_TEXT_WARNING)
	        << "Hydra not in motion-controller mode - attempting to change modes. "
	        << "Please be sure that the left and right sensors are to the left and "
	        << "right sides of the base for automatic calibration to take place.";

	/// Prompt to start streaming motion data
	send_feature_report(HYDRA_FEATURE_REPORT_LEN, HYDRA_FEATURE_REPORT);

	vrpn_uint8 buf[91] = {0};
	buf[0] = 0;
	get_feature_report(91, buf);

	status = HYDRA_LISTENING_AFTER_SET_FEATURE;
	vrpn_gettimeofday(&_set_feature, NULL);
}

void vrpn_Tracker_RazerHydra::_report_for_sensor(int sensorNum, vrpn_uint8 * data) {
	if (!d_connection) {
		return;
	}
	static const double MM_PER_METER = 0.001;
	static const double SCALE_INT16_TO_FLOAT_PLUSMINUS_1 = 1.0 / 32768.0;
	static const double SCALE_UINT8_TO_FLOAT_0_TO_1 = 1.0 / 255.0;
	const int channelOffset = sensorNum * 3;
	const int buttonOffset = sensorNum * 8;

	d_sensor = sensorNum;
	pos[0] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER;
	pos[1] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER;
	pos[2] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER;

	d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	vrpn_uint8 buttonBits = vrpn_unbuffer_from_little_endian<vrpn_uint8>(data);

	/// "middle" button
	buttons[0 + buttonOffset] = buttonBits & 0x20;

	/// Numbered buttons
	buttons[1 + buttonOffset] = buttonBits & 0x04;
	buttons[2 + buttonOffset] = buttonBits & 0x08;
	buttons[3 + buttonOffset] = buttonBits & 0x02;
	buttons[4 + buttonOffset] = buttonBits & 0x10;

	/// "Bumper" button
	buttons[5 + buttonOffset] = buttonBits & 0x01;

	/// Joystick button
	buttons[6 + buttonOffset] = buttonBits & 0x40;

	/// Joystick X, Y
	channel[0 + channelOffset] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	channel[1 + channelOffset] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;

	/// Trigger analog
	channel[2 + channelOffset] = vrpn_unbuffer_from_little_endian<vrpn_uint8>(data) * SCALE_UINT8_TO_FLOAT_0_TO_1;

	char msgbuf[512];
	int len = vrpn_Tracker::encode_to(msgbuf);
	if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf,
	                               vrpn_CONNECTION_LOW_LATENCY)) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra: cannot write message: tossing\n");
	}
}

#endif // VRPN_USE_HID
