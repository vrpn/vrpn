// vrpn_dreamcheeky.C: VRPN driver for Dream Cheeky USB roll-up drum kit

#include "vrpn_DreamCheeky.h"

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__APPLE__)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 DREAMCHEEKY_VENDOR = 6465;
static const vrpn_uint16 USB_ROLL_UP_DRUM_KIT = 32801;

vrpn_DreamCheeky::vrpn_DreamCheeky(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c)
  : _filter(filter)
  , vrpn_HidInterface(_filter)
  , vrpn_BaseClass(name, c)
{
}

vrpn_DreamCheeky::~vrpn_DreamCheeky()
{
	delete _filter;
}

void vrpn_DreamCheeky::reconnect()
{
	vrpn_HidInterface::reconnect();
}

void vrpn_DreamCheeky::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

vrpn_DreamCheeky_Drum_Kit::vrpn_DreamCheeky_Drum_Kit(const char *name, vrpn_Connection *c,
                                                     bool debounce)
  : vrpn_DreamCheeky(_filter = new vrpn_HidProductAcceptor(DREAMCHEEKY_VENDOR, USB_ROLL_UP_DRUM_KIT), name, c)
  , vrpn_Button(name, c)
  , d_debounce(debounce)
{
	vrpn_Button::num_buttons = 6;

	// Initialize the state of all the buttons
	memset(buttons, 0, sizeof(buttons));
	memset(lastbuttons, 0, sizeof(lastbuttons));
}

void vrpn_DreamCheeky_Drum_Kit::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

	vrpn_Button::server_mainloop();
}

void vrpn_DreamCheeky_Drum_Kit::report(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_DreamCheeky_Drum_Kit::report_changes(void) {
	vrpn_Button::timestamp = _timestamp;
	vrpn_Button::report_changes();
}

void vrpn_DreamCheeky_Drum_Kit::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // The reports are each 9 bytes long.  The first byte of each report
  // is zero.  The others are 8 identical reports.
  // There is one byte per report, and it holds a binary encoding of
  // the buttons.  Button 0 is in the LSB, and the others proceed up
  // the bit chain.  Parse each report and then send any changes on.
  // need to send between each report so we don't miss a button press/release
  // all in one packet.

  size_t i, r;
  // Decode all full reports, each of which is 9 bytes long
  for (i = 0; i < bytes / 9; i++) {

    // If we're debouncing the buttons, then we set the button
    // to "pressed" if it has 4 or more pressed events in the
    // set of 8 and to "released" if it has less than 4.
    if (d_debounce) {
      int btn;
      for (btn = 0; btn < vrpn_Button::num_buttons; btn++) {
        unsigned count = 0;
        vrpn_uint8 mask = 1 << btn;
        for (r = 1; r < bytes; r++) { // Skip the all-zeroes byte
          vrpn_uint8 *report = buffer + 9*i + r;
          count += ((*report & mask) != 0);
        }
        buttons[btn] = (count >= 4);
        vrpn_gettimeofday(&_timestamp, NULL);
        report_changes();
      }

    // If we're not debouncing, then we report each button event
    // independently.
    }else {
      for (r = 1; r < bytes; r++) { // Skip the all-zeroes byte
        vrpn_uint8 *report = buffer + 9*i + r;
        int btn;
        for (btn = 0; btn < vrpn_Button::num_buttons; btn++) {
          vrpn_uint8 mask = 1 << btn;
          buttons[btn] = ((*report & mask) != 0);
        }
        vrpn_gettimeofday(&_timestamp, NULL);
        report_changes();
      }
    }
  }
}

#endif

