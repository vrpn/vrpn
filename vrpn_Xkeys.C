// vrpn_Xkeys.C: VRPN driver for P.I. Engineering's X-Keys devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset

#include "vrpn_Xkeys.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 XKEYS_VENDOR = 0x05F3;
static const vrpn_uint16 XKEYS_DESKTOP = 0x0281;
static const vrpn_uint16 XKEYS_JOG_AND_SHUTTLE = 0x0241;
static const vrpn_uint16 XKEYS_PRO = 0x0291;
static const vrpn_uint16 XKEYS_JOYSTICK = 0x0251;
static const vrpn_uint16 XKEYS_XK3 = 0x042C;
static const vrpn_uint16 XKEYS_JOG_AND_SHUTTLE12 = 0x0426;
static const vrpn_uint16 XKEYS_JOG_AND_SHUTTLE68 = 0x045a;
static const vrpn_uint16 XKEYS_JOYSTICK12 = 0x0429;

vrpn_Xkeys::vrpn_Xkeys(vrpn_HidAcceptor *filter, const char *name,
    vrpn_Connection *c, vrpn_uint16 vendor, vrpn_uint16 product,
    bool toggle_light)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
  , _toggle_light(toggle_light)
{
	init_hid();
}

vrpn_Xkeys::~vrpn_Xkeys()
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Xkeys::~vrpn_Xkeys(): delete failed\n");
    return;
  }
}

