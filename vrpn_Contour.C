// vrpn_Contour.C: VRPN driver for Contour Design devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_Contour.h"
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 CONTOUR_VENDOR = 0x0b33;
static const vrpn_uint16 CONTOUR_SHUTTLEXPRESS = 0x0020;

static void normalize_axis(const unsigned int value, const short deadzone, const vrpn_float64 scale, vrpn_float64& channel) {
	channel = (static_cast<float>(value) - 128.0f);
	if (fabs(channel) < deadzone)
	{
		channel = 0.0f;
	}
	else
	{
		channel /= 128.0f;
	}
	channel *= scale;
	if (channel < -1.0) { channel = -1.0; }
	if (channel > 1.0) { channel = 1.0; }
}

static void normalize_axes(const unsigned int x, const unsigned int y, const short deadzone, const vrpn_float64 scale, vrpn_float64& channelX, vrpn_float64& channelY) {
	normalize_axis(x, deadzone, scale, channelX);
	normalize_axis(y, deadzone, scale, channelY);
}

vrpn_Contour::vrpn_Contour(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c)
  : vrpn_HidInterface(filter)
  , vrpn_BaseClass(name, c)
  , _filter(filter)
{
	init_hid();
}

vrpn_Contour::~vrpn_Contour(void)
{
  delete _filter;
}

void vrpn_Contour::init_hid(void) {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Contour::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Contour::on_last_disconnect(void *thisPtr, vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Contour *me = static_cast<vrpn_Contour *>(thisPtr);
	return 0;
}

int vrpn_Contour::on_connect(void *thisPtr, vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Contour *me = static_cast<vrpn_Contour *>(thisPtr);
	return 0;
}

vrpn_Contour_ShuttleXpress::vrpn_Contour_ShuttleXpress(const char *name, vrpn_Connection *c)
  : vrpn_Contour(_filter = new vrpn_HidProductAcceptor(CONTOUR_VENDOR, CONTOUR_SHUTTLEXPRESS), name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Analog(name, c)
  , vrpn_Dial(name, c)
{
  vrpn_Analog::num_channel = 1;
  vrpn_Dial::num_dials = 1;
  vrpn_Button::num_buttons = 5;

  // Initialize the state of all the analogs, buttons, and dials
  _lastDial = 0;
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_Contour_ShuttleXpress::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, _timestamp) > POLL_INTERVAL ) {
		_timestamp = current_time;
		report_changes();

		if (vrpn_Analog::num_channel > 0)
		{
			vrpn_Analog::server_mainloop();
		}
		if (vrpn_Button::num_buttons > 0)
		{
			vrpn_Button::server_mainloop();
		}
		if (vrpn_Dial::num_dials > 0)
		{
			vrpn_Dial::server_mainloop();
		}
	}
}

void vrpn_Contour_ShuttleXpress::report(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report();
	}
}

void vrpn_Contour_ShuttleXpress::report_changes(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report();
	}
}

void vrpn_Contour_ShuttleXpress::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Decode all full reports, each of which is 5 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 5) {
		// analog (1st byte): 0 center, 1..7 right, -1..-7 left
		normalize_axis((unsigned int) ((static_cast<float>(static_cast<vrpn_int8>(buffer[0])) * 128.0f / 7.0f) + 128.0f), 0, 1.0f, channel[0]);

		if (vrpn_Dial::num_dials > 0)
		{
			// dial (2nd byte)
			// Do the unsigned/signed conversion at the last minute so the signed values work properly.
			dials[0] = static_cast<vrpn_int8>(buffer[1] - _lastDial);
			_lastDial = buffer[1];
		}
		else
		{
			// dial (2nd byte)
			normalize_axis((unsigned int) (static_cast<float>(static_cast<vrpn_int8>(buffer[1])) + 128.0f), 0, 1.0f, channel[1]);
		}

		vrpn_uint8 value;
		// buttons (4th byte):
		value = buffer[3];
		for (int btn = 0; btn < 4; btn++) {
			vrpn_uint8 mask = static_cast<vrpn_uint8>((1 << (btn % 8)) << 4);
			buttons[btn] = ((value & mask) != 0);
		}
		// buttons (5th byte):
		value = buffer[4];
		for (int btn = 0; btn < 1; btn++) {
			vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn + 4] = ((value & mask) != 0);
		}
	} else {
		fprintf(stderr, "vrpn_Contour_ShuttleXpress: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
