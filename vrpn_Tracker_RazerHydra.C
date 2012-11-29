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

#ifdef VRPN_USE_HID
#include "quat.h"                       // for q_vec_type, q_normalize, etc
#include "vrpn_BaseClass.h"
#include "vrpn_BufferUtils.h"
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface, etc
#include "vrpn_OneEuroFilter.h"         // for OneEuroFilterQuat, etc
#include "vrpn_SendTextMessageStreamProxy.h"  // for operator<<, etc

// Library/third-party includes
// - none

// Standard includes
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for char_traits, basic_string, etc
#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, NULL, stderr
#include <string.h>                     // for memset


const unsigned int HYDRA_VENDOR = 0x1532;
const unsigned int HYDRA_PRODUCT = 0x0300;
const unsigned int HYDRA_INTERFACE = 0x0;
const unsigned int HYDRA_CONTROL_INTERFACE = 0x1;

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

static inline double duration_seconds(struct timeval t1, struct timeval t2) {
	return duration(t1, t2) / double(1000000L);
}


class vrpn_Tracker_RazerHydra::MyInterface : public vrpn_HidInterface {
	public:
		MyInterface(unsigned which_interface, vrpn_Tracker_RazerHydra *hydra)
#ifdef __APPLE__
		// The InterfaceNumber is not supported on the mac version -- it
		// is always returned as -1.  So we need to do this based on which
		// device shows up first and hope that it is always the same order.
		// On my mac, the control interface shows up first on iHid, so we
		// try this order.  If we get it wrong, then we swap things out later.
			: vrpn_HidInterface(new vrpn_HidNthMatchAcceptor(which_interface,
#else
			: vrpn_HidInterface(new vrpn_HidBooleanAndAcceptor(
			                        new vrpn_HidInterfaceNumberAcceptor(which_interface),
#endif
			                    new vrpn_HidProductAcceptor(HYDRA_VENDOR, HYDRA_PRODUCT))) {
			d_my_interface = which_interface;
			d_hydra = hydra;
		}

		void on_data_received(size_t bytes, vrpn_uint8 *buffer) {
			if (d_my_interface == HYDRA_CONTROL_INTERFACE) {
#ifdef __APPLE__
				d_hydra->send_text_message(vrpn_TEXT_WARNING)
				        << "Got report on controller channel.  This means that we need to swap channels. "
				        << "Swapping channels.";

				MyInterface *t = d_hydra->_ctrl;
				d_hydra->_ctrl = d_hydra->_data;
				d_hydra->_data = t;
				d_hydra->_ctrl->set_interface(HYDRA_CONTROL_INTERFACE);
				d_hydra->_data->set_interface(HYDRA_INTERFACE);
#else
				fprintf(stderr, "Unexpected receipt of %d bytes on Hydra control interface!\n", static_cast<int>(bytes));
				for (size_t i = 0; i < bytes; ++i) {
					fprintf(stderr, "%x ", buffer[i]);
				}
				fprintf(stderr, "\n");
#endif
			} else {
				if (bytes != 52) {
					d_hydra->send_text_message(vrpn_TEXT_WARNING)
					        << "Got input report of " << bytes << " bytes, expected 52! Discarding, and re-connecting to Hydra."
#ifdef _WIN32
					        << " Please make sure that you have completely quit the Hydra Configurator software and the Hydra system tray icon,"
					        << " since this usually indicates that the Razer software has changed the Hydra's mode behind our back."
#endif
					        ;
					d_hydra->reconnect();
					return;
				}

				if (d_hydra->status != HYDRA_REPORTING) {

					d_hydra->send_text_message(vrpn_TEXT_WARNING)
					        << "Got first motion controller report! This means everything is working properly now. "
					        << "(Took " << d_hydra->_attempt << " attempt" << (d_hydra->_attempt > 1 ? "s" : "") << " to change modes.)";
					d_hydra->status = HYDRA_REPORTING;
				}

				vrpn_gettimeofday(&d_hydra->_timestamp, NULL);
				double dt = duration_seconds(d_hydra->_timestamp, d_hydra->vrpn_Button::timestamp);
				d_hydra->vrpn_Button::timestamp = d_hydra->_timestamp;
				d_hydra->vrpn_Tracker::timestamp = d_hydra->_timestamp;
				d_hydra->_report_for_sensor(0, buffer + 8, dt);
				d_hydra->_report_for_sensor(1, buffer + 30, dt);

				d_hydra->vrpn_Analog::report_changes(vrpn_CONNECTION_LOW_LATENCY, d_hydra->_timestamp);
				d_hydra->vrpn_Button::report_changes();
			}
		}


		std::string getSerialNumber() {
			if (connected()) {
				char buf[256];
				memset(buf, 0, sizeof(buf));
				int bytes = get_feature_report(sizeof(buf) - 1, reinterpret_cast<vrpn_uint8*>(buf));
				if (bytes > 0) {
					return std::string(buf + 216, 17);
				} else {
					return "[FAILED TO GET FEATURE REPORT]";
				}
			} else {
				return "[HYDRA CONTROL INTERFACE NOT CONNECTED]";
			}
		}

		void setMotionControllerMode() {
			/// Prompt to start streaming motion data
			send_feature_report(HYDRA_FEATURE_REPORT_LEN, HYDRA_FEATURE_REPORT);

			vrpn_uint8 buf[91] = {0};
			buf[0] = 0;
			get_feature_report(91, buf);
		}

		void setGamepadMode() {
			/// Prompt to stop streaming motion data
			send_feature_report(HYDRA_GAMEPAD_COMMAND_LEN, HYDRA_GAMEPAD_COMMAND);
		}

		bool connected() {
			return vrpn_HidInterface::connected();
		}

		void update() {
			vrpn_HidInterface::update();
		}

		void set_interface(unsigned which_interface) {
			d_my_interface = which_interface;
		}

	protected:
		unsigned    d_my_interface;
		vrpn_Tracker_RazerHydra *d_hydra;
};

struct vrpn_Tracker_RazerHydra::FilterData {
	FilterData() {
		for (int i = 0; i < vrpn_Tracker_RazerHydra::POSE_CHANNELS; ++i) {
			_filters[i].setMinCutoff(1.150);
			_filters[i].setBeta(.5);
			_filters[i].setDerivativeCutoff(1.2);

			_qfilters[i].setMinCutoff(1.5);
			_qfilters[i].setBeta(.5);
			_qfilters[i].setDerivativeCutoff(1.2);
		}
	}
	OneEuroFilterVec _filters[vrpn_Tracker_RazerHydra::POSE_CHANNELS];
	OneEuroFilterQuat _qfilters[vrpn_Tracker_RazerHydra::POSE_CHANNELS];
};

vrpn_Tracker_RazerHydra::vrpn_Tracker_RazerHydra(const char * name, vrpn_Connection * con)
	: vrpn_Analog(name, con)
	, vrpn_Button_Filter(name, con)
	, vrpn_Tracker(name, con)
	, status(HYDRA_WAITING_FOR_CONNECT)
	, _wasInGamepadMode(false) /// assume not - if we have to send a command, then set to true
	, _attempt(0)
	, _f(new FilterData()) {
	// Set up the control and data channels
	_ctrl = new MyInterface(HYDRA_CONTROL_INTERFACE, this);
	_data = new MyInterface(HYDRA_INTERFACE, this);

	/// Set up sensor counts
	vrpn_Analog::num_channel = ANALOG_CHANNELS; /// 3 analog channels from each controller
	vrpn_Button::num_buttons = BUTTON_CHANNELS; /// 7 for each controller, starting at a nice number for each
	vrpn_Tracker::num_sensors = POSE_CHANNELS;

	/// Initialize all data
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));

