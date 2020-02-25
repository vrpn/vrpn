// vrpn_Griffin.C: VRPN driver for Griffin Technologies devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_Griffin.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 GRIFFIN_VENDOR = 0x077d;
static const vrpn_uint16 GRIFFIN_POWERMATE = 0x0410;

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

vrpn_Griffin::vrpn_Griffin(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();
}

vrpn_Griffin::~vrpn_Griffin(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Griffin::~vrpn_Griffin(): delete failed\n");
    return;
  }
}

void vrpn_Griffin::init_hid(void) {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Griffin::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Griffin::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Griffin::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

vrpn_Griffin_PowerMate::vrpn_Griffin_PowerMate(const char *name, vrpn_Connection *c)
    : vrpn_Griffin(new vrpn_HidProductAcceptor(GRIFFIN_VENDOR, GRIFFIN_POWERMATE), name, c, GRIFFIN_VENDOR, GRIFFIN_POWERMATE)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
  vrpn_Analog::num_channel = 0;
  vrpn_Dial::num_dials = 1;
  vrpn_Button::num_buttons = 1;

  // Initialize the state of all the analogs, buttons, and dials
  _lastDial = 0;
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_Griffin_PowerMate::mainloop(void)
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

void vrpn_Griffin_PowerMate::report(vrpn_uint32 class_of_service) {
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

void vrpn_Griffin_PowerMate::report_changes(vrpn_uint32 class_of_service) {
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

void vrpn_Griffin_PowerMate::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Decode all full reports, each of which is 8 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 6) {

		if (vrpn_Dial::num_dials > 0) {
			// dial (2nd byte)
			// Do the unsigned/signed conversion at the last minute so the
			// signed values work properly.
			dials[0] = static_cast<vrpn_int8>(buffer[1]);
		} else {
			// dial (2nd byte)
			normalize_axis(buffer[1], 5, 1.0f, channel[0]);
		}

		vrpn_uint8 value;
		// switches (1st byte):
		value = buffer[0];
		//	button #0: 01 button
		for (int btn = 0; btn < 1; btn++) {
			vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn] = ((value & mask) != 0);
		}
	} else {
		fprintf(stderr, "vrpn_Griffin_PowerMate: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
