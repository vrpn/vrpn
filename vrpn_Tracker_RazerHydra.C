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

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// "One Euro" filter for reducing jitter
// http://hal.inria.fr/hal-00670496/

class LowPassFilterVec
{
public:
	LowPassFilterVec() : _firstTime(true) 
	{
	}

	const vrpn_float64 *filter(const vrpn_float64 *x, vrpn_float64 alpha)
	{
		if(_firstTime)
		{
			_firstTime = false;
			memcpy(_hatxprev, x, sizeof(vrpn_float64) * 3);
		}

		vrpn_float64 hatx[3];
		hatx[0] = alpha * x[0] + (1 - alpha) * _hatxprev[0];
		hatx[1] = alpha * x[1] + (1 - alpha) * _hatxprev[1];
		hatx[2] = alpha * x[2] + (1 - alpha) * _hatxprev[2];

		memcpy(_hatxprev, hatx, sizeof(vrpn_float64) * 3);
		return _hatxprev;
	}

	const vrpn_float64 *hatxprev() { return _hatxprev; }

private:
	bool _firstTime;
	vrpn_float64 _hatxprev[3];
};

class OneEuroFilterVec
{
public:
	OneEuroFilterVec(vrpn_float64 rate, vrpn_float64 mincutoff, vrpn_float64 beta, LowPassFilterVec &xfilt, vrpn_float64 dcutoff, LowPassFilterVec &dxfilt) :
		_firstTime(true), _xfilt(xfilt), _dxfilt(dxfilt),
		_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff), _rate(rate) {};

	const vrpn_float64 *filter(const vrpn_float64 *x)
	{
		vrpn_float64 dx[3];

		if(_firstTime)
		{
			_firstTime = false;
			dx[0] = dx[1] = dx[2] = 0.0f;

		} else {
			const vrpn_float64 *filtered_prev = _xfilt.hatxprev();
			dx[0] = (x[0] - filtered_prev[0]) * _rate;
			dx[1] = (x[1] - filtered_prev[1]) * _rate;
			dx[2] = (x[2] - filtered_prev[2]) * _rate;
		}

		const vrpn_float64 *edx = _dxfilt.filter(dx, alpha(_rate, _dcutoff));
		vrpn_float64 norm = sqrt(edx[0]*edx[0] + edx[1]*edx[1] + edx[2]*edx[2]);
		vrpn_float64 cutoff = _mincutoff + _beta * norm;

		return _xfilt.filter(x, alpha(_rate, cutoff));
	}

private:
	vrpn_float64 alpha(vrpn_float64 rate, vrpn_float64 cutoff)
	{
		vrpn_float64 tau = (vrpn_float64)(1.0f/(2.0f * M_PI * cutoff));
		vrpn_float64 te  = 1.0f/rate;
		return 1.0f/(1.0f + tau/te);
	}

	bool _firstTime;
	vrpn_float64 _rate;
	vrpn_float64 _mincutoff, _dcutoff;
	vrpn_float64 _beta;
	LowPassFilterVec &_xfilt, &_dxfilt;
};

class LowPassFilterQuat
{
public:
	LowPassFilterQuat() : _firstTime(true) 
	{
	}

	const double *filter(const q_type x, vrpn_float64 alpha)
	{
		if(_firstTime)
		{
			_firstTime = false;
			q_copy(_hatxprev, x);
		}

		q_type hatx;
		q_slerp(hatx, _hatxprev, x, alpha);
		q_copy(_hatxprev, hatx);
		return _hatxprev;
	}

	const double *hatxprev() { return _hatxprev; }

private:
	bool _firstTime;
	q_type _hatxprev;
};

class OneEuroFilterQuat
{
public:
	OneEuroFilterQuat(vrpn_float64 rate, vrpn_float64 mincutoff, vrpn_float64 beta, LowPassFilterQuat &xfilt, vrpn_float64 dcutoff, LowPassFilterQuat &dxfilt) :
		_firstTime(true), _xfilt(xfilt), _dxfilt(dxfilt),
		_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff), _rate(rate) {};

	const double *filter(const q_type x)
	{
		q_type dx;

		if(_firstTime)
		{
			_firstTime = false;
			dx[0] = dx[1] = dx[2] = 0;
			dx[3] = 1;

		} else {
			q_type filtered_prev;
			q_copy(filtered_prev, _xfilt.hatxprev());

			q_type inverse_prev;
			q_invert(inverse_prev, filtered_prev);
			q_mult(dx, x, inverse_prev);
		}

		q_type edx;
		q_copy(edx, _dxfilt.filter(dx, alpha(_rate, _dcutoff)));
		
		// avoid taking acos of an invalid number due to numerical errors
		if(edx[Q_W] > 1.0) edx[Q_W] = 1.0;
		if(edx[Q_W] < -1.0) edx[Q_W] = -1.0;		

		double ax,ay,az,angle;
		q_to_axis_angle(&ax, &ay, &az, &angle, edx);
		
		vrpn_float64 cutoff = _mincutoff + _beta * angle;

		const double *result = _xfilt.filter(x, alpha(_rate, cutoff));

		return result;
	}

