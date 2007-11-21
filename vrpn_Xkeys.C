// vrpn_Xkeys.C: VRPN driver for P.I. Engineering's X-Keys devices

#include "vrpn_Xkeys.h"

#ifdef _WIN32

// USB vendor and product IDs for the models we support
static const vrpn_uint16 XKEYS_VENDOR = 0x05F3;
static const vrpn_uint16 XKEYS_DESKTOP = 0x0281;
static const vrpn_uint16 XKEYS_JOG_AND_SHUTTLE = 0x0241;
static const vrpn_uint16 XKEYS_PRO = 0x0291;
static const vrpn_uint16 XKEYS_JOYSTICK = 0x251;

vrpn_Xkeys::vrpn_Xkeys(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c)
  : _filter(filter)
  , vrpn_HidInterface(_filter)
  , vrpn_BaseClass(name, c)
{
	init_hid();
}

vrpn_Xkeys::~vrpn_Xkeys()
{
	// Indicate we're no longer waiting for a connection by
        // turning off both the red and green LEDs.
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 0;
	send_data(9, outputs);

	delete _filter;
}

void vrpn_Xkeys::reconnect()
{
	vrpn_HidInterface::reconnect();
}

void vrpn_Xkeys::init_hid() {
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);

	// Indicate we're waiting for a connection by turning on the red LED
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 128;
	send_data(9, outputs);
}

void vrpn_Xkeys::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Xkeys::on_last_disconnect(void *thisPtr, vrpn_HANDLERPARAM p) {
	// Set light to red to indicate we have no active connections
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 128;

	static_cast<vrpn_Xkeys *>(thisPtr)->send_data(9, outputs);

	return 0;
}

int vrpn_Xkeys::on_connect(void *thisPtr, vrpn_HANDLERPARAM p) {
	// Set light to green to indicate we have an active connection
	vrpn_uint8 outputs[9] = {0};
	outputs[8] = 64;

	static_cast<vrpn_Xkeys *>(thisPtr)->send_data(9, outputs);

	return 0;
}

vrpn_Xkeys_Desktop::vrpn_Xkeys_Desktop(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_DESKTOP), name, c)
  , vrpn_Button(name, c)
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
	// Decode all full reports, each of which is 12 bytes long
	for (size_t i = 0; i < bytes / 12; i++) {
		vrpn_uint8 *report = buffer + (i * 12);

		if ((report[0] != 0) || !(report[11] & 0x08)) {
			// Apparently we got a corrupted report; skip this one.
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", bytes);
			continue;
		}

		// Decode the "programming switch"
		buttons[0] = (report[11] & 0x10) != 0;

		// Decode the other buttons into column-major order
		for (int btn = 0; btn < 20; btn++) {
			vrpn_uint8 *offset, mask;
			
			offset = buffer + btn / 5 + 1;
			mask = 1 << (btn % 5);

			buttons[btn + 1] = (*offset & mask) != 0;
		}
	}
}

vrpn_Xkeys_Jog_And_Shuttle::vrpn_Xkeys_Jog_And_Shuttle(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOG_AND_SHUTTLE), name, c)
  , vrpn_Button(name, c)
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
	for (size_t i = 0; i < bytes / 15; i++) {
		vrpn_uint8 *report = buffer + (i * 15);

		if ((report[0] != 0) || !(report[14] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", bytes);
			continue;
		}

		// Decode the "programming switch"
		buttons[0] = (report[14] & 0x10) != 0;

		// Decode the other buttons in column-major order
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		for (int btn = 0; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 4;
			mask = 1 << (btn % 7);

			buttons[btn + 1] = (*offset & mask) != 0;
		}

                // Report jog dial as analog and dial
                // Report shuttle knob as analog

                // Double cast on channel 0 ensures negative values stay negative
                channel[0] = static_cast<float>(static_cast<signed char>(buffer[1])) / 7.0;
                channel[1] = static_cast<float>(buffer[2]);

                // Do the unsigned/signed conversion at the last minute so the
                // signed values work properly.
                dials[0] = static_cast<vrpn_int8>(buffer[2] - _lastDial) / 256.0;

                // Store the current dial position for the next delta
                _lastDial = buffer[2];
	}
}

vrpn_Xkeys_Joystick::vrpn_Xkeys_Joystick(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_JOYSTICK), name, c)
  , vrpn_Button(name, c)
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
	for (size_t i = 0; i < bytes / 15; i++) {
		vrpn_uint8 *report = buffer + (i * 15);

		if ((report[0] != 0) || !(report[14] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", bytes);
			continue;
		}

		// Decode the "programming switch"
		buttons[0] = (report[14] & 0x10) != 0;

		// Decode the other buttons in column-major order
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		for (int btn = 0; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 4;
			mask = 1 << (btn % 7);

			buttons[btn + 1] = (*offset & mask) != 0;
		}

                // Report joystick axes as analogs				
                channel[0] = (static_cast<float>(buffer[1]) - 128) / 128.0;
                channel[1] = (static_cast<float>(buffer[2]) - 128) / 128.0;
                channel[2] = (static_cast<float>(buffer[3]) - 128) / 128.0;
	}
}

vrpn_Xkeys_Pro::vrpn_Xkeys_Pro(const char *name, vrpn_Connection *c)
  : vrpn_Xkeys(_filter = new vrpn_HidProductAcceptor(XKEYS_VENDOR, XKEYS_PRO), name, c)
  , vrpn_Button(name, c)
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
	for (size_t i = 0; i < bytes / 15; i++) {
		vrpn_uint8 *report = buffer + (i * 15);

		if ((report[0] != 0) || !(report[14] & 0x08)) {
			// Garbled report; skip it
			fprintf(stderr, "vrpn_Xkeys: Found a corrupted report; # total bytes = %u\n", bytes);
			continue;
		}

		// Decode the "programming switch"
		buttons[0] = (report[14] & 0x10) != 0;

		// Decode the other buttons in column-major order
		// This results in some gaps when using a shuttle or joystick model,
		// but there really aren't any internally consistent numbering schemes.
		for (int btn = 0; btn < 58; btn++) {
			vrpn_uint8 *offset, mask;

			offset = buffer + btn / 7 + 4;
			mask = 1 << (btn % 7);

			buttons[btn + 1] = (*offset & mask) != 0;
		}
	}
}

// End of VPRN_USE_HID
#endif
