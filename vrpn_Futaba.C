// vrpn_Futaba.C: VRPN driver for Futaba devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_Futaba.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 FUTABA_VENDOR = 0x1781;
static const vrpn_uint16 FUTABA_ELITE = 0x0e56;

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

vrpn_Futaba::vrpn_Futaba(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();
}

vrpn_Futaba::~vrpn_Futaba(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Futaba::~vrpn_Futaba(): delete failed\n");
    return;
  }
}

void vrpn_Futaba::init_hid(void) {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Futaba::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Futaba::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Futaba::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

vrpn_Futaba_InterLink_Elite::vrpn_Futaba_InterLink_Elite(const char *name, vrpn_Connection *c)
    : vrpn_Futaba(new vrpn_HidProductAcceptor(FUTABA_VENDOR, FUTABA_ELITE), name, c, FUTABA_VENDOR, FUTABA_ELITE)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
  vrpn_Analog::num_channel = 5;
  vrpn_Dial::num_dials = ((vrpn_Analog::num_channel == 4) ? 1 : 0);
  vrpn_Button::num_buttons = 18;

  // Initialize the state of all the analogs, buttons, and dials
  _lastDial = 0;
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_Futaba_InterLink_Elite::mainloop(void)
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

void vrpn_Futaba_InterLink_Elite::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::timestamp = _timestamp;
	}

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report();
	}
}

void vrpn_Futaba_InterLink_Elite::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::timestamp = _timestamp;
	}

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report();
	}
}

void vrpn_Futaba_InterLink_Elite::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Decode all full reports, each of which is 8 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 8) {
		if (buffer[0] == 5) {

			// Report joystick axes as analogs
			//	rudder (6th byte, left joy left/right, left/right): Left 2B, center (normal) 82 (calc 84), right DC
			//	throttle (4th byte, left joy up/down): Up 32, (center calc 7f), down CC
			normalize_axes(buffer[5], buffer[3], 5, (1.0f / 0.70f), channel[0], channel[1]);
			//	aileron (2nd byte, right joy left/right, roll): Left 27, center (normal) 81 (calc 7e), right D5
			//	elevator (3rd byte, right joy up/down, forward/back): Up 34, center (normal) 81 (calc 81), down CF
			normalize_axes(buffer[1], buffer[2], 5, (1.0f / 0.70f), channel[2], channel[3]);

			if (vrpn_Dial::num_dials > 0) {
				// dial (5th byte): Ch6 00-FF
				// Do the unsigned/signed conversion at the last minute so the
				// signed values work properly.
				dials[0] = static_cast<vrpn_int8>(buffer[4] - _lastDial) / 128.0;
				// Store the current dial position for the next delta
				_lastDial = buffer[4];
			}
			else
			{
				// dial (5th byte): Ch6 Flaps Gain 00-FF
				normalize_axis(buffer[4], 5, 1.0f, channel[4]);
			}

			vrpn_uint8 value;
			// switches (8th byte):
			value = buffer[7];
			//	button #0: 01 Ch5 fwd
			//	button #1: 02 Ch7 fwd
			//	button #2: 04 reset
			//	button #3: 08 Ch8 down
			//	button #4: 10 Ch8 up
			//	button #5: 20 <none>
			//	button #6: 40 menu/select
			//	button #7: 80 cancel
			for (int btn = 0; btn < 8; btn++) {
				vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << (btn % 8));
				buttons[btn] = ((value & mask) != 0);
			}

			// trim switches (7th byte): it is not possible to have the odd button at the same times as the previous even button
			value = buffer[6];
			//	button #8: 01 aileron right
			//	button #9: 02 aileron left
			//	button #10:03 elevator up
			//	button #11:06 elevator down
			//	button #12:09 rudder right
			//	button #13:12 rudder left
			//	button #14:1B throttle up
			//	button #15:36 throttle down
			//	button #16:51 up
			//	button #17:a2 down
			//0000 0001    1
			//0000 0010     2
			//0000 0011    3,4,5
			//0000 0110     6,7,8
			//0000 1001    9,a,b,c,d,e,f,10,11
			//0001 0010     12,13,14,15,16,17,18,19,1a
			//0001 1011    1b,...,35
			//0011 0110     36,...,50
			//0101 0001    51,...,a1
			//1010 0010     a2,...,f2
			//start with highest and work down.
			const vrpn_uint8 btnCount = 10;
			const vrpn_uint8 btnValues[btnCount] = {0xa2, 0x51, 0x36, 0x1b, 0x12, 0x09, 0x06, 0x03, 0x02, 0x01};
			for (int i = 0; i < btnCount; i++) {
				if ((value & btnValues[i]) == btnValues[i]) {
					value ^= btnValues[i];
					buttons[17 - i] = 1;
				} else {
					buttons[17 - i] = 0;
				}
			}
		} else {
			fprintf(stderr, "vrpn_Futaba_InterLink_Elite: Unknown report = %u\n", static_cast<unsigned>(buffer[0]));
		}
	} else {
		fprintf(stderr, "vrpn_Futaba_InterLink_Elite: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