private:
	vrpn_float64 alpha(vrpn_float64 rate, vrpn_float64 cutoff)
	{
		vrpn_float64 tau = (vrpn_float64)(1.0f/(2.0f * M_PI * cutoff));
		vrpn_float64 te  = 1.0f/rate;
		return 1.0f/(1.0f + tau/te);
	}

	bool _firstTime;
	vrpn_float64 _rate;
	vrpn_float64 _mincutoff, _dcutoff;
	vrpn_float64 _beta;
	LowPassFilterQuat &_xfilt, &_dxfilt;
};


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

	_old_position = new vrpn_float64[vrpn_Tracker::num_sensors * 3];
	memset(_old_position, 0, sizeof(vrpn_float64) * vrpn_Tracker::num_sensors * 3);

	_calibration_done = new bool[vrpn_Tracker::num_sensors];

    _xfilters = new LowPassFilterVec *[vrpn_Tracker::num_sensors];
    _dxfilters = new LowPassFilterVec *[vrpn_Tracker::num_sensors];
    _filters = new OneEuroFilterVec *[vrpn_Tracker::num_sensors];

    _qxfilters = new LowPassFilterQuat *[vrpn_Tracker::num_sensors];
    _qdxfilters = new LowPassFilterQuat *[vrpn_Tracker::num_sensors];
    _qfilters = new OneEuroFilterQuat *[vrpn_Tracker::num_sensors];

	for(int i = 0; i < vrpn_Tracker::num_sensors; ++i) 
	{
		_calibration_done[i] = false;
		
		LowPassFilterVec *xfilt = new LowPassFilterVec;
		LowPassFilterVec *dxfilt = new LowPassFilterVec;

		_xfilters[i] = xfilt;
		_dxfilters[i] = dxfilt;
		_filters[i] = new OneEuroFilterVec(60, 0.01f, 0.5f, *xfilt, 0.1f, *dxfilt);

		LowPassFilterQuat *qxfilt = new LowPassFilterQuat;
		LowPassFilterQuat *qdxfilt = new LowPassFilterQuat;

		_qxfilters[i] = qxfilt;
		_qdxfilters[i] = qdxfilt;
		_qfilters[i] = new OneEuroFilterQuat(60, 0.4f, 0.5f, *qxfilt, 0.9f, *qdxfilt);
	}
}

vrpn_Tracker_RazerHydra::~vrpn_Tracker_RazerHydra() {
	if (status == HYDRA_REPORTING && _wasInGamepadMode) {
		send_text_message(vrpn_TEXT_WARNING)
		        << "Hydra was in gamepad mode when we started: switching back to gamepad mode.";
		send_feature_report(HYDRA_GAMEPAD_COMMAND_LEN, HYDRA_GAMEPAD_COMMAND);

		send_text_message() << "Waiting 2 seconds for mode change to complete.";
		vrpn_SleepMsecs(2000);
	}

	delete[] _old_position;
	delete[] _calibration_done;

	for(int i = 0; i < vrpn_Tracker::num_sensors; ++i) 
	{
		delete _xfilters[i];
		delete _dxfilters[i];
		delete _filters[i];

		delete _qxfilters[i];
		delete _qdxfilters[i];
		delete _qfilters[i];
	}

    delete[] _xfilters;
    delete[] _dxfilters;
    delete[] _filters;

    delete[] _qxfilters;
    delete[] _qdxfilters;
    delete[] _qfilters;
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

	/*
	if(sensorNum == 0)
	{
		static int count = 0;
		fprintf(stderr, "%d    %3.3f %3.3f %3.3f \n", count++, pos[0], pos[1], pos[2]);
	}
	*/

	// fix hemisphere transitions
	if(_calibration_done[sensorNum])
	{
		double dist_direct = (pos[0] - _old_position[sensorNum * 3]) * (pos[0] - _old_position[sensorNum * 3]) + 
                             (pos[1] - _old_position[sensorNum * 3 + 1]) * (pos[1] - _old_position[sensorNum * 3 + 1]) + 
						     (pos[2] - _old_position[sensorNum * 3 + 2]) * (pos[2] - _old_position[sensorNum * 3 + 2]);

   		double dist_mirror = (-pos[0] - _old_position[sensorNum * 3]) * (-pos[0] - _old_position[sensorNum * 3]) + 
                             (-pos[1] - _old_position[sensorNum * 3 + 1]) * (-pos[1] - _old_position[sensorNum * 3 + 1]) + 
						     (-pos[2] - _old_position[sensorNum * 3 + 2]) * (-pos[2] - _old_position[sensorNum * 3 + 2]);

		// too big jump, likely hemisphere switch
		// in that case the coordinates given are mirrored symmetrically across the base
		if(dist_direct > dist_mirror)
		{
			pos[0] *= -1;
			pos[1] *= -1;
			pos[2] *= -1;
		} 

		_old_position[sensorNum * 3] = pos[0];
		_old_position[sensorNum * 3 + 1] = pos[1];
		_old_position[sensorNum * 3 + 2] = pos[2];
	} else {
		_calibration_done[sensorNum] = true;

		// first start - assume that the controllers are on the base
		// left one (sensor 0) should have negative x coordinate, right one (sensor 1) positive.
		// if it isn't so, mirror them.

		if((sensorNum == 0 && pos[0] > 0) ||
			(sensorNum == 1 && pos[0] < 0))
		{
			pos[0] *= -1;
			pos[1] *= -1;
			pos[2] *= -1;
		}
	}

	d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_int16>(data) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	vrpn_uint8 buttonBits = vrpn_unbuffer_from_little_endian<vrpn_uint8>(data);

	// jitter filtering
	const vrpn_float64 *filtered = _filters[sensorNum]->filter(pos);
	memcpy(pos, filtered, sizeof(vrpn_float64) * 3);	

	const double *q_filtered = _qfilters[sensorNum]->filter(d_quat);
	d_quat[Q_W] = q_filtered[Q_W];
	d_quat[Q_X] = q_filtered[Q_X];
	d_quat[Q_Y] = q_filtered[Q_Y];
	d_quat[Q_Z] = q_filtered[Q_Z];
	
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
