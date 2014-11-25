// vrpn_XInputGamepad.C: Gamepad driver for devices using XInput
// such as (read: primarily) the Microsoft Xbox 360 controller.

#include "vrpn_XInputGamepad.h"

#if defined(_WIN32) && defined(VRPN_USE_WINDOWS_XINPUT)
#include <xinput.h>

vrpn_XInputGamepad::vrpn_XInputGamepad(const char *name, vrpn_Connection *c, unsigned int controllerIndex):
	vrpn_Analog(name, c),
	vrpn_Button_Filter(name, c),
	vrpn_Analog_Output(name, c),
	_controllerIndex(controllerIndex)
{
	vrpn_Analog::num_channel = 7;
	vrpn_Button::num_buttons = 10;
	vrpn_Analog_Output::o_num_channel = 2;

	_motorSpeed[0] = 0;
	_motorSpeed[1] = 0;

	if (register_autodeleted_handler(request_m_id,
	 handle_request_message, this, d_sender_id)) {
		fprintf(stderr, "vrpn_XInputGamepad: Can't register request-single handler\n");
		return;
	}

	if (register_autodeleted_handler(request_channels_m_id,
	 handle_request_channels_message, this, d_sender_id)) {
		fprintf(stderr, "vrpn_XInputGamepad: Can't register request-multiple handler\n");
		return;
	}

	if (register_autodeleted_handler(
	 d_connection->register_message_type(vrpn_dropped_last_connection),
	 handle_last_connection_dropped, this)) {
		fprintf(stderr, "vrpn_XInputGamepad: Can't register connections-dropped handler\n");
		return;
	}
}

vrpn_XInputGamepad::~vrpn_XInputGamepad() {
}

