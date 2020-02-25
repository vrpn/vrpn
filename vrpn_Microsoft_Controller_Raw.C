// vrpn_Microsoft_Controller_Raw.C: VRPN driver for Microsoft Controller Raw devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for sqrt and fabs

#include "vrpn_Microsoft_Controller_Raw.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

// USB vendor and product IDs for the models we support
static const vrpn_uint16 MICROSOFT_VENDOR = 0x045e;
static const vrpn_uint16 SIDEWINDER_PRECISION_2 = 0x0038;
static const vrpn_uint16 SIDEWINDER = 0x003c;
static const vrpn_uint16 XBOX_S = 0x0289;
static const vrpn_uint16 XBOX_360 = 0x028e;
static const vrpn_uint16 XBOX_360_WIRELESS = 0x02a1;

// and generic controllers that act the same as the above
static const vrpn_uint16 AFTERGLOW_VENDOR = 0x0e6f;
static const vrpn_uint16 AX1_FOR_XBOX_360 = 0x0213;

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

#define MS_GAMEPAD_LEFT_THUMB_DEADZONE 7849
#define MS_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define MS_GAMEPAD_TRIGGER_THRESHOLD 30

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
#ifdef FUTURE
	// adapted from: http://msdn.microsoft.com/en-us/library/windows/desktop/ee417001%28v=vs.85%29.aspx
	// determine how far the controller is pushed
	float magnitude = (float) sqrt((double) ((x * x) + (y * y)));

	// determine the direction the controller is pushed
	float normalizedX = ((magnitude > 0.0f) ? (x / magnitude) : 0.0f);
	float normalizedY = ((magnitude > 0.0f) ? (y / magnitude) : 0.0f);

	float normalizedMagnitude = 0.0f;

	// check if the controller is outside a circular dead zone
	if (magnitude > deadzone)
	{
		// clip the magnitude at its expected maximum value
		if (magnitude > 32767)
		{
			magnitude = 32767;
		}

		// adjust magnitude relative to the end of the dead zone
		magnitude -= deadzone;

		// optionally normalize the magnitude with respect to its
		// expected range giving a magnitude value of 0.0 to 1.0
		normalizedMagnitude = magnitude / (32767.0f - deadzone);
	}
	else
	{	// if the controller is in the deadzone zero out the magnitude
		magnitude = 0.0f;
		normalizedMagnitude = 0.0f;
	}
#else
	normalize_axis(x, deadzone, scale, channelX, wordSize);
	normalize_axis(y, deadzone, scale, channelY, wordSize);
#endif // FUTURE
}

static vrpn_float64 normalize_trigger(unsigned int trigger)
{
	// Filter out low-intensity signals
	int value = trigger - 0x80;
	return ((fabs(static_cast<double>(value)) < MS_GAMEPAD_TRIGGER_THRESHOLD) ? 0.0f : (value * 2.0f / 255.0f));
}

//////////////////////////////////////////////////////////////////////////
// Common base class
//////////////////////////////////////////////////////////////////////////
vrpn_Microsoft_Controller_Raw::vrpn_Microsoft_Controller_Raw(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();
}

vrpn_Microsoft_Controller_Raw::~vrpn_Microsoft_Controller_Raw(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Microsoft_Controller_Raw::~vrpn_Microsoft_Controller_Raw(): delete failed\n");
    return;
  }
}

