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
static const vrpn_uint16 CONTOUR_SHUTTLEPROV2 = 0x0030;

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

vrpn_Contour::vrpn_Contour(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();

	// We've not yet gotten a dial response.
	_gotDial = false;
}

vrpn_Contour::~vrpn_Contour(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Contour::~vrpn_Contour(): delete failed\n");
    return;
  }
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

int vrpn_Contour::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Contour::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

vrpn_Contour_ShuttleXpress::vrpn_Contour_ShuttleXpress(const char *name, vrpn_Connection *c)
    : vrpn_Contour(new vrpn_HidProductAcceptor(CONTOUR_VENDOR, CONTOUR_SHUTTLEXPRESS), name, c, CONTOUR_VENDOR, CONTOUR_SHUTTLEXPRESS)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
  vrpn_Analog::num_channel = 2;
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

                // Call the server_mainloop on our unique base class.
                server_mainloop();
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
		vrpn_Analog::report_changes(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report_changes();
	}
}

void vrpn_Contour_ShuttleXpress::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Decode all full reports, each of which is 5 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	if (bytes == 5) {
		if (!_gotDial) {
			_gotDial = true;
		}
		else {
			dials[0] = static_cast<vrpn_int8>(buffer[1] - _lastDial) / 10.0;
		}
		_lastDial = buffer[1];

		// analog (1st byte): 0 center, 1..7 right, -1..-7 left
		normalize_axis((unsigned int)((static_cast<float>(static_cast<vrpn_int8>(buffer[0])) * 128.0f / 7.0f) + 128.0f), 0, 1.0f, channel[0]);
		// Second analog integrates the dial value.
		channel[1] += dials[0];

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


vrpn_Contour_ShuttlePROv2::vrpn_Contour_ShuttlePROv2(const char *name, vrpn_Connection *c)
    : vrpn_Contour(new vrpn_HidProductAcceptor(CONTOUR_VENDOR, CONTOUR_SHUTTLEPROV2), name, c, CONTOUR_VENDOR, CONTOUR_SHUTTLEPROV2)
	, vrpn_Analog(name, c)
	, vrpn_Button_Filter(name, c)
	, vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 15;

	// Initialize the state of all the analogs, buttons, and dials
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Contour_ShuttlePROv2::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, _timestamp) > POLL_INTERVAL) {
		_timestamp = current_time;
		report_changes();

                // Call the server_mainloop on our unique base class.
                server_mainloop();
	}
}

void vrpn_Contour_ShuttlePROv2::report(vrpn_uint32 class_of_service)
{
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

void vrpn_Contour_ShuttlePROv2::report_changes(vrpn_uint32 class_of_service)
{
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
		vrpn_Analog::report_changes(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report_changes();
	}
}

void vrpn_Contour_ShuttlePROv2::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Print the report so we can figure out what is going on.
/*	for (size_t i = 0; i < bytes; i++) {
	printf("%02x ", buffer[i]);
	}
	printf("\n");
	return; */

	// Decode all full reports, each of which is 5 bytes long.
	// Because there is only one type of report, the initial "0" report-type
	// byte is removed by the HIDAPI driver.
	if (bytes == 5) {
		if (!_gotDial) {
			_gotDial = true;
		}
		else {
			dials[0] = static_cast<vrpn_int8>(buffer[1] - _lastDial) / 10.0;
		}
		_lastDial = buffer[1];

		// analog (1st byte): 0 center, 1..7 right, -1..-7 left
		normalize_axis((unsigned int)((static_cast<float>(static_cast<vrpn_int8>(buffer[0])) * 128.0f / 7.0f) + 128.0f), 0, 1.0f, channel[0]);
		// Second analog integrates the dial value.
		channel[1] += dials[0];

		// Top row of four buttons, from left to right: index[bit]
		//   3[0], 3[1], 3[2], 3[3]
		// Second row of 5 buttons, from left to right:
		//   3[4], 3[5], 3[6], 3[7], 4[0]
		// Four lower buttons, from left to right and then down to next row:
		//   4[1], 4[2], 4[3], 4[4]
		// Two black buttons, from left to right:
		//   4[5], 4[6]
		for (int btn = 0; btn <= 15; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 8 + 3;
			mask = static_cast<vrpn_uint8>(1 << (btn % 8));

			buttons[btn] = (*offset & mask) != 0;
		}
	}
	else {
		fprintf(stderr, "vrpn_Contour_ShuttlePROv2: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