	vrpn_gettimeofday(&_timestamp, NULL);

	for (int i = 0; i < vrpn_Tracker::num_sensors; ++i) {
		_calibration_done[i] = false;
		_mirror[i] = 1;
	}
}

vrpn_Tracker_RazerHydra::~vrpn_Tracker_RazerHydra() {
	if (status == HYDRA_REPORTING && _wasInGamepadMode) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Hydra was in gamepad mode when we started: switching back to gamepad mode.";
		_ctrl->setGamepadMode();

		send_text_message() << "Waiting 2 seconds for mode change to complete.";
		vrpn_SleepMsecs(2000);
	}

	delete _ctrl;
	delete _f;
}

void vrpn_Tracker_RazerHydra::mainloop() {
	// server update
	vrpn_Analog::server_mainloop();
	vrpn_Button::server_mainloop();
	vrpn_Tracker::server_mainloop();

	if (_data->connected()) {
		// Update state
		if (status == HYDRA_WAITING_FOR_CONNECT) {
			_waiting_for_connect();
		}

		// device update
		_data->update();
		_ctrl->update();

		// Check/update listening state during connection/handshaking
		if (status == HYDRA_LISTENING_AFTER_CONNECT) {
			_listening_after_connect();
		} else if (status == HYDRA_LISTENING_AFTER_SET_FEATURE) {
			_listening_after_set_feature();
		}
	}
}

