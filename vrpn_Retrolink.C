// vrpn_Retrolink.C: VRPN driver for Retrolink Classic Controller devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_Retrolink.h"
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 RETROLINK_VENDOR = 0x0079;
static const vrpn_uint16 RETROLINK_GAMECUBE = 0x0006;
static const vrpn_uint16 RETROLINK_GENESIS = 0x0011;

// A channel goes from -1 (value 0) to 1 (value 255), with
// an initial value between 0x80 and 0x83 for the one I developed
// with.
// XXX Consider adding a dead zone and putting it in the config file
static vrpn_float64 normalize_axis(const vrpn_uint8 value)
{
	vrpn_float64 offset = static_cast<vrpn_float64>(value) - 128;
	vrpn_float64 scaled = offset / 127;
	if (scaled > 1) { scaled = 1; }
	if (scaled < -1) { scaled = -1; }
	return scaled;
}

// Convert the 0-f nybble for the rocker switch into its angle
static void angle_and_buttons_from_rocker_byte(const vrpn_uint8 value,
	vrpn_float64 *angle,
	bool *up, bool *right, bool *down, bool *left)
{
	switch (value) {
	case 0: *angle = 0;   *up = true; *right = false; *down = false; *left = false; break;
	case 1: *angle = 45;  *up = true; *right = true; *down = false; *left = false; break;
	case 2: *angle = 90;  *up = false; *right = true; *down = false; *left = false; break;
	case 3: *angle = 135; *up = false; *right = true; *down = true; *left = false; break;
	case 4: *angle = 180; *up = false; *right = false; *down = true; *left = false; break;
	case 5: *angle = 225; *up = false; *right = false; *down = true; *left = true; break;
	case 6: *angle = 270; *up = false; *right = false; *down = false; *left = true; break;
	case 7: *angle = 315; *up = true; *right = false; *down = false; *left = true; break;
	default: *angle = -1; *up = false; *right = false; *down = false; *left = false; break;
	}
}

// Convert the two bytes for the rocker switch into its angle
static void angle_and_buttons_from_rocker_doublet(
	const vrpn_uint8 value0, const vrpn_uint8 value1,
	vrpn_float64 *angle,
	bool *up, bool *right, bool *down, bool *left)
{
	vrpn_uint8 value = 8; // Default is  nothing set in case we don't recognize it.
	switch (value0) {
	case 0x00:
		switch (value1) {
		case 0x00: value = 7; break;
		case 0x7F: value = 6; break;
		case 0xFF: value = 5; break;
		}
		break;

	case 0x7F:
		switch (value1) {
		case 0x00: value = 0; break;
		case 0x7F: value = 8; break;
		case 0xFF: value = 4; break;
		}
		break;

	case 0xFF:
		switch (value1) {
		case 0x00: value = 1; break;
		case 0x7F: value = 2; break;
		case 0xFF: value = 3; break;
		}
		break;
	}
	angle_and_buttons_from_rocker_byte(value, angle, up, right, down, left);
}

vrpn_Retrolink::vrpn_Retrolink(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();
}

vrpn_Retrolink::~vrpn_Retrolink(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Retrolink::~vrpn_Retrolink(): delete failed\n");
    return;
  }
}