void vrpn_Microsoft_Controller_Raw::init_hid() {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Microsoft_Controller_Raw::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Microsoft_Controller_Raw::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Microsoft_Controller_Raw::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// SideWinder Precision 2 Joystick
//////////////////////////////////////////////////////////////////////////
vrpn_Microsoft_SideWinder_Precision_2::vrpn_Microsoft_SideWinder_Precision_2(const char *name, vrpn_Connection *c) :
vrpn_Microsoft_Controller_Raw(new vrpn_HidProductAcceptor(MICROSOFT_VENDOR, SIDEWINDER_PRECISION_2), name, c, MICROSOFT_VENDOR, SIDEWINDER_PRECISION_2),
	vrpn_Analog(name, c), vrpn_Button_Filter(name, c), vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 5;
	vrpn_Dial::num_dials = 0;
	vrpn_Button::num_buttons = 12;

	// Initialize the state of all the analogs, buttons, and dials
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Microsoft_SideWinder_Precision_2::mainloop(void)
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

void vrpn_Microsoft_SideWinder_Precision_2::report(vrpn_uint32 class_of_service)
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

void vrpn_Microsoft_SideWinder_Precision_2::report_changes(vrpn_uint32 class_of_service)
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

void vrpn_Microsoft_SideWinder_Precision_2::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// SideWinder Precision 2 joystick

	// Decode all full reports, each of which is 40 bytes long.
		// Because there is only one type of report, the initial "0" report-type
		// byte is removed by the HIDAPI driver.
	/*
	Byte   : Bit 7   6   5   4   3   2   1   0 | 7   6   5   4   3   2   1   0
	[0]:	X-axis (left=00, right=ff)
	[1]:	Y-axis (up=00, down=ff)
	[2]:	Z-rotate (left=00, right=ff)
	[3]:	Slider (up=00, down=ff)
	[4]:	buttons (bit flags: none=0x00, "1" (trigger)=0x01, "2"=0x02, "3"=0x04, ..., "8"=0x80
	[5]:	POV Hat high nibble (none=0x80, N=0x00, NE=0x10, ... NW=0x80)
	*/
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 6)
	{
		normalize_axes(buffer[0], buffer[1], 0x08, 1.0f, channel[0], channel[1], 8);
		normalize_axis(buffer[2], 0x08, 1.0f, channel[2], 8);
		normalize_axis(buffer[3], 0x08, 1.0f, channel[3], 8);

		vrpn_uint8 value, mask;
		value = buffer[4];
		for (int btn = 0; btn < 8; btn++)
		{
			mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn] = ((value & mask) != 0);
		}

		// Point of View Hat
		buttons[8] = buttons[9] = buttons[10] = buttons[11] = 0;
		switch (buffer[5] >> 4)
		{
		case 0:		// up
			buttons[8] = true;
			break;
		case 1:
			buttons[8] = buttons[9] = true;
			break;
		case 2:		// right
			buttons[9] = true;
			break;
		case 3:
			buttons[9] = buttons[10] = true;
			break;
		case 4:		// down
			buttons[10] = true;
			break;
		case 5:
			buttons[10] = buttons[11] = true;
			break;
		case 6:		// left
			buttons[11] = true;
			break;
		case 7:
			buttons[11] = buttons[8] = true;
			break;
		case 8:
		default:
			// nothing to do
			break;
		}
		channel[4] = normalize_dpad(buttons[8], buttons[9], buttons[10], buttons[11]);
	}
	else
	{
		fprintf(stderr, "vrpn_Microsoft_SideWinder_Precision_2: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

//////////////////////////////////////////////////////////////////////////
// SideWinder Joystick
//////////////////////////////////////////////////////////////////////////
vrpn_Microsoft_SideWinder::vrpn_Microsoft_SideWinder(const char *name, vrpn_Connection *c) :
vrpn_Microsoft_Controller_Raw(new vrpn_HidProductAcceptor(MICROSOFT_VENDOR, SIDEWINDER), name, c, MICROSOFT_VENDOR, SIDEWINDER),
	vrpn_Analog(name, c), vrpn_Button_Filter(name, c), vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 3;
	vrpn_Dial::num_dials = 0;
	vrpn_Button::num_buttons = 8;

	// Initialize the state of all the analogs, buttons, and dials
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Microsoft_SideWinder::mainloop(void)
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

void vrpn_Microsoft_SideWinder::report(vrpn_uint32 class_of_service)
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

void vrpn_Microsoft_SideWinder::report_changes(vrpn_uint32 class_of_service)
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

void vrpn_Microsoft_SideWinder::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// SideWinder Precision 2 joystick

	// Decode all full reports, each of which is 40 bytes long.
		// Because there is only one type of report, the initial "0" report-type
		// byte is removed by the HIDAPI driver.
	/*
	Byte   : Bit 7   6   5   4   3   2   1   0 | 7   6   5   4   3   2   1   0
	[0]:	X-axis (left=00, right=ff)
	[1]:	Y-axis (up=00, down=ff)
	[2]:	slider (up/fwd=00, down/back=ff)
	[3]:	buttons (bit flags: none=0x00, "1" (trigger)=0x01, "2"=0x02, "3"=0x04, ..., "8"=0x80)
	*/
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 4)
	{
		normalize_axes(buffer[0], buffer[1], 0x08, 1.0f, channel[0], channel[1], 8);
		normalize_axis(buffer[2], 0x08, 1.0f, channel[2], 8);

		vrpn_uint8 value, mask;
		value = buffer[3];
		for (int btn = 0; btn < 8; btn++)
		{
			mask = static_cast<vrpn_uint8>(1 << (btn % 8));
			buttons[btn] = ((value & mask) != 0);
		}
	}
	else
	{
		fprintf(stderr, "vrpn_Microsoft_SideWinder: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

//////////////////////////////////////////////////////////////////////////
// Xbox S
//////////////////////////////////////////////////////////////////////////
vrpn_Microsoft_Controller_Raw_Xbox_S::vrpn_Microsoft_Controller_Raw_Xbox_S(const char *name, vrpn_Connection *c)
    : vrpn_Microsoft_Controller_Raw(new vrpn_HidProductAcceptor(MICROSOFT_VENDOR, XBOX_S), name, c, MICROSOFT_VENDOR, XBOX_S)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
  vrpn_Analog::num_channel = 5;
  vrpn_Dial::num_dials = 0;
  vrpn_Button::num_buttons = 16;

  // Initialize the state of all the analogs, buttons, and dials
  _lastDial = 0;
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_Microsoft_Controller_Raw_Xbox_S::mainloop()
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

void vrpn_Microsoft_Controller_Raw_Xbox_S::report(vrpn_uint32 class_of_service) {
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

void vrpn_Microsoft_Controller_Raw_Xbox_S::report_changes(vrpn_uint32 class_of_service) {
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

void vrpn_Microsoft_Controller_Raw_Xbox_S::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Xbox Controller S requires adapter to be made/purchased to connect USB to computer.
	// Also, it may require a driver to be installed such as XBCD_Installer_0.2.7.exe (Windows).

	// Decode all full reports, each of which is 40 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	/*
	Byte   : Bit 7   6   5   4   3   2   1   0 | 7   6   5   4   3   2   1   0
	[0]    :   |           Report ID           |
	[1]    :   |B8 |B7 |B6 |B5 |B4 |B3 |B2 |B1 |
	[2]    :   |B16|B15|B14|B13|B12|B11|B10|B9 |
	[3]    :   |B24|B23|B22|B21|B20|B19|B18|B17|   <------- no such buttons exist and data does not change
	[4-5]  :   |                            X Axis                             |
	[6-7]  :   |                            Y Axis                             |
	[8-9]  :   |                            Z Axis                             |
	[10-11] :  |                            RX Axis                            |
	[12-13]:   |                            RY Axis                            |
	[14-15]:   |                            RZ Axis                            |
	[16-17]:   |                          Slider Axis                          |
	[18]   :   |     EMPTY     |      POV      |
	[19]   :   |       Current MapMatrix       |
	--- Original data from Xbox controller ---
	[20]   :   |             0x00              |
	[21]   :   |      0x14(Size of report)     |
	[22]   :   |RSp|LSp|Bk |St |Rgt|Lft|Dn |Up |   <------- other buttons
	[23]   :   |             0x00              |
	[24]   :   |           Button A            |
	[25]   :   |           Button B            |
	[26]   :   |           Button X            |
	[27]   :   |           Button Y            |
	[28]   :   |         Button Black          |
	[29]   :   |         Button White          |
	[30]   :   |         Left Trigger          |
	[31]   :   |         Right Trigger         |
	[32-33]:   |                         Left-Stick X                          |
	[34-35]:   |                         Left-Stick Y                          |
	[36-37]:   |                         Right-Stick X                         |
	[38-39]:   |                         Right-Stick Y                         |
	*/
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 40) {
		if (buffer[0] == 1) {

			// Report joystick axes as analogs
			vrpn_uint16 x, y;
			vrpn_uint8 *bufptr;
#ifdef OLD_DATA
			//	left joy left/right: Left 27, center (normal) 81 (calc 7e), right D5
			bufptr = &buffer[32];
			x = vrpn_unbuffer<vrpn_int16>(bufptr);
			//	left joy up/down: Up 34, center (normal) 81 (calc 81), down CF
			bufptr = &buffer[34];
			y = vrpn_unbuffer<vrpn_int16>(bufptr);
			normalize_axes(x, y, MS_GAMEPAD_LEFT_THUMB_DEADZONE, 1.0f, channel[0], channel[1]);
			//	right joy up/down: Up 32, (center calc 7f), down CC
			bufptr = &buffer[36];
			x = vrpn_unbuffer<vrpn_int16>(bufptr);
			//	right joy left/right: Left 2B, center (normal) 82 (calc 84), right DC
			bufptr = &buffer[38];
			y = vrpn_unbuffer<vrpn_int16>(bufptr);
			normalize_axes(x, y, MS_GAMEPAD_RIGHT_THUMB_DEADZONE, 1.0f, channel[2], channel[3]);
#else
			vrpn_int16 temp;
			bufptr = &buffer[4];
			temp = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) / 2;
			x = temp + (1 << 15);
			x *= 2;
			bufptr = &buffer[6];
			temp = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) / 2;
			y = temp + (1 << 15);
			y *= 2;
			normalize_axes(x, y, MS_GAMEPAD_LEFT_THUMB_DEADZONE, 1.0f, channel[0], channel[1]);
			bufptr = &buffer[10];
			temp = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) / 2;
			x = temp + (1 << 15);
			bufptr = &buffer[12];
			temp = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) / 2;
			y = temp + (1 << 15);
			normalize_axes(x, y, MS_GAMEPAD_LEFT_THUMB_DEADZONE, 2.0f, channel[2], channel[3]);
#endif // OLD_DATA

#ifdef OLD_DATA
			//	button #0: 24 A
			buttons[0] = (buffer[24] != 0);
			//	button #1: 25 B
			buttons[1] = (buffer[25] != 0);
			//	button #2: 26 X
			buttons[2] = (buffer[26] != 0);
			//	button #3: 27 Y
			buttons[3] = (buffer[27] != 0);
			//	button #4: 28 Black
			buttons[4] = (buffer[28] != 0);
			//	button #5: 29 White
			buttons[5] = (buffer[29] != 0);
			//	button #6: 40 Start
			buttons[6] = ((buffer[22] & 0x10) != 0);
			//	button #7: 80 Back
			buttons[7] = ((buffer[22] & 0x20) != 0);
			//	button #8:  left joy
			buttons[8] = ((buffer[22] & 0x40) != 0);
			//	button #9:  right joy
			buttons[9] = ((buffer[22] & 0x80) != 0);
			//	button #10:30 left trigger
			buttons[10] = (buffer[30] != 0);
			//	button #11:31 right trigger
			buttons[11] = (buffer[31] != 0);
#else
			vrpn_uint8 value, mask;
			value = buffer[1];
			for (int btn = 0; btn < 8; btn++)
			{
				mask = static_cast<vrpn_uint8>(1 << (btn % 8));
				buttons[btn] = ((value & mask) != 0);
			}
			value = buffer[2];
			for (int btn = 0; btn < 4; btn++)
			{
				mask = static_cast<vrpn_uint8>(1 << (btn % 8));
				buttons[8 + btn] = ((value & mask) != 0);
			}
#endif // OLD_DATA

			// Point of View Hat
#ifdef OLD_DATA
			buttons[12] = ((buffer[22] & 0x01) != 0);	// Up
			buttons[14] = ((buffer[22] & 0x02) != 0);	// Down
			buttons[15] = ((buffer[22] & 0x04) != 0);	// Left
			buttons[13] = ((buffer[22] & 0x08) != 0);	// Right
#else
			// Point of View Hat
			buttons[12] = buttons[13] = buttons[14] = buttons[15] = 0;
			switch (buffer[18] & 0x0f)
			{
			case 0:		// up
				buttons[12] = true;
				break;
			case 1:
				buttons[12] = buttons[13] = true;
				break;
			case 2:		// right
				buttons[13] = true;
				break;
			case 3:
				buttons[13] = buttons[14] = true;
				break;
			case 4:		// down
				buttons[14] = true;
				break;
			case 5:
				buttons[14] = buttons[15] = true;
				break;
			case 6:		// left
				buttons[15] = true;
				break;
			case 7:
				buttons[15] = buttons[12] = true;
				break;
			case 8:
			default:
				// nothing to do
				break;
			}
#endif // OLD_DATA
			channel[4] = normalize_dpad(buttons[12], buttons[13], buttons[14], buttons[15]);
		} else {
			fprintf(stderr, "vrpn_Microsoft_Controller_Raw_Xbox_S: Unknown report = %u\n", static_cast<unsigned>(buffer[0]));
		}
	} else {
		fprintf(stderr, "vrpn_Microsoft_Controller_Raw_Xbox_S: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

//////////////////////////////////////////////////////////////////////////
// Xbox 360
//////////////////////////////////////////////////////////////////////////
vrpn_Microsoft_Controller_Raw_Xbox_360_base::vrpn_Microsoft_Controller_Raw_Xbox_360_base(const char *name, vrpn_Connection *c, vrpn_uint16 vendor, vrpn_uint16 product)
    : vrpn_Microsoft_Controller_Raw(new vrpn_HidProductAcceptor(vendor, product), name, c, vendor, product)
, vrpn_Analog(name, c)
, vrpn_Button_Filter(name, c)
, vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 6;
	vrpn_Dial::num_dials = 0;
	vrpn_Button::num_buttons = 18;

	// Initialize the state of all the analogs, buttons, and dials
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Microsoft_Controller_Raw_Xbox_360_base::mainloop()
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

void vrpn_Microsoft_Controller_Raw_Xbox_360_base::report(vrpn_uint32 class_of_service) {
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

void vrpn_Microsoft_Controller_Raw_Xbox_360_base::report_changes(vrpn_uint32 class_of_service) {
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

void vrpn_Microsoft_Controller_Raw_Xbox_360_base::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Decode all full reports, each of which is 14 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	/*
	0: left-X
	1: "
	2: left-Y
	3: "
	4: right-X
	5: "
	6: right-Y
	7: "
	8: 00/80: not sure with left & right triggers
	9: 80 normal, ...ff=left trigger, ...00=right trigger
	10: Button bit flags: 01=A, ..., 80=start
	11: Button bit flags: 01=left joy, 02=right joy, 04=hat up add 04 every 45 degrees right
	12: varies with everything
	13: "
	??: not sure where Xbox button is
	*/
	// XXX Check to see that this works with HIDAPI, there may be two smaller reports.
	if (bytes == 14) {
		if (true) {
			vrpn_uint8 *bufptr;

			// Report joystick axes as analogs
			//	left joy left/right
			vrpn_uint16 x, y;
			bufptr = &buffer[0];
			x = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr);
			//	left joy up/down
			bufptr = &buffer[2];
			y = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr);
			normalize_axes(x, y, MS_GAMEPAD_LEFT_THUMB_DEADZONE, 1.0f, channel[0], channel[1]);
			//	right joy left/right
			bufptr = &buffer[4];
			x = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr);
			//	right joy up/down
			bufptr = &buffer[6];
			y = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr);
			normalize_axes(x, y, MS_GAMEPAD_RIGHT_THUMB_DEADZONE, 1.0f, channel[2], channel[3]);

			// triggers: left goes positive, right goes negative
			channel[4] = normalize_trigger(buffer[9]);

			vrpn_uint8 value;
			value = buffer[10];
			for (int btn = 0; btn < 8; btn++) {
				/*
				1: A
				2: B
				3: X
				4: Y
				5: left bump
				6: right bump
				7: back
				8: start
				*/
				vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << (btn % 8));
				buttons[btn] = ((value & mask) != 0);
			}
			value = buffer[11];
			for (int btn = 0; btn < 2; btn++) {
				/*
				9: left joy
				10: right joy
				*/
				vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << (btn % 8));
				buttons[8 + btn] = ((value & mask) != 0);
			}
			value &= 0x3c;	// remove joystick buttons and isolate just the "Point of View Hat"
			value >>= 2;
			/*
			11: 04 (>>2=1, >>3=0) hat up (0)
				08 (>>2=2, >>3=1) up-right (45)
			12: 0c (>>2=3, >>3=1) hat right (90)
				10 (>>2=4, >>3=2) right-down (135)
			13: 14 (>>2=5, >>3=2) hat down (180)
				18 (>>2=6, >>3=3) down-left (225)
			14: 1c (>>2=7, >>3=3) hat left (270)
				20 (>>2=8, >>3=4) left-up (315)
			*/
			buttons[10] = buttons[11] = buttons[12] = buttons[13] = false;
			if (value != 0)
			{
				//int lowerBtn = (10 + (value >> 3)) & 0x03;
				switch (value)
				{
				case 1:
					buttons[10] = true;
					break;
				case 2:
					buttons[10] = buttons[11] = true;
					break;
				case 3:
					buttons[11] = true;
					break;
				case 4:
					buttons[11] = buttons[12] = true;
					break;
				case 5:
					buttons[12] = true;
					break;
				case 6:
					buttons[12] = buttons[13] = true;
					break;
				case 7:
					buttons[13] = true;
					break;
				case 8:
					buttons[13] = buttons[10] = true;
					break;
				}
			}
			channel[5] = normalize_dpad(buttons[10], buttons[11], buttons[12], buttons[13]);
		} else {
			fprintf(stderr, "vrpn_Microsoft_Controvrpn_Microsoft_Controller_Raw_Xbox_360_baseller_Raw_Xbox_360: Unknown report = %u\n", static_cast<unsigned>(buffer[0]));
		}
	} else {
		fprintf(stderr, "vrpn_Microsoft_Controller_Raw_Xbox_360_base: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// The original Xbox_360.  We just declare the vendor and product ID.
vrpn_Microsoft_Controller_Raw_Xbox_360::vrpn_Microsoft_Controller_Raw_Xbox_360(const char *name, vrpn_Connection *c)
    : vrpn_Microsoft_Controller_Raw_Xbox_360_base(name, c, MICROSOFT_VENDOR, XBOX_360)
{
}

// The wireless Xbox_360.  We just declare the vendor and product ID.
vrpn_Microsoft_Controller_Raw_Xbox_360_Wireless::vrpn_Microsoft_Controller_Raw_Xbox_360_Wireless(const char *name, vrpn_Connection *c)
    : vrpn_Microsoft_Controller_Raw_Xbox_360_base(name, c, MICROSOFT_VENDOR, XBOX_360_WIRELESS)
{
}

// The device otherwise behaves exactly like an Xbox_360, so we re-use all of
// the functions from that class.  We just declare the vendor and product ID.
vrpn_Afterglow_Ax1_For_Xbox_360::vrpn_Afterglow_Ax1_For_Xbox_360(const char *name, vrpn_Connection *c)
    : vrpn_Microsoft_Controller_Raw_Xbox_360_base(name, c, AFTERGLOW_VENDOR, AX1_FOR_XBOX_360)
{
}

// End of VRPN_USE_HID
#endif