bool vrpn_Tracker_RazerHydra::reconnect() {
	status = HYDRA_WAITING_FOR_CONNECT;

	// Reset calibration if we have to reconnect.
	for (int i = 0; i < vrpn_Tracker::num_sensors; ++i) {
		_calibration_done[i] = false;
		_mirror[i] = 1;
	}

	_data->reconnect();
	return _ctrl->reconnect();
}

void vrpn_Tracker_RazerHydra::_waiting_for_connect() {
	if (status != HYDRA_WAITING_FOR_CONNECT) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_waiting_for_connect(): bad status\n");
		return;
	}
	if (_data->connected() && _ctrl->connected()) {
		send_text_message(vrpn_TEXT_WARNING) << "Connected to Razer Hydra with serial number " << _ctrl->getSerialNumber();

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
	if (status != HYDRA_LISTENING_AFTER_CONNECT) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_listening_after_connect(): bad status\n");
		return;
	}
	if (!_data->connected() || !_ctrl->connected()) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_listening_after_connect(): Data or control channel not connected\n");
		return;
	}
	struct timeval now;
	vrpn_gettimeofday(&now, NULL);
	if (duration(now, _connected) > MAXIMUM_INITIAL_WAIT_USEC) {
		_enter_motion_controller_mode();
	}
}

void vrpn_Tracker_RazerHydra::_listening_after_set_feature() {
	if (status != HYDRA_LISTENING_AFTER_SET_FEATURE) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_listening_after_set_feature(): bad status\n");
		return;
	}
	if (!_data->connected() || !_ctrl->connected()) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_listening_after_set_feature(): Data or control channel not connected\n");
		return;
	}
	struct timeval now;
	vrpn_gettimeofday(&now, NULL);
	if (duration(now, _set_feature) > MAXIMUM_WAIT_USEC) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Really sleepy device - won't start motion controller reports despite our earlier "
		        << _attempt << " attempt" << (_attempt > 1 ? ". " : "s. ")
		        << " Will give it another try. "
		        << "If this doesn't work, unplug and replug device and restart the VRPN server.";
#ifdef __APPLE__
		if ((_attempt % 2) == 0) {
			send_text_message(vrpn_TEXT_WARNING)
			        << "Switching control and data interface (mac can't tell the difference).";
			MyInterface *t = _ctrl;
			_ctrl = _data;
			_data = t;
			_ctrl->set_interface(HYDRA_CONTROL_INTERFACE);
			_data->set_interface(HYDRA_INTERFACE);
		}
#endif
		_enter_motion_controller_mode();
	}
}

void vrpn_Tracker_RazerHydra::_enter_motion_controller_mode() {
	if ( (status != HYDRA_LISTENING_AFTER_CONNECT) &&
	     (status != HYDRA_LISTENING_AFTER_SET_FEATURE) ) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_enter_motion_controller_mode(): bad status\n");
		return;
	}
	if (!_data->connected()) {
		fprintf(stderr, "vrpn_Tracker_RazerHydra::_enter_motion_controller_mode(): Control channel not connected\n");
		return;
	}

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
	_ctrl->setMotionControllerMode();

	status = HYDRA_LISTENING_AFTER_SET_FEATURE;
	vrpn_gettimeofday(&_set_feature, NULL);
}

