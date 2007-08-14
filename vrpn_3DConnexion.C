// vrpn_3DConnexion.C: VRPN driver for 3DConnexion Space Navigator and Space Traveler

#include "vrpn_3DConnexion.h"

#ifdef VRPN_USE_HID

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_3DCONNEXION_VENDOR = 1133;
static const vrpn_uint16 vrpn_3DCONNEXION_TRAVELER = 50723;
static const vrpn_uint16 vrpn_3DCONNEXION_NAVIGATOR = 50726;

vrpn_3DConnexion::vrpn_3DConnexion(vrpn_HidAcceptor *filter, unsigned num_buttons,
                                   const char *name, vrpn_Connection *c)
  : _filter(filter)
  , vrpn_HidInterface(_filter)
  , vrpn_Analog(name, c)
  , vrpn_Button(name, c)
{
  vrpn_Analog::num_channel = 6;
  vrpn_Button::num_buttons = num_buttons;

  // Initialize the state of all the analogs and buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

vrpn_3DConnexion::~vrpn_3DConnexion()
{
	delete _filter;
}

void vrpn_3DConnexion::reconnect()
{
	vrpn_HidInterface::reconnect();
}

void vrpn_3DConnexion::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

void vrpn_3DConnexion::mainloop()
{
	update();
	server_mainloop();
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();

	vrpn_Analog::server_mainloop();
	vrpn_Button::server_mainloop();
}

void vrpn_3DConnexion::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_3DConnexion::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

// Swap the endian-ness of the 2-byte entry in the buffer.
// This is used to make the little-endian int 16 values
// returned by the device into the big-endian format that is
// expected by the VRPN unbuffer routines.

static void swap_endian2(char *buffer)
{
	char c;
	c = buffer[0]; buffer[0] = buffer[1]; buffer[1] = c;
}

void vrpn_3DConnexion::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // Decode all full reports.
  // Full reports for all of the pro devices are 7 bytes long.
  for (size_t i = 0; i < bytes / 7; i++) {
    vrpn_uint8 *report = buffer + (i * 7);

    // There are three types of reports.  Parse whichever type
    // this is.
    char  report_type = buffer[0];
    char *bufptr = static_cast<char *>(static_cast<void*>(&buffer[1]));
    vrpn_int16 temp;
    const float scale = static_cast<float>(1.0/400.0);
    switch (report_type)  {
      // Report types 1 and 2 come one after the other.  Each seems
      // to change when the puck is moved.  It looks like each pair
      // of values records a signed value for one channel; report
      // type 1 is translation and report type 2 is rotation.
      // The minimum and maximum values seem to vary somewhat.
      // They all seem to be able to get over 400, so we scale
      // by 400 and then clamp to (-1..1).
      // The first byte is the low-order byte and the second is the
      // high-order byte.
      case 1:
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[0] = temp * scale;
        if (channel[0] < -1.0) { channel[0] = -1.0; }
        if (channel[0] > 1.0) { channel[0] = 1.0; }
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[1] = temp * scale;
        if (channel[1] < -1.0) { channel[1] = -1.0; }
        if (channel[1] > 1.0) { channel[1] = 1.0; }
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[2] = temp * scale;
        if (channel[2] < -1.0) { channel[2] = -1.0; }
        if (channel[2] > 1.0) { channel[2] = 1.0; }
        break;

      case 2:
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[3] = temp * scale;
        if (channel[3] < -1.0) { channel[3] = -1.0; }
        if (channel[3] > 1.0) { channel[3] = 1.0; }
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[4] = temp * scale;
        if (channel[4] < -1.0) { channel[4] = -1.0; }
        if (channel[4] > 1.0) { channel[4] = 1.0; }
        swap_endian2(bufptr); vrpn_unbuffer(const_cast<const char **>(&bufptr), &temp);
        channel[5] = temp * scale;
        if (channel[5] < -1.0) { channel[5] = -1.0; }
        if (channel[5] > 1.0) { channel[5] = 1.0; }
        break;

      case 3: { // Button report
        int btn;

        // Button reports are encoded as bits in the first byte
        // after the type.  Presumably, there can be more if there
        // are more then 8 buttons on a future device but for now
        // we just unpack this byte into however many buttons we
        // have.
        for (btn = 0; btn < vrpn_Button::num_buttons; btn++) {
	        vrpn_uint8 *location, mask;

	        location = report + 1;
	        mask = 1 << (btn % 8);

	        buttons[btn] = (*location & mask) != 0;
        }
        break;
      }

      default:
        vrpn_gettimeofday(&_timestamp, NULL);
        send_text_message("Unknown report type", _timestamp, vrpn_TEXT_WARNING);
    }
  }
}

vrpn_3DConnexion_Navigator::vrpn_3DConnexion_Navigator(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR), 2, name, c)
{
}

vrpn_3DConnexion_Traveler::vrpn_3DConnexion_Traveler(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_TRAVELER), 8, name, c)
{
}

// End of VPRN_USE_HID
#endif