void vrpn_Retrolink::init_hid(void) {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Retrolink::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Retrolink::on_last_disconnect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Retrolink::on_connect(void* /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

vrpn_Retrolink_GameCube::vrpn_Retrolink_GameCube(const char *name, vrpn_Connection *c)
    : vrpn_Retrolink(new vrpn_HidProductAcceptor(RETROLINK_VENDOR, RETROLINK_GAMECUBE), name, c, RETROLINK_VENDOR, RETROLINK_GAMECUBE)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
{
  vrpn_Analog::num_channel = 5;
  vrpn_Button::num_buttons = 12;

  // Initialize the state of all the analogs, buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
}

void vrpn_Retrolink_GameCube::mainloop(void)
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

void vrpn_Retrolink_GameCube::report(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_Retrolink_GameCube::report_changes(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_Retrolink_GameCube::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	/*
	// Print the report so we can figure out what is going on.
	for (size_t i = 0; i < bytes; i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
	return;
	*/

	// Because there is only one type of report, the initial "0" report-type
	// byte is removed by the HIDAPI driver.
	// Reports should be 8 bytes.  They are encoded as follows:
	// 0 = Left joystick X axis, goes from 0 (left) to 255 (right)
	// 1 = Left joystick Y axis, goes from 0 (up) to 255 (down)
	// 2 = Uncontrolled jibberish, must be from an unconnected channel
	// 3 = Left joystick X axis, goes from 0 (left) to 255 (right)
	// 4 = Left joystick Y axis, goes from 0 (left) to 255 (right)
	// 5 upper nybble = bit 0 (Y), 1 (X), 2 (A), 3 (B)
	// 5 lower nybble = rocker: 255 when nothing; 0 (up), 1 (UR), 2 (right), .. 7 (UL)
	// 6 = bit # 0 (left trigger), 1 (right trigger), 2 (Z), 5 (Start/pause)
	// 7 = always 0x20 (maybe unconnected buttons; probably safest to ignore)

	// Controls for the Nintendo 64 classic
	// 0 = Left joystick X axis, goes from 0 (left) to 255 (right)
	// 1 = Left joystick Y axis, goes from 0 (up) to 255 (down)
	// 2 = Uncontrolled jibberish, must be from an unconnected channel
	// 5 upper nybble = bit 0 (C up), 1 (C right), 2 (C down), 3 (C Left)
	// 5 lower nybble = rocker: 255 when nothing; 0 (up), 1 (UR), 2 (right), .. 7 (UL)
	// 6 = bit # 0 (left trigger), 1 (right trigger), 2 (A), 3 (Z), 4 (B), 5 (Start)
	// 7 = always 0x20 (maybe unconnected buttons; probably safest to ignore)

	if (bytes == 8) {
		// Figure out the joystick axes.
		channel[0] = normalize_axis(buffer[0]);
		channel[1] = normalize_axis(buffer[1]);
		channel[2] = normalize_axis(buffer[3]);
		channel[3] = normalize_axis(buffer[4]);

		// Figure out the buttons.
		buttons[0] = (buffer[5] & (1 << 4)) != 0;
		buttons[1] = (buffer[5] & (1 << 5)) != 0;
		buttons[2] = (buffer[5] & (1 << 6)) != 0;
		buttons[3] = (buffer[5] & (1 << 7)) != 0;
		buttons[4] = (buffer[6] & (1 << 0)) != 0;
		buttons[5] = (buffer[6] & (1 << 1)) != 0;
		buttons[6] = (buffer[6] & (1 << 2)) != 0;
		buttons[7] = (buffer[6] & (1 << 5)) != 0;

		// Figure out the rocker.
		vrpn_uint8 rocker = buffer[5] & 0x0f;
		vrpn_float64 angle;
		bool up, right, down, left;
		angle_and_buttons_from_rocker_byte(rocker, &angle, &up, &right, &down, &left);
		channel[4] = angle;
		buttons[8] = up; buttons[9] = right;
		buttons[10] = down;
		buttons[11] = left;
	} else {
		fprintf(stderr, "vrpn_Retrolink_GameCube: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

vrpn_Retrolink_Genesis::vrpn_Retrolink_Genesis(const char *name, vrpn_Connection *c)
    : vrpn_Retrolink(new vrpn_HidProductAcceptor(RETROLINK_VENDOR, RETROLINK_GENESIS), name, c, RETROLINK_VENDOR, RETROLINK_GENESIS)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
{
  vrpn_Analog::num_channel = 1;
  vrpn_Button::num_buttons = 12;

  // Initialize the state of all the analogs, buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
}

void vrpn_Retrolink_Genesis::mainloop(void)
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

void vrpn_Retrolink_Genesis::report(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_Retrolink_Genesis::report_changes(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_Retrolink_Genesis::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	/*
	// Print the report so we can figure out what is going on.
	for (size_t i = 0; i < bytes; i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
	return;
	*/

	// Because there is only one type of report, the initial "0" report-type
	// byte is removed by the HIDAPI driver.
	// Reports should be 8 bytes.  They are encoded as follows:
	// 0 = No input (has the value 01)
	// 1 = No input (has the value 7F)
	// 2 = No input (has the value 7F)
	// 3 = Rocker X axis, goes from 00 (left) to 7F (center) to FF (right)
	// 4 = Rocker Y axis, goes from 00 (top) to 7F (center) to FF (bottom)
	// 5 = bit # 4 (Y), 5 (B), 6 (A), 7 (X); lower nybble is F
	// 6 = bit # 0 (Z), 1 (C), 4 (Mode), 5 (Start)
	// 7 = always 0x00 (maybe unconnected buttons; probably safest to ignore)

	if (bytes == 8) {
		// Figure out the buttons.
		buttons[0] = (buffer[5] & (1 << 6)) != 0;
		buttons[1] = (buffer[5] & (1 << 5)) != 0;
		buttons[2] = (buffer[6] & (1 << 1)) != 0;
		buttons[3] = (buffer[5] & (1 << 7)) != 0;
		buttons[4] = (buffer[5] & (1 << 4)) != 0;
		buttons[5] = (buffer[6] & (1 << 0)) != 0;
		buttons[6] = (buffer[6] & (1 << 4)) != 0;
		buttons[7] = (buffer[6] & (1 << 5)) != 0;

		// Figure out the rocker.
		vrpn_uint8 rocker0 = buffer[3];
		vrpn_uint8 rocker1 = buffer[4];
		vrpn_float64 angle;
		bool up, right, down, left;
		angle_and_buttons_from_rocker_doublet(rocker0, rocker1,
			&angle, &up, &right, &down, &left);
		channel[0] = angle;
		buttons[8] = up;
		buttons[9] = right;
		buttons[10] = down;
		buttons[11] = left;
	} else {
		fprintf(stderr, "vrpn_Retrolink_Genesis: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
	}
}

// End of VRPN_USE_HID
#endif
