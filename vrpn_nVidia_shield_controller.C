// vrpn_nVidia_shield_controller.C: VRPN driver for nVidia shield devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_nVidia_shield_controller.h"
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, report.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 NVIDIA_VENDOR = 0x955;
static const vrpn_uint16 NVIDIA_SHIELD_USB = 0x7210;
static const vrpn_uint16 NVIDIA_SHIELD_STEALTH_USB = 0x7214;

vrpn_nVidia_shield::vrpn_nVidia_shield(vrpn_HidAcceptor *filter,
    const char *name, vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , d_filter(filter)
{
	init_hid();
}

vrpn_nVidia_shield::~vrpn_nVidia_shield(void)
{
  try {
    delete d_filter;
  } catch (...) {
    fprintf(stderr, "vrpn_nVidia_shield::~vrpn_nVidia_shield(): delete failed\n");
    return;
  }
}

void vrpn_nVidia_shield::init_hid(void) {
}

void vrpn_nVidia_shield::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

vrpn_nVidia_shield_USB::vrpn_nVidia_shield_USB(const char *name, vrpn_Connection *c)
    : vrpn_nVidia_shield(new vrpn_HidProductAcceptor(NVIDIA_VENDOR, NVIDIA_SHIELD_USB), name, c, NVIDIA_VENDOR, NVIDIA_SHIELD_USB)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
{
  vrpn_Analog::num_channel = 10;
  vrpn_Button::num_buttons = 21;

  // Initialize the state of all the analogs and buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_nVidia_shield_USB::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, d_timestamp) > POLL_INTERVAL ) {
		d_timestamp = current_time;
		report_changes();

                // Call the server_mainloop on our unique base class.
                server_mainloop();
	}
}

void vrpn_nVidia_shield_USB::report(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = d_timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = d_timestamp;
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

void vrpn_nVidia_shield_USB::report_changes(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = d_timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = d_timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report_changes(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_nVidia_shield_USB::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // There are two types of reports, type 1 is 15 bytes long (plus the
  // type byte) and type 2 is 5 bytes long (plus the type byte)
  // (On Linux, this shows up as a 16-byte message, even when it is
  // type 2, on the mac it shows up as 6.)

  if ( (bytes >= 6) && (buffer[0] == 2) ) {

    // 6-byte reports.  Thumb touch pad.
    // Byte 0 is 02 (report type 2)
    // Byte 1:
    //   Bit 0: pad button pressed
    //   Bit 7: finger touching pad
    // Byte 2 seems to be X position, near 39 to left and near DF to right
    // Byte 3 is 00
    // Byte 4 seems to be Y position, near 33 at top and near 5B at bottom
    // Byte 5 is 00

    buttons[8] = ( (buffer[1] & (1 << 0)) != 0);
    buttons[20] = ( (buffer[1] & (1 << 7)) != 0);
    channel[6] = buffer[2] / 255.0;
    channel[7] = buffer[4] / 255.0;

  } else if ( (bytes == 16) && (buffer[0] == 1) ) {

    // 16-byte reports.
    // Byte 0 is 01 (report type 1)

    // Byte 1:
    //  Bit 0: A button
    //  Bit 1: B button
    //  Bit 2: X button
    //  Bit 3: Y button
    //  Bit 4: Left finger trigger button
    //  Bit 5: Right finger trigger button
    //  Bit 6: Left joystick button
    //  Bit 7: Right joystick button
    // Byte 2:
    //  Bit 0: Touch pad button pressed or center of volume control pressed
    //         or left of volume control pressed or right of volume control pressed
    //  Bit 1: Play/pause button pressed
    //  Bit 3: Right of volume control (+) pressed
    //  Bit 4: Left of volume control (-) pressed
    //  Bit 5: Shield emblem pressed
    //  Bit 6: Go Back button pressed
    //  Bit 7: Home button pressed

    int first_byte = 1;
    int num_bytes = 2;
    int first_button = 0;
    for (int byte = first_byte; byte < first_byte + num_bytes; byte++) {
      vrpn_uint8 value = buffer[byte];
      for (int btn = 0; btn < 8; btn++) {
        vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << btn);
        buttons[8*(byte - first_byte) + btn + first_button] = ((value & mask) != 0);
      }

    }

    // Byte 3: Left hi-hat:
    //  0F: Nothing pressed
    //  0 = North, 1 = NE, 2 = E, 3 = SE, 4 = S, 5 = SW, 6 = W, 7 = NW
    //  This is encoded as buttons 16 (up), 17 (right), 18 (down), and 19 (left)
    //  It is also encoded as two analogs: 8 (X, -1 left 1 right) and
    //    9 (Y, -1 up 1 down).
    switch (buffer[3]) {
    case 0:
      buttons[16] = 1;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = -1;
      break;

    case 1:
      buttons[16] = 1;
      buttons[17] = 1;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = -1;
      break;

    case 2:
      buttons[16] = 0;
      buttons[17] = 1;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = 0;
      break;

    case 3:
      buttons[16] = 0;
      buttons[17] = 1;
      buttons[18] = 1;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = 1;
      break;

    case 4:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 1;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = 1;
      break;

    case 5:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 1;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = 1;
      break;

    case 6:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = 0;
      break;

    case 7:
      buttons[16] = 1;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = -1;
      break;

    default:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = 0;
      break;
    }

    // Left stick X axis is bytes 4-5, least byte first, FF 7F in middle
    //  00 00 to left, FF FF to right
    // Left stick Y axis is bytes 6-7, 00 00 is up and FF FF is down
    // Right stick X axis is bytes 8-9, 00 00 is left
    // Right stick Y axis is bytes 10-11, 00 00 is up
    int first_joy_axis = 4;
    int num_joy_axis = 4;
    int first_analog = 0;
    vrpn_uint8 *bufptr = &buffer[4];
    for (int axis = first_joy_axis; axis < first_joy_axis + num_joy_axis; axis++) {
      vrpn_uint16 raw_val = vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr);
      vrpn_int32 signed_val = raw_val - static_cast<int>(32767);
      double value = signed_val / 32768.0;
      channel[first_analog + axis - first_joy_axis] = value;
    }

    // Left analog finger trigger is bytes 12-13, 00 00 is out, FF FF is in
    // Right analog finger trigger is bytes 14-15, 00 00 is out, FF FF is in
    int first_trigger = 4;
    int num_trigger = 2;
    first_analog = 4;
    bufptr = &buffer[12];
    for (int trig = first_trigger; trig < first_trigger + num_trigger; trig++) {
      vrpn_uint16 raw_val = vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr);
      double value = raw_val / 65535.0;
      channel[first_analog + trig - first_trigger] = value;
    }

	} else {
    vrpn_uint8 type = 0;
    if (bytes > 0) { type = buffer[0]; }
		fprintf(stderr, "vrpn_nVidia_shield_USB: Unrecognized report type (%u); # total bytes = %u\n", type, static_cast<unsigned>(bytes));
	}

}