void vrpn_XInputGamepad::mainloop() {
	XINPUT_STATE state;
	DWORD rv;

	server_mainloop();
	if ((rv = XInputGetState(_controllerIndex, &state)) != ERROR_SUCCESS) {
		char errMsg[256];
		struct timeval now;

		if (rv == ERROR_DEVICE_NOT_CONNECTED)
			sprintf(errMsg, "XInput device %u not connected", _controllerIndex);
		else
			sprintf(errMsg, "XInput device %u returned Windows error code %u",
				_controllerIndex, rv);

		vrpn_gettimeofday(&now, NULL);
		send_text_message(errMsg, now, vrpn_TEXT_ERROR);
		return;
	}

	// Set device state in VRPN_Analog
	channel[0] = normalize_axis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[1] = normalize_axis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[2] = normalize_axis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[3] = normalize_axis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[4] = normalize_dpad(state.Gamepad.wButtons);
	channel[5] = normalize_trigger(state.Gamepad.bLeftTrigger);
	channel[6] = normalize_trigger(state.Gamepad.bRightTrigger);

	// Set device state in VRPN_Button
	// Buttons are listed in DirectInput ordering
	buttons[0] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
	buttons[1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
	buttons[2] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
	buttons[3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
	buttons[4] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
	buttons[5] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
	buttons[6] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
	buttons[7] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
	buttons[8] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
	buttons[9] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

vrpn_float64 vrpn_XInputGamepad::normalize_dpad(WORD buttons) const {
	int x = 0;
	int y = 0;

	if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
		x += 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
		x -= 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_UP)
		y += 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
		y -= 1;

	size_t index = (x + 1) * 3 + (y + 1);
	vrpn_float64 angles[] = {225, 270, 315, 180, -1, 0, 135, 90, 45};
	return angles[index];
}


vrpn_float64 vrpn_XInputGamepad::normalize_axis(SHORT axis, SHORT deadzone) const {
	// Filter out areas near the center
	if (axis > -deadzone && axis < deadzone)
		return 0;

	// Note ranges are asymmetric (-32768 to 32767)
	return axis / ((axis < 0) ? 32768.0 : 32767.0);
}

vrpn_float64 vrpn_XInputGamepad::normalize_trigger(BYTE trigger) const {
	// Filter out low-intensity signals
	if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		return 0;

	return trigger / 255.0;
}

void vrpn_XInputGamepad::update_vibration() {
	XINPUT_VIBRATION vibration;

	vibration.wLeftMotorSpeed = _motorSpeed[0];
	vibration.wRightMotorSpeed = _motorSpeed[1];

	DWORD rv = XInputSetState(_controllerIndex, &vibration);
	if (rv != ERROR_SUCCESS) {
		char errMsg[256];
		struct timeval now;

		if (rv == ERROR_DEVICE_NOT_CONNECTED)
			sprintf(errMsg, "XInput device %u not connected", _controllerIndex);
		else
			sprintf(errMsg, "XInput device %u returned Windows error code %u",
				_controllerIndex, rv);

		vrpn_gettimeofday(&now, NULL);
		send_text_message(errMsg, now, vrpn_TEXT_ERROR);
		return;
	}
}

void vrpn_XInputGamepad::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_XInputGamepad::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

// Static callback
int VRPN_CALLBACK vrpn_XInputGamepad::handle_request_message(void *selfPtr,
	vrpn_HANDLERPARAM data)
{
    const char *bufptr = data.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
	vrpn_XInputGamepad *me = (vrpn_XInputGamepad *) selfPtr;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
	if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
		fprintf(stderr,"vrpn_Analog_Output_Server::handle_request_message(): Index out of bounds\n");
		char msg[1024];
		sprintf( msg, "Error:  (handle_request_message):  channel %d is not active.  Squelching.", chan_num );
		me->send_text_message( msg, data.msg_time, vrpn_TEXT_ERROR );
		return 0;
	}
	me->o_channel[chan_num] = value;

	float magnitude = static_cast<float>(value);
	magnitude = (magnitude < 0) ? 0 : (magnitude > 1) ? 1 : magnitude;

	me->_motorSpeed[chan_num] = static_cast<WORD>(magnitude * 65535);
	me->update_vibration();

    return 0;
}

// Static callback
int VRPN_CALLBACK vrpn_XInputGamepad::handle_request_channels_message(void *selfPtr,
	vrpn_HANDLERPARAM data)
{
	const char *bufptr = data.buffer;
	vrpn_int32 chan_num;
	vrpn_int32 pad;
	vrpn_XInputGamepad *me = (vrpn_XInputGamepad *) selfPtr;
	int i;

	// Read the parameters from the buffer
	vrpn_unbuffer(&bufptr, &chan_num);
	vrpn_unbuffer(&bufptr, &pad);

	if (chan_num > me->o_num_channel) {
		char msg[1024];
		sprintf( msg, "Error:  (handle_request_channels_message):  channels above %d not active; "
			"bad request up to channel %d.  Squelching.", me->o_num_channel, chan_num );
		me->send_text_message( msg, data.msg_time, vrpn_TEXT_ERROR );
		chan_num = me->o_num_channel;
	}
	if (chan_num < 0) {
		char msg[1024];
		sprintf( msg, "Error:  (handle_request_channels_message):  invalid channel %d.  Squelching.", chan_num );
		me->send_text_message( msg, data.msg_time, vrpn_TEXT_ERROR );
		return 0;
	}
	for (i = 0; i < chan_num; i++) {
		vrpn_float64 value;
		vrpn_unbuffer(&bufptr, &value);

		float magnitude = static_cast<float>(value);
		magnitude = (magnitude < 0) ? 0 : (magnitude > 1) ? 1 : magnitude;

		me->_motorSpeed[chan_num] = static_cast<WORD>(magnitude * 65535);
	}

	me->update_vibration();

	return 0;
}

// Static callback
int VRPN_CALLBACK vrpn_XInputGamepad::handle_last_connection_dropped(void *selfPtr,
	vrpn_HANDLERPARAM data)
{
	vrpn_XInputGamepad *me = static_cast<vrpn_XInputGamepad *>(selfPtr);

	// Kill force feedback if no one is connected
	me->_motorSpeed[0] = 0;
	me->_motorSpeed[1] = 0;
	me->update_vibration();

	return 0;
}

#endif // _WIN32 && VRPN_USE_DIRECTINPUT