void vrpn_Xkeys::init_hid() {
  // Get notifications when clients connect and disconnect
  register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
  register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Xkeys::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Xkeys::on_last_disconnect(void *thisPtr, vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Xkeys *me = static_cast<vrpn_Xkeys *>(thisPtr);
	if (me->_toggle_light) {
		// Set light to red to indicate we have no active connections
		me->setLEDs(On, Off);
	}
	return 0;
}

int vrpn_Xkeys::on_connect(void *thisPtr, vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Xkeys *me = static_cast<vrpn_Xkeys *>(thisPtr);

	if (me->_toggle_light) {
		// Set light to green to indicate we have an active connection
		me->setLEDs(Off, On);
	}
	return 0;
}

void vrpn_Xkeys_v1::setLEDs(LED_STATE red, LED_STATE green)
{
  vrpn_uint8 outputs[9] = {0};
  switch (red) {
    case Off:  // do not set anything to 1 for off.
      break;
    case On:
    case Flash:
      outputs[8] |= 128;
      break;
    default:   // For both on and flash, turn on (we don't know how to flash)
      fprintf(stderr,"vrpn_Xkeys_v2::setLED(): Unrecognized state\n");
  }
  switch (green) {
    case Off:  // do not set anything to 1 for off.
      break;
    case On:
    case Flash:
      outputs[8] |= 64;
      break;
    default:   // For both on and flash, turn on (we don't know how to flash)
      fprintf(stderr,"vrpn_Xkeys_v2::setLED(): Unrecognized state\n");
  }

  // Send the combined command to turn on and off both red and green.
  send_data(9, outputs);
}

void vrpn_Xkeys_v2::setLEDs(LED_STATE red, LED_STATE green)
{
  vrpn_uint8 outputs[36] = {0};
  outputs[1] = 0xb3;

  // Send the red command.
  outputs[2] = 7;        // 6 = green LED, 7 = red LED
  switch (red) {
    case On:  // do not set anything to 1 for off.
      outputs[3] = 1;        // 0 = off, 1 = on, 2 = flash
      break;
    case Flash:  // do not set anything to 1 for off.
      outputs[3] = 2;        // 0 = off, 1 = on, 2 = flash
      break;
    case Off:  // do not set anything to 1 for off.
      outputs[3] = 0;        // 0 = off, 1 = on, 2 = flash
      break;
    default:   // Never heard of this state.
      fprintf(stderr,"vrpn_Xkeys_v2::setLED(): Unrecognized state\n");
  }
  send_data(36, outputs);

  // Send the green command.
  outputs[2] = 6;        // 6 = green LED, 7 = red LED
  switch (green) {
    case On:  // do not set anything to 1 for off.
      outputs[3] = 1;        // 0 = off, 1 = on, 2 = flash
      break;
    case Flash:  // do not set anything to 1 for off.
      outputs[3] = 2;        // 0 = off, 1 = on, 2 = flash
      break;
    case Off:  // do not set anything to 1 for off.
      outputs[3] = 0;        // 0 = off, 1 = on, 2 = flash
      break;
    default:   // Never heard of this state.
      fprintf(stderr,"vrpn_Xkeys_v2::setLED(): Unrecognized state\n");
  }
  send_data(36, outputs);
}

vrpn_Xkeys_Desktop::vrpn_Xkeys_Desktop(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v1(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_DESKTOP), name, c, XKEYS_VENDOR, XKEYS_DESKTOP)
  , vrpn_Button_Filter(name, c)
{
  // 21 buttons (don't forget about button 0)
  vrpn_Button::num_buttons = 21;

  // Initialize the state of all the buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
}

void vrpn_Xkeys_Desktop::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Desktop::report(void)
{
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Desktop::report_changes(void)
{
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Desktop::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Decode all full reports, each of which is 11 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	for (size_t i = 0; i < bytes / 11; i++) {
		vrpn_uint8 *report = buffer + (i * 11);

		if (!(report[10] & 0x08)) {
			// Apparently we got a corrupted report; skip this one.
			fprintf(stderr, "vrpn_Xkeys_Desktop: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			continue;
		}

		// Decode the "programming switch"
		buttons[0] = (report[10] & 0x10) != 0;

		// Decode the other buttons into column-major order
		for (int btn = 0; btn < 20; btn++) {
			vrpn_uint8 *offset, mask;
			
			offset = report + btn / 5;
			mask = static_cast<vrpn_uint8>(1 << (btn % 5));

			buttons[btn + 1] = (*offset & mask) != 0;
		}
	}
}

vrpn_Xkeys_Jog_And_Shuttle::vrpn_Xkeys_Jog_And_Shuttle(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v1(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE), name, c, XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 59;  // Don't forget button 0

	// Initialize the state of all the analogs, buttons, and dials
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Xkeys_Jog_And_Shuttle::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Jog_And_Shuttle::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

void vrpn_Xkeys_Jog_And_Shuttle::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void vrpn_Xkeys_Jog_And_Shuttle::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Decode all full reports.
        // Full reports for all of the pro devices are 15 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver, leaving 14 bytes.
	// Also, it appears as though the
	// remaining 14-byte command is split into two, one 8-byte that is sent
	// first and then a 6-byte that is sent later.  So we need to check the
	// length of the packet to see which it is and then parse it appropriately.
	if (bytes == 8) {	// The first 8 bytes of a report (happens on Linux)

                // Report jog dial as analog and dial
                // Report shuttle knob as analog

                // Double cast on channel 0 ensures negative values stay negative
                channel[0] = static_cast<float>(static_cast<signed char>(buffer[0])) / 7.0;
                channel[1] = static_cast<float>(buffer[1]);

                // Do the unsigned/signed conversion at the last minute so the
                // signed values work properly.
                dials[0] = static_cast<vrpn_int8>(buffer[1] - _lastDial) / 256.0;

                // Store the current dial position for the next delta
                _lastDial = buffer[1];

		// Decode the other buttons in column-major order.  We skip the
		// first three bytes, which record the joystick value or the
		// shuttle/jog value (depending on device).
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The first 35 buttons are in this report, the remaining 23 in the next.
		for (int btn = 0; btn < 35; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 3;
			mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 6) {	// The last 6 bytes of a report (happens on Linux)

		if (!(buffer[5] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			return;
		}

		// Decode the "programming switch"
		buttons[0] = (buffer[5] & 0x10) != 0;

		// Decode the other buttons in column-major order.
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The last 23 buttons are in this report, the remaining 23 in the next.
		for (int btn = 35; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;
			int local_btn = btn - 35;

			offset = buffer + local_btn / 7;
			mask = static_cast<vrpn_uint8>(1 << (local_btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 14) {	// A full report in one swoop (happens on Windows)
          // Full reports for all of the pro devices are 15 bytes long.
          // Because there is only one type of report, the initial "0" report-type
          // byte is removed by the HIDAPI driver, leaving 14 bytes.
          size_t i;
	  for (i = 0; i < bytes / 14; i++) {
		  vrpn_uint8 *report = buffer + (i * 14);

		  if (!(report[13] & 0x08)) {
			  // Garbled report; skip it
			  fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			  continue;
		  }

		  // Decode the "programming switch"
		  buttons[0] = (report[13] & 0x10) != 0;

		  // Decode the other buttons in column-major order
		  // This results in some gaps when using a shuttle or joystick model,
		  // but there really aren't any internally consistent numbering schemes.
		  for (int btn = 0; btn < 58; btn++) {
			  vrpn_uint8 *offset, mask;

			  offset = report + btn / 7 + 3;
			  mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			  buttons[btn + 1] = (*offset & mask) != 0;
		  }

                  // Report jog dial as analog and dial
                  // Report shuttle knob as analog

                  // Double cast on channel 0 ensures negative values stay negative
                  channel[0] = static_cast<float>(static_cast<signed char>(report[0])) / 7.0;
                  channel[1] = static_cast<float>(report[1]);

                  // Do the unsigned/signed conversion at the last minute so the
                  // signed values work properly.
                  dials[0] = static_cast<vrpn_int8>(report[1] - _lastDial) / 256.0;

                  // Store the current dial position for the next delta
                  _lastDial = report[1];

	  }
	} else {
		fprintf(stderr,"vrpn_Xkeys_Jog_And_Shuttle::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Jog_And_Shuttle12::vrpn_Xkeys_Jog_And_Shuttle12(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v2(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE12), name, c, XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE12)
	, vrpn_Analog(name, c)
	, vrpn_Button_Filter(name, c)
	, vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 13;  // Don't forget button 0

	// Initialize the state of all the analogs, buttons, and dials
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Xkeys_Jog_And_Shuttle12::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Jog_And_Shuttle12::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

void vrpn_Xkeys_Jog_And_Shuttle12::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void vrpn_Xkeys_Jog_And_Shuttle12::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
/*
	// Print the report so we can figure out what is going on.
	for (size_t i = 0; i < bytes; i++) {
	printf("%02x ", buffer[i]);
	}
	printf("\n");
	return;
*/

	// The full report is 32 bytes long.  Byte 0 is always 0, as are
	// bytes 12-31.  Bytes 8-11 seem to be some sort of time code.
	// Buttons: The reset button is the low-order bit in byte 1.
	//   The first column are the 3 low-order bits in byte 2
	//   The next three columns are the low-order bits in bytes 3-5.
	// Jog: Shows up as various high-order bits in the button bytes
	//      but also as a signed number in byte 7 that ranges from
	//      0 through 7 for positive rotation and down to F9 for
	//      negative.
	// Shuttle: Shows up as high-order bits in the button bytes,
	//      but also as a signed number in byte 6, which goes to
	//      1 briefly as you jog right and to FF briefly as you
	//      jog left; then back to 0.
	
	if (bytes % 32 == 0) {

		// Loop through the reports in case we got multiple.
		size_t i;
		for (i = 0; i < bytes / 32; i++) {
			vrpn_uint8 *report = buffer + (i * 32);

			// Check for zero in the bytes that should be.
			if (report[0] != 0) {
				// Garbled report; skip it
				fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
				continue;
			}
			for (size_t j = 12; j < 32; j++) {
				if (report[j] != 0) {
					// Garbled report; skip it
					fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
					continue;
				}
			}

			// Decode the "programming switch"
			buttons[0] = (report[1] & 0x01) != 0;

			// Decode the other buttons in column-major order
			for (int btn = 0; btn < 12; btn++) {
				vrpn_uint8 *offset, mask;

				offset = report + btn / 3 + 2;
				mask = static_cast<vrpn_uint8>(1 << (btn % 3));

				buttons[btn + 1] = (*offset & mask) != 0;
			}

			// Report jog dial as analog and dial
			// Report shuttle knob as analog

			// Do the unsigned/signed conversion at the last minute so the
			// signed values work properly.  The dial turns 1/10th of the
			// way around for each tick.
			dials[0] = static_cast<vrpn_int8>(report[6]) / 10.0;

			// Double cast on channel 0 ensures negative values stay negative.
			// Channel 1 sums the revolutions of the dial.
			channel[0] = static_cast<float>(static_cast<signed char>(report[7])) / 7.0;
			channel[1] += dials[0];

			// Store the current dial position for the next delta
			_lastDial = report[1];
		}
	}
	else {
		fprintf(stderr, "vrpn_Xkeys_Jog_And_Shuttle12::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Jog_And_Shuttle68::vrpn_Xkeys_Jog_And_Shuttle68(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v2(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE68), name, c, XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE68)
	, vrpn_Analog(name, c)
	, vrpn_Button_Filter(name, c)
	, vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 69;  // Don't forget button 0

	// Initialize the state of all the analogs, buttons, and dials
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Xkeys_Jog_And_Shuttle68::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Jog_And_Shuttle68::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

void vrpn_Xkeys_Jog_And_Shuttle68::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void vrpn_Xkeys_Jog_And_Shuttle68::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
/*
	// Print the report so we can figure out what is going on.
	for (size_t i = 0; i < bytes; i++) {
	printf("%02x ", buffer[i]);
	}
	printf("\n");
	return;
*/

	// The full report is 32 bytes long.  Byte 0 is always 0, as are
	// bytes 22-31.  Bytes 19-21 seem to be some sort of time code.
	// Buttons: The reset button is the low-order bit in byte 1.
	//   The first column of 8 buttons are the bits in byte 2.
	//   The next 9 columns are the bits in bytes 3-11.
        //   The last 3 bits are unused in the middle four columns,
        // where the shuttle-jog lives (columns 3-6).
	// Jog: Shows up as various high-order bits in the button bytes
	//      but also as a signed number in byte 17 that ranges from
	//      0 through 7 for positive rotation and down to F9 for
	//      negative.
	// Shuttle: Shows up as high-order bits in the button bytes,
	//      but also as a signed number in byte 16, which goes to
	//      1 briefly as you jog right and to FF briefly as you
	//      jog left; then back to 0.
	
	if (bytes % 32 == 0) {

		// Loop through the reports in case we got multiple.
		size_t i;
		for (i = 0; i < bytes / 32; i++) {
			vrpn_uint8 *report = buffer + (i * 32);

			// Check for zero in the bytes that should be.
			if (report[0] != 0) {
				// Garbled report; skip it
				fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
				continue;
			}
			for (size_t j = 22; j < 32; j++) {
				if (report[j] != 0) {
					// Garbled report; skip it
					fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
					continue;
				}
			}

			// Decode the "programming switch"
			buttons[0] = (report[1] & 0x01) != 0;

			// Decode the other buttons in column-major order
			for (int btn = 0; btn < 68; btn++) {

                                // Convert the button number into slot
                                // number, which means skipping the last
                                // 3 elements in the middle four columns.
                                int slot = btn;
                                if (btn > 28) { slot += 3; }
                                if (btn > 33) { slot += 3; }
                                if (btn > 38) { slot += 3; }
                                if (btn > 43) { slot += 3; }
				vrpn_uint8 *offset, mask;

				offset = report + slot / 8 + 2;
				mask = static_cast<vrpn_uint8>(1 << (slot % 8));
				buttons[btn + 1] = (*offset & mask) != 0;
			}

			// Report jog dial as analog and dial
			// Report shuttle knob as analog

			// Do the unsigned/signed conversion at the last minute so the
			// signed values work properly.  The dial turns 1/10th of the
			// way around for each tick.
			dials[0] = static_cast<vrpn_int8>(report[16]) / 10.0;

			// Double cast on channel 0 ensures negative values stay negative.
			// Channel 1 sums the revolutions of the dial.
			channel[0] = static_cast<float>(static_cast<signed char>(report[17])) / 7.0;
			channel[1] += dials[0];

			// Store the current dial position for the next delta
			_lastDial = report[1];
		}
	}
	else {
		fprintf(stderr, "vrpn_Xkeys_Jog_And_Shuttle68::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Joystick::vrpn_Xkeys_Joystick(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v1(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOYSTICK), name, c, XKEYS_VENDOR, XKEYS_JOYSTICK)
  , vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 59;  // Don't forget button 0

	// Initialize the state of all the analogs, buttons, and dials
	_gotDial = false;
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Xkeys_Joystick::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Joystick::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

void vrpn_Xkeys_Joystick::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void vrpn_Xkeys_Joystick::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Decode all full reports.
        // Full reports for all of the pro devices are 15 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver, leaving 14 bytes.
	// Also, it appears as though the
	// remaining 14-byte command is split into two, one 8-byte that is sent
	// first and then a 6-byte that is sent later.  So we need to check the
	// length of the packet to see which it is and then parse it appropriately.
	if (bytes == 8) {	// The first 8 bytes of a report

        // Report joystick axes as analogs				
        channel[0] = (static_cast<float>(buffer[0]) - 128) / 128.0;
        channel[1] = (static_cast<float>(buffer[1]) - 128) / 128.0;
        channel[2] = (static_cast<float>(buffer[2]) - 128) / 128.0;

		// Decode the other buttons in column-major order.  We skip the
		// first three bytes, which record the joystick value or the
		// shuttle/jog value (depending on device).
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The first 35 buttons are in this report, the remaining 23 in the next.
		for (int btn = 0; btn < 35; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 3;
			mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 6) {	// The last 6 bytes of a report

		if (!(buffer[5] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			return;
		}

		// Decode the "programming switch"
		buttons[0] = (buffer[5] & 0x10) != 0;

		// Decode the other buttons in column-major order.
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The last 23 buttons are in this report, the remaining 23 in the next.
		for (int btn = 35; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;
			int local_btn = btn - 35;

			offset = buffer + local_btn / 7;
			mask = static_cast<vrpn_uint8>(1 << (local_btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 14) {	// A full report in one swoop (happens on Windows)
          // Full reports for all of the pro devices are 15 bytes long.
          // Because there is only one type of report, the initial "0" report-type
          // byte is removed by the HIDAPI driver, leaving 14 bytes.
          size_t i;
	  for (i = 0; i < bytes / 14; i++) {
		  vrpn_uint8 *report = buffer + (i * 14);

		  if (!(report[13] & 0x08)) {
			  // Garbled report; skip it
			  fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			  continue;
		  }

		  // Decode the "programming switch"
		  buttons[0] = (report[13] & 0x10) != 0;

		  // Decode the other buttons in column-major order
		  // This results in some gaps when using a shuttle or joystick model,
		  // but there really aren't any internally consistent numbering schemes.
		  for (int btn = 0; btn < 58; btn++) {
			  vrpn_uint8 *offset, mask;

			  offset = report + btn / 7 + 3;
			  mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			  buttons[btn + 1] = (*offset & mask) != 0;
		  }

		  // Report joystick twist as analog and dial
		  if (!_gotDial) {
			  _gotDial = true;
		  }
		  else {
			  dials[0] = static_cast<vrpn_int8>(report[2] - _lastDial) / 256.0;
		  }
		  _lastDial = report[2];

		  // Double cast on channels 0 and 1 ensures negative values stay negative.
		  // Channel 2 sums the revolutions of the dial.
		  channel[0] = static_cast<float>(static_cast<signed char>(report[6])) / 127.0;
		  channel[1] = static_cast<float>(static_cast<signed char>(report[7])) / 127.0;
		  channel[2] += dials[0];

          // Report joystick axes as analogs				
          channel[0] = (static_cast<float>(report[0]) - 128) / 128.0;
          channel[1] = -(static_cast<float>(report[1]) - 128) / 128.0;
          channel[2] += dials[0];
	  }
	} else {
		fprintf(stderr,"vrpn_Xkeys_Joystick::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Joystick12::vrpn_Xkeys_Joystick12(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v2(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOYSTICK12), name, c, XKEYS_VENDOR, XKEYS_JOYSTICK12)
	, vrpn_Analog(name, c)
	, vrpn_Button_Filter(name, c)
	, vrpn_Dial(name, c)
{
	vrpn_Analog::num_channel = 3;
	vrpn_Dial::num_dials = 1;
	vrpn_Button::num_buttons = 13;  // Don't forget button 0

	// Initialize the state of all the analogs, buttons, and dials
	_gotDial = false;
	_lastDial = 0;
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
	memset(channel, 0, sizeof(channel));
	memset(last, 0, sizeof(last));
}

void vrpn_Xkeys_Joystick12::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Joystick12::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

void vrpn_Xkeys_Joystick12::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Dial::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void vrpn_Xkeys_Joystick12::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	/*
	// Print the report so we can figure out what is going on.
	for (size_t i = 0; i < bytes; i++) {
	printf("%02x ", buffer[i]);
	}
	printf("\n");
	return;
*/

	// The full report is 32 bytes long.  Byte 0 is always 0, as are
	// bytes 16-31.  Bytes 12-15 seem to be some sort of time code.
	// Buttons: The reset button is the low-order bit in byte 1.
	//   The first column are the 3 low-order bits in byte 2
	//   The next three columns are the low-order bits in bytes 3-5.
	// Joystick X axis: Byte 6 goes up to 7F to right, down to 81 to left.
	// Joystick Y axis: Byte 7 goes to 7F when down, 81 when up.
	// Joystick Z axis: Byte 8 goes up when spun right, down left, and
	//   wraps around from 255 to 0.

	if (bytes % 32 == 0) {

		// Loop through the reports in case we got multiple.
		size_t i;
		for (i = 0; i < bytes / 32; i++) {
			vrpn_uint8 *report = buffer + (i * 32);

			// Check for zero in the bytes that should be.
			if (report[0] != 0) {
				// Garbled report; skip it
				fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
				continue;
			}
			for (size_t j = 16; j < 32; j++) {
				if (report[j] != 0) {
					// Garbled report; skip it
					fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
					continue;
				}
			}

			// Decode the "programming switch"
			buttons[0] = (report[1] & 0x01) != 0;

			// Decode the other buttons in column-major order
			for (int btn = 0; btn < 12; btn++) {
				vrpn_uint8 *offset, mask;

				offset = report + btn / 3 + 2;
				mask = static_cast<vrpn_uint8>(1 << (btn % 3));

				buttons[btn + 1] = (*offset & mask) != 0;
			}

			// Report joystick twist as analog and dial
			if (!_gotDial) {
				_gotDial = true;
			} else {
				dials[0] = static_cast<vrpn_int8>(report[8] - _lastDial) / 256.0;
			}
			_lastDial = report[8];

			// Double cast on channels 0 and 1 ensures negative values stay negative.
			// Channel 2 sums the revolutions of the dial.
			channel[0] = static_cast<float>(static_cast<signed char>(report[6])) / 127.0;
			channel[1] = -static_cast<float>(static_cast<signed char>(report[7])) / 127.0;
			channel[2] += dials[0];
		}
	}
	else {
		fprintf(stderr, "vrpn_Xkeys_Joystick12::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Pro::vrpn_Xkeys_Pro(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_v1(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_PRO), name, c, XKEYS_VENDOR, XKEYS_PRO)
  , vrpn_Button_Filter(name, c)
{
	vrpn_Button::num_buttons = 59;  // Don't forget button 0

	// Initialize the state of all the buttons
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
}

void vrpn_Xkeys_Pro::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_Pro::report(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Pro::report_changes(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Pro::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
	// Decode all full reports.
        // Full reports for all of the pro devices are 15 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver, leaving 14 bytes.
	// Also, it appears as though the
	// remaining 14-byte command is split into two, one 8-byte that is sent
	// first and then a 6-byte that is sent later.  So we need to check the
	// length of the packet to see which it is and then parse it appropriately.
	if (bytes == 8) {	// The first 8 bytes of a report

		// Decode the other buttons in column-major order.  We skip the
		// first three bytes, which record the joystick value or the
		// shuttle/jog value (depending on device).
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The first 35 buttons are in this report, the remaining 23 in the next.
		for (int btn = 0; btn < 35; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 3;
			mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 6) {	// The last 6 bytes of a report

		if (!(buffer[5] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			return;
		}

		// Decode the "programming switch"
		buttons[0] = (buffer[5] & 0x10) != 0;

		// Decode the other buttons in column-major order.
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		// The last 23 buttons are in this report, the remaining 23 in the next.
		for (int btn = 35; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;
			int local_btn = btn - 35;

			offset = buffer + local_btn / 7;
			mask = static_cast<vrpn_uint8>(1 << (local_btn % 7));

			buttons[btn + 1] = (*offset & mask) != 0;
		}

	} else if (bytes == 14) {	// A full report in one swoop (happens on Windows)
          // Full reports for all of the pro devices are 15 bytes long.
          // Because there is only one type of report, the initial "0" report-type
          // byte is removed by the HIDAPI driver, leaving 14 bytes.
          size_t i;
	  // Decode all full reports.
          // Full reports for all of the pro devices are 15 bytes long.
	  for (i = 0; i < bytes / 14; i++) {
		  vrpn_uint8 *report = buffer + (i * 14);

		  if (!(report[13] & 0x08)) {
			  // Garbled report; skip it
			  fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			  continue;
		  }

		  // Decode the "programming switch"
		  buttons[0] = (report[13] & 0x10) != 0;

		  // Decode the other buttons in column-major order
		  // This results in some gaps when using a shuttle or joystick model,
		  // but there really aren't any internally consistent numbering schemes.
		  for (int btn = 0; btn < 58; btn++) {
			  vrpn_uint8 *offset, mask;

			  offset = buffer + btn / 7 + 3;
			  mask = static_cast<vrpn_uint8>(1 << (btn % 7));

			  buttons[btn + 1] = (*offset & mask) != 0;
		  }
	  }
	} else {
		fprintf(stderr,"vrpn_Xkeys_Pro::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_XK3::vrpn_Xkeys_XK3(const char *name, vrpn_Connection *c)
    : vrpn_Xkeys_noLEDs(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_XK3), name, c, XKEYS_VENDOR, XKEYS_XK3, false)
  , vrpn_Button_Filter(name, c)
{
  // 3 buttons
  vrpn_Button::num_buttons = 3;

  // Initialize the state of all the buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
}

void vrpn_Xkeys_XK3::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

        // Call the server_mainloop on our unique base class.
        server_mainloop();
}

void vrpn_Xkeys_XK3::report(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_XK3::report_changes(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_XK3::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
	// Decode all full reports, each of which is 32 bytes long.
        // Because there is only one type of report, the initial "0" report-type
        // byte is removed by the HIDAPI driver.
	for (size_t i = 0; i < bytes / 32; i++) {
		vrpn_uint8 *report = buffer + (i * 32);

		// The first two bytes of the report always seem to be
		// 0x00 0x01.
		if ((report[0] != 0x00) || (report[1] != 0x01) ) {
			// Apparently we got a corrupted report; skip this one.
			fprintf(stderr, "vrpn_Xkeys_XK3: Found a corrupted report; # total bytes = %u\n", static_cast<unsigned>(bytes));
			continue;
		}

		// The left button is bit 1 (value 0x02) in the third byte
		// The middle button is bit 2 (value 0x04) in the third byte
		// The right button is bit 3 (value 0x08) in the third byte
		buttons[0] = (report[2] & 0x02) != 0;
		buttons[1] = (report[2] & 0x04) != 0;
		buttons[2] = (report[2] & 0x08) != 0;
	}
}

// End of VRPN_USE_HID
#endif