vrpn_nVidia_shield_stealth_USB::vrpn_nVidia_shield_stealth_USB(const char *name, vrpn_Connection *c)
    : vrpn_nVidia_shield(new vrpn_HidProductAcceptor(NVIDIA_VENDOR, NVIDIA_SHIELD_STEALTH_USB), name, c, NVIDIA_VENDOR, NVIDIA_SHIELD_STEALTH_USB)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
{
  vrpn_Analog::num_channel = 10;
  vrpn_Button::num_buttons = 21;

  // Initialize the state of all the analogs and buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_nVidia_shield_stealth_USB::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, d_timestamp) > POLL_INTERVAL ) {
		d_timestamp = current_time;
		report_changes();

                // Call the server_mainloop on our unique base class.
                server_mainloop();
	}
}

void vrpn_nVidia_shield_stealth_USB::report(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = d_timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = d_timestamp;
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

void vrpn_nVidia_shield_stealth_USB::report_changes(vrpn_uint32 class_of_service) {
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = d_timestamp;
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::timestamp = d_timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report_changes(class_of_service);
	}
	if (vrpn_Button::num_buttons > 0)
	{
		vrpn_Button::report_changes();
	}
}

void vrpn_nVidia_shield_stealth_USB::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // There is one type of report, type 1 is 32 bytes long (plus the
  // type byte).  On Windows, the packet is 232 bytes long (plus the
  // type byte).

  if ( (bytes >= 33) && (buffer[0] == 1) ) {

    // 32-byte reports.
    // Byte 0 is 01 (report type 1)
    // Byte 1 is a counter index on reports that loops

    // Byte 3:
    //  Bit 0: A button
    //  Bit 1: B button
    //  Bit 2: X button
    //  Bit 3: Y button
    //  Bit 4: Left finger trigger button
    //  Bit 5: Right finger trigger button
    //  Bit 6: Left joystick button
    //  Bit 7: Right joystick button

    int first_byte = 3;
    int num_bytes = 1;
    int first_button = 0;
    for (int byte = first_byte; byte < first_byte + num_bytes; byte++) {
      vrpn_uint8 value = buffer[byte];
      for (int btn = 0; btn < 8; btn++) {
        vrpn_uint8 mask = static_cast<vrpn_uint8>(1 << btn);
        buttons[8*(byte - first_byte) + btn + first_button] = ((value & mask) != 0);
      }
    }

    // Byte 4:
    //  Bit 0: Play/pause button
    buttons[9] = buffer[4] & 0x01;

    // Byte 17:
    //  Bit 0: Circle button pressed
    //  Bit 1: Left-arrow button pressed
    //  Bit 2: Shield emblem pressed
    buttons[12] = buffer[17] & 0x01;
    buttons[11] = buffer[17] & 0x02;
    buttons[13] = buffer[17] & 0x04;

    // Byte 2: Left hi-hat:
    //  80: Nothing pressed
    //  0 = North, 1 = NE, 2 = E, 3 = SE, 4 = S, 5 = SW, 6 = W, 7 = NW
    //  This is encoded as buttons 16 (up), 17 (right), 18 (down), and 19 (left)
    //  It is also encoded as two analogs: 8 (X, -1 left 1 right) and
    //    9 (Y, -1 up 1 down).
    switch (buffer[2]) {
    case 0:
      buttons[16] = 1;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = -1;
      break;

    case 1:
      buttons[16] = 1;
      buttons[17] = 1;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = -1;
      break;

    case 2:
      buttons[16] = 0;
      buttons[17] = 1;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = 0;
      break;

    case 3:
      buttons[16] = 0;
      buttons[17] = 1;
      buttons[18] = 1;
      buttons[19] = 0;
      channel[8] = 1;
      channel[9] = 1;
      break;

    case 4:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 1;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = 1;
      break;

    case 5:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 1;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = 1;
      break;

    case 6:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = 0;
      break;

    case 7:
      buttons[16] = 1;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 1;
      channel[8] = -1;
      channel[9] = -1;
      break;

    default:
      buttons[16] = 0;
      buttons[17] = 0;
      buttons[18] = 0;
      buttons[19] = 0;
      channel[8] = 0;
      channel[9] = 0;
      break;
    }

    // Bytes 9 and 10 are a 16-bit left-joystick X axis analog, byte 10 MSB
    //   00 00 to the left, ff ff to the right, around 00 80 centered
    // Bytes 11 and 12 are a 16-bit left-joystick Y axis analog, byte 12 MSB
    //   00 00 up, ff ff down, around 00 80 centered
    // Bytes 13 and 14 are a 16-bit right-joystick X axis analog, byte 14 MSB
    //   00 00 to the left, ff ff to the right, around 00 80 centered
    // Bytes 15 and 16 are a 16-bit right-joystick Y axis analog, byte 16 MSB
    //   00 00 up, ff ff down, around 00 80 centered
    int first_joy_axis = 0;
    int num_joy_axis = 4;
    int first_analog = 0;
    vrpn_uint8 *bufptr = &buffer[9];
    for (int axis = first_joy_axis; axis < first_joy_axis + num_joy_axis; axis++) {
      vrpn_uint16 raw_val = vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr);
      vrpn_int32 signed_val = raw_val - static_cast<int>(32767);
      double value = signed_val / 32768.0;
      channel[first_analog + axis - first_joy_axis] = value;
    }

    // Bytes 5 and 6 are a 16-bit left-finger analog, byte 6 MSB
    // Bytes 7 and 8 are a 16-bit right-finger analog, byte 8 MSB
    int first_trigger = 0;
    int num_trigger = 2;
    first_analog = 4;
    bufptr = &buffer[5];
    for (int trig = first_trigger; trig < first_trigger + num_trigger; trig++) {
      vrpn_uint16 raw_val = vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr);
      double value = raw_val / 65535.0;
      channel[first_analog + trig - first_trigger] = value;
    }

	} else {
    vrpn_uint8 type = 0;
    if (bytes > 0) { type = buffer[0]; }
		fprintf(stderr, "vrpn_nVidia_shield_stealth_USB: Unrecognized report type (%u); # total bytes = %u\n", type, static_cast<unsigned>(bytes));
	}

}

// End of VRPN_USE_HID
#endif
