// vrpn_Saitek_Controller_Raw.C: VRPN driver for Saitek Controller Raw devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for sqrt and fabs

#include "vrpn_Saitek_Controller_Raw.h"
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 SAITEK_VENDOR = 0x06a3;
static const vrpn_uint16 ST290_PRO = 0x0d60;

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

#define GAMEPAD_TRIGGER_THRESHOLD 30

//////////////////////////////////////////////////////////////////////////
// helpers
//////////////////////////////////////////////////////////////////////////
static vrpn_float64 normalize_dpad(unsigned char up, unsigned char right, unsigned char down, unsigned char left)
{
	int x = 0;
	int y = 0;
	if (right)
	{
		x += 1;
	}
	if (left)
	{
		x -= 1;
	}
	if (up)
	{
		y += 1;
	}
	if (down)
	{
		y -= 1;
	}
	size_t index = ((x + 1) * 3) + (y + 1);
	vrpn_float64 angles[] = {225, 270, 315, 180, -1, 0, 135, 90, 45};
	return (angles[index]);
}

static void normalize_axis(const unsigned int value, const short deadzone, const vrpn_float64 scale, vrpn_float64& channel, int wordSize = 16)
{
	channel = (static_cast<float>(value) - (float) (1 << (wordSize - 1)));
	if (fabs(channel) < (deadzone * 3 / 4))
	{
		channel = 0.0f;
	}
	else
	{
		channel /= (float) (1 << (wordSize - 1));
	}
	channel *= scale;
	if (channel < -1.0) { channel = -1.0; }
	if (channel > 1.0) { channel = 1.0; }
}

static void normalize_axes(const unsigned int x, const unsigned int y, const short deadzone, const vrpn_float64 scale, vrpn_float64& channelX, vrpn_float64& channelY, int wordSize = 16)
{
	normalize_axis(x, deadzone, scale, channelX, wordSize);
	normalize_axis(y, deadzone, scale, channelY, wordSize);
}

//////////////////////////////////////////////////////////////////////////
// Common base class
//////////////////////////////////////////////////////////////////////////
vrpn_Saitek_Controller_Raw::vrpn_Saitek_Controller_Raw(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
        vrpn_uint16 vendor, vrpn_uint16 product) :
	vrpn_BaseClass(name, c), vrpn_HidInterface(filter, vendor, product), _filter(filter)
{
	init_hid();
}

vrpn_Saitek_Controller_Raw::~vrpn_Saitek_Controller_Raw(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Saitek_Controller_Raw::~vrpn_Saitek_Controller_Raw(): delete failed\n");
    return;
  }
}

void vrpn_Saitek_Controller_Raw::init_hid()
{
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Saitek_Controller_Raw::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
	decodePacket(bytes, buffer);
}

int vrpn_Saitek_Controller_Raw::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return (0);
}

int vrpn_Saitek_Controller_Raw::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return (0);
}

//////////////////////////////////////////////////////////////////////////
// ST290 Pro Joystick
//////////////////////////////////////////////////////////////////////////
vrpn_Saitek_ST290_Pro::vrpn_Saitek_ST290_Pro(const char *name, vrpn_Connection *c) :
vrpn_Saitek_Controller_Raw(new vrpn_HidProductAcceptor(SAITEK_VENDOR, ST290_PRO), name, c, SAITEK_VENDOR, ST290_PRO),
	vrpn_Analog(name, c), vrpn_Button_Filter(name, c), vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 5;
	vrpn_Dial::num_dials = 0;
	vrpn_Button::num_buttons = 10;

	// Initialize the state of all the analogs, buttons, and dials
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Saitek_ST290_Pro::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, _timestamp) > POLL_INTERVAL)
	{
		_timestamp = current_time;
		report_changes();

                // Call the server_mainloop on our unique base class.
                server_mainloop();
	}
}

void vrpn_Saitek_ST290_Pro::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::timestamp = _timestamp;
	}

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	if (vrpn_Dial::num_dials > 0)
	{
		vrpn_Dial::report();
	}
}

void vrpn_Saitek_ST290_Pro::report_changes(vrpn_uint32 class_of_service)
{
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

void vrpn_Saitek_ST290_Pro::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// ST290 Pro joystick

	// Decode all full reports, each of which is 6 bytes long.
		// Because there is only one type of report, the initial "0" report-type
		// byte is removed by the HIDAPI driver.
	/*
	[0]:	X-axis (left=00, right=ff)
	[1]:	Y-axis - lower byte (up=00, down=ff)
	[2]:	Z-rotate (left=00, right=ff)
	[3]:	Slider (up=00, down=ff)
	[4]:	buttons high nibble "1"=0x10, "2"=0x20, "3"=0x40, "4"=0x80, POV Hat low nibble (none=0x00, N=0x01, NE=0x02, ... NW=0x08)
	[5]:	0xf0 high nibble, buttons low nibble (none=0x00, "5"=0x01, "6"=0x02)
	*/
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 6)
	{
		normalize_axes(buffer[0], buffer[1], 0x08, 1.0f, channel[0], channel[1], 8);
		normalize_axis(buffer[2], 0x08, 1.0f, channel[2], 8);
		normalize_axis(buffer[3], 0x08, 1.0f, channel[3], 8);

		vrpn_uint8 value, mask;
		value = (buffer[4] >> 4);
		for (int btn = 0; btn < 4; btn++)
		{
			mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn] = ((value & mask) != 0);
		}
		value = (buffer[5] & 0x0f);
		for (int btn = 0; btn < 4; btn++)
		{
			mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn + 4] = ((value & mask) != 0);
		}

		// Point of View Hat
		buttons[6] = buttons[7] = buttons[8] = buttons[9] = 0;
		switch (buffer[4] & 0x0f)
		{
		case 1:		// up
			buttons[6] = true;
			break;
		case 2:
			buttons[6] = buttons[7] = true;
			break;
		case 3:		// right
			buttons[7] = true;
			break;
		case 4:
			buttons[7] = buttons[8] = true;
			break;
		case 5:		// down
			buttons[8] = true;
			break;
		case 6:
			buttons[8] = buttons[9] = true;
			break;
		case 7:		// left
			buttons[9] = true;
			break;
		case 8:
			buttons[9] = buttons[6] = true;
			break;
		case 0:
		default:
			// nothing to do
			break;
		}
		channel[4] = normalize_dpad(buttons[7], buttons[8], buttons[9], buttons[10]);
	}
	else
	{
		fprintf(stderr, "vrpn_Saitek_ST290_Pro: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