void vrpn_Tracker_RazerHydra::_report_for_sensor(int sensorNum, vrpn_uint8 * data, double dt) {
	if (!d_connection) {
		return;
	}
	static const double MM_PER_METER = 0.001;
	static const double SCALE_INT16_TO_FLOAT_PLUSMINUS_1 = 1.0 / 32768.0;
	static const double SCALE_UINT8_TO_FLOAT_0_TO_1 = 1.0 / 255.0;
	const int channelOffset = sensorNum * 3;
	const int buttonOffset = sensorNum * 8;

	d_sensor = sensorNum;
	/// @todo Why do we sometimes have to invert the y axis to get right-handed behavior?
	pos[0] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER * _mirror[sensorNum];
	pos[1] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER * _mirror[sensorNum];
	pos[2] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * MM_PER_METER * _mirror[sensorNum];

	d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	q_normalize(d_quat, d_quat);
	/*
	if(sensorNum == 0)
	{
		static int count = 0;
		fprintf(stderr, "%d    %3.3f %3.3f %3.3f \n", count++, pos[0], pos[1], pos[2]);
	}
	*/

	// fix hemisphere transitions
	if (_calibration_done[sensorNum]) {
		double dist_direct = (pos[0] - _old_position[sensorNum][0]) * (pos[0] - _old_position[sensorNum][0]) +
		                     (pos[1] - _old_position[sensorNum][1]) * (pos[1] - _old_position[sensorNum][1]) +
		                     (pos[2] - _old_position[sensorNum][2]) * (pos[2] - _old_position[sensorNum][2]);

		double dist_mirror = (-pos[0] - _old_position[sensorNum][0]) * (-pos[0] - _old_position[sensorNum][0]) +
		                     (-pos[1] - _old_position[sensorNum][1]) * (-pos[1] - _old_position[sensorNum][1]) +
		                     (-pos[2] - _old_position[sensorNum][2]) * (-pos[2] - _old_position[sensorNum][2]);


		// too big jump, likely hemisphere switch
		// in that case the coordinates given are mirrored symmetrically across the base
		if (dist_direct - dist_mirror > 10 * Q_EPSILON) {
			pos[0] *= -1;
			pos[1] *= -1;
			pos[2] *= -1;
			_mirror[sensorNum] *= -1;
		}
	} else {
		_calibration_done[sensorNum] = true;

		// first start - assume that the controllers are on the base
		// left one (sensor 0) should have negative x coordinate, right one (sensor 1) positive.
		// if it isn't so, mirror them.

		if ((sensorNum == 0 && pos[0] > 0) ||
		        (sensorNum == 1 && pos[0] < 0)) {
			pos[0] *= -1;
			pos[1] *= -1;
			pos[2] *= -1;
			_mirror[sensorNum] = -1;
		}
	}
	q_vec_copy(_old_position[sensorNum], pos);

	vrpn_uint8 buttonBits = vrpn_unbuffer_from_little_endian<vrpn_uint8>(data);

	// jitter filtering
	const vrpn_float64 *filtered = _f->_filters[sensorNum].filter(dt, pos);

	q_vec_copy(pos, filtered);

	const double *q_filtered = _f->_qfilters[sensorNum].filter(dt, d_quat);
	q_normalize(d_quat, q_filtered);

	/// "middle" button
	buttons[0 + buttonOffset] = (buttonBits & 0x20) != 0;

	/// Numbered buttons
	buttons[1 + buttonOffset] = (buttonBits & 0x04) != 0;
	buttons[2 + buttonOffset] = (buttonBits & 0x08) != 0;
	buttons[3 + buttonOffset] = (buttonBits & 0x02) != 0;
	buttons[4 + buttonOffset] = (buttonBits & 0x10) != 0;

	/// "Bumper" button
	buttons[5 + buttonOffset] = (buttonBits & 0x01) != 0;

	/// Joystick button
	buttons[6 + buttonOffset] = (buttonBits & 0x40) != 0;

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
