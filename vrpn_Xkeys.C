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

vrpn_Xkeys::vrpn_Xkeys(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c, bool toggle_light)
  : _filter(filter)
  , vrpn_HidInterface(filter)
  , vrpn_BaseClass(name, c)
  , _toggle_light(toggle_light)
{
	init_hid();
}

vrpn_Xkeys::~vrpn_Xkeys()
{
  if (_toggle_light) {
	// Indicate we're no longer waiting for a connection by
        // turning off both the red and green LEDs.
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 0;
	send_data(9, outputs);

  }
  delete _filter;
}

void vrpn_Xkeys::init_hid() {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);

	// Indicate we're waiting for a connection by turning on the red LED
  if (_toggle_light) {
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 128;
	send_data(9, outputs);
  }
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
		vrpn_uint8 outputs[9] = {0};
		outputs[8] = 128;
		me->send_data(9, outputs);
	}
	return 0;
}

int vrpn_Xkeys::on_connect(void *thisPtr, vrpn_HANDLERPARAM /*p*/)
{
	vrpn_Xkeys *me = static_cast<vrpn_Xkeys *>(thisPtr);

	if (me->_toggle_light) {
		// Set light to green to indicate we have an active connection
		vrpn_uint8 outputs[9] = {0};
		outputs[8] = 64;
		me->send_data(9, outputs);
	}
	return 0;
}

vrpn_Xkeys_Desktop::vrpn_Xkeys_Desktop(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_DESKTOP), name, c)
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

	vrpn_Button::server_mainloop();
}

void vrpn_Xkeys_Desktop::report(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Desktop::report_changes(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Desktop::decodePacket(size_t bytes, vrpn_uint8 *buffer) {
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
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE), name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Analog(name, c)
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

	vrpn_Analog::server_mainloop();
	vrpn_Button::server_mainloop();
	vrpn_Dial::server_mainloop();
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

vrpn_Xkeys_Joystick::vrpn_Xkeys_Joystick(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOYSTICK), name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Analog(name, c)
{
	vrpn_Analog::num_channel = 2;
	vrpn_Button::num_buttons = 59;  // Don't forget button 0

	// Initialize the state of all the analogs and buttons
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

	vrpn_Analog::server_mainloop();
	vrpn_Button::server_mainloop();
}

void vrpn_Xkeys_Joystick::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_Xkeys_Joystick::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
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

                  // Report joystick axes as analogs				
                  channel[0] = (static_cast<float>(report[0]) - 128) / 128.0;
                  channel[1] = (static_cast<float>(report[1]) - 128) / 128.0;
                  channel[2] = (static_cast<float>(report[2]) - 128) / 128.0;
	  }
	} else {
		fprintf(stderr,"vrpn_Xkeys_Joystick::decodePacket(): Unrecognized packet length (%u)\n", static_cast<unsigned>(bytes));
		return;
	}
}

vrpn_Xkeys_Pro::vrpn_Xkeys_Pro(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_PRO), name, c)
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

	vrpn_Button::server_mainloop();
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
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_XK3), name, c, false)
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

	vrpn_Button::server_mainloop();
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
