// vrpn_3DConnexion.C: VRPN driver for 3DConnexion
//  Space Navigator, Space Traveler, Space Explorer, Space Mouse, Spaceball 5000
#include <string.h>                     // for memset

#include "vrpn_3DConnexion.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_WARNING

// There is a non-HID Linux-based driver for this device that has a capability
// not implemented in the HID interface.  It uses the input.h interface.
#if defined(linux) && !defined(VRPN_USE_HID)
#define VRPN_USING_3DCONNEXION_EVENT_IFACE
#include <linux/input.h>
#include <stdlib.h> // for malloc, free, etc
#include <unistd.h> // for write, etc
#endif

typedef struct input_devinfo {
        vrpn_uint16 bustype;
        vrpn_uint16 vendor;
        vrpn_uint16 product;
        vrpn_uint16 version;
} XXX_should_have_been_in_system_includes;

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_3DCONNEXION_VENDOR = 0x046d; //1133;	// 3Dconnexion is made by Logitech
static const vrpn_uint16 vrpn_SPACEMOUSEWIRELESS_VENDOR = 9583; 	// Made by a different vendor...
static const vrpn_uint16 vrpn_3DCONNEXION_TRAVELER = 50723;
static const vrpn_uint16 vrpn_3DCONNEXION_NAVIGATOR = 50726;
static const vrpn_uint16 vrpn_3DCONNEXION_NAVIGATOR_FOR_NOTEBOOKS = 0xc628;	// 50728;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEEXPLORER = 0xc627;   // 50727
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSE = 50691;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSEPRO = 50731;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSECOMPACT = 50741;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSEWIRELESS = 50735;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSEPROWIRELESS = 0xC631;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEBALL5000 = 0xc621;   // 50721;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEPILOT =  0xc625;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEPILOTPRO = 0xc629;

vrpn_3DConnexion::vrpn_3DConnexion(vrpn_HidAcceptor *filter, unsigned num_buttons,
                                   const char *name, vrpn_Connection *c,
                                   vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_Button_Filter(name, c)
  , vrpn_Analog(name, c)
#if defined(VRPN_USE_HID)
  , vrpn_HidInterface(filter, vendor, product)
#endif
  , _filter(filter)
{
  vrpn_Analog::num_channel = 6;
  vrpn_Button::num_buttons = num_buttons;

  // Initialize the state of all the analogs and buttons
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));

  // Initialize the timestamp.
  vrpn_gettimeofday(&_timestamp, NULL);

// There is a non-HID Linux-based driver for this device that has a capability
// not implemented in the HID interface.  It is implemented using the Event
// interface.
#if defined(VRPN_USING_3DCONNEXION_EVENT_IFACE)
  // Use the Event interface to open devices looking for the one
  // we want.  Call the acceptor with all the devices we find
  // until we get one that we want.
    fd = -1;
    FILE *f;
    int i = 0;

    // try to autodetect the device
    char *fname = (char *)malloc(1000*sizeof(char));
    while(i < 256) {
        sprintf(fname, "/dev/input/event%d", i++);
        f = fopen(fname, "r+b");
        if(f) {
          // We got an active device.  Fill in its values and see if it
          // is acceptable to the filter.
          struct input_devinfo ID;
          ioctl(fileno(f), EVIOCGID, &ID);
          vrpn_HIDDEVINFO info;
          info.product = ID.product;
          info.vendor = ID.vendor;
          if (_filter->accept(info)) {
            fd = fileno(f);
            set_led(1);
            break;
          } else {
            fclose(f);
            f = NULL;
          }
        }
    }

    if(!f) {
        perror("Could not open the device");
        exit(1);
    }

    fclose(f);
    free(fname);

    // turn the LED on
    set_led(1);
#else
#ifndef VRPN_USE_HID
    fprintf(stderr,"vrpn_3DConnexion::vrpn_3DConnexion(): No implementation compiled in "
                   "to open this device.  Please recompile.\n");
#endif
#endif
}

vrpn_3DConnexion::~vrpn_3DConnexion()
{
#if defined(VRPN_USING_3DCONNEXION_EVENT_IFACE)
	set_led(0);
#endif
        try {
          delete _filter;
        } catch (...) {
          fprintf(stderr, "vrpn_3DConnexion::~vrpn_3DConnexion(): delete failed\n");
          return;
        }
}

#if defined(VRPN_USE_HID)
void vrpn_3DConnexion::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}
#endif

void vrpn_3DConnexion::mainloop()
{
#if defined(VRPN_USE_HID)
	// Full reports are 7 bytes long.
	// XXX If we get a 2-byte report mixed in, then something is going to get
	// truncated.
	update();
#elif defined(VRPN_USING_3DCONNEXION_EVENT_IFACE)
    struct timeval zerotime;
    fd_set fdset;
    struct input_event ev;
    int i;

    zerotime.tv_sec = 0;
    zerotime.tv_usec = 0;

    FD_ZERO(&fdset);                      /* clear fdset              */
    FD_SET(fd, &fdset);                   /* include fd in fdset      */
    int moreData = 0;
    do {
        vrpn_noint_select(fd + 1, &fdset, NULL, NULL, &zerotime);
        moreData = 0;
        if (FD_ISSET(fd, &fdset)) {
            moreData = 1;
            if (vrpn_noint_block_read(fd, reinterpret_cast<char*>(&ev), sizeof(struct input_event)) != sizeof(struct input_event)) {
                send_text_message("Error reading from vrpn_3DConnexion", vrpn_Analog::timestamp, vrpn_TEXT_ERROR);
                if (d_connection) { d_connection->send_pending_reports(); }
                return;
            }
            switch (ev.type) {
                case EV_KEY:    // button movement
                    vrpn_gettimeofday((timeval *)&this->vrpn_Button::timestamp, NULL);
                    buttons[ev.code & 0x0ff] = ev.value;
                    break;
 
                case EV_REL:    // axis movement
                case EV_ABS:    // new kernels send more logical _ABS instead of _REL
                    vrpn_gettimeofday((timeval *)&this->vrpn_Analog::timestamp, NULL);
                    // Convert from short to int to avoid a short/double conversion
                    // bug in GCC 3.2.
                    i = ev.value;
                    channel[ev.code] = static_cast<double>(i)/400.0;           
                    break;
 
                default:
                    break;
            }
        }
        report_changes();
    } while (moreData == 1);
#endif

    server_mainloop();
    vrpn_gettimeofday(&_timestamp, NULL);
}

void vrpn_3DConnexion::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_3DConnexion::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

#if defined(linux) && !defined(VRPN_USE_HID)
int vrpn_3DConnexion::set_led(int led_state)
{
  struct input_event event;
  int ret;

  event.type  = EV_LED;
  event.code  = LED_MISC;
  event.value = led_state;

  ret = write(fd, &event, sizeof(struct input_event));
  if (ret < 0) {
    perror ("setting led state failed");
  }
  return ret < static_cast<int>(sizeof(struct input_event));
}
#endif

#if defined(VRPN_USE_HID)
void vrpn_3DConnexion::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // Force 'small' buffers (ie button under linux - 3 bytes - and apple - 2 bytes - into 7 bytes
  // so we get through the report loop once.  XXX Problem: this is skipping 7 bytes per report
  // regardless of how many bytes were in the report.  This is going to get us into trouble for
  // multi-report packets.  Instead, we should go until we've parsed all characters and add the
  // number of characters parsed each time rather than a constant 7 reports.
  if(bytes<7) bytes=7;
  if (bytes > 7) {
	  fprintf(stderr, "vrpn_3DConnexion::decodePacket(): Long packet (%d bytes), may mis-parse\n",
		  static_cast<int>(bytes));
  }
  // Decode all full reports.
  // Full reports for all of the pro devices are 7 bytes long (the first
  // byte is the report type, because this device has multiple ones the
  // HIDAPI library leaves it in the report).
  for (size_t i = 0; i < bytes / 7; i++) {
    vrpn_uint8 *report = buffer + (i * 7);

    // There are three types of reports.  Parse whichever type
    // this is.
    char  report_type = report[0];
    vrpn_uint8 *bufptr = &report[1];
    const float scale = 1.0f/400.0f;
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
        channel[0] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[0] < -1.0) { channel[0] = -1.0; }
        if (channel[0] > 1.0) { channel[0] = 1.0; }
        channel[1] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[1] < -1.0) { channel[1] = -1.0; }
        if (channel[1] > 1.0) { channel[1] = 1.0; }
        channel[2] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[2] < -1.0) { channel[2] = -1.0; }
        if (channel[2] > 1.0) { channel[2] = 1.0; }
        break;

      case 2:
        channel[3] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[3] < -1.0) { channel[3] = -1.0; }
        if (channel[3] > 1.0) { channel[3] = 1.0; }
        channel[4] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[4] < -1.0) { channel[4] = -1.0; }
        if (channel[4] > 1.0) { channel[4] = 1.0; }
        channel[5] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;
        if (channel[5] < -1.0) { channel[5] = -1.0; }
        if (channel[5] > 1.0) { channel[5] = 1.0; }
        break;

      case 3: { // Button report
        int btn;

        // Button reports are encoded as bits in the first 2 bytes
        // after the type.  There can be more than one byte if there
        // are more than 8 buttons such as on SpaceExplorer or SpaceBall5000.
        // If 8 or less, we don't look at 2nd byte.
        // SpaceExplorer buttons are (for example):
        // Name           Number
        // 1              0
        // 2              1
        // T              2
        // L              3
        // R              4
        // F              5
        // ESC            6
        // ALT            7
        // SHIFT          8
        // CTRL           9
        // FIT            10
        // PANEL          11
        // +              12
        // -              13
        // 2D             14

        for (btn = 0; btn < vrpn_Button::num_buttons; btn++) {
            vrpn_uint8 *location, mask;
            location = report + 1 + (btn / 8);
            mask = 1 << (btn % 8);
            buttons[btn] = ( (*location) & mask) != 0;
        }
        break;
      }

      default:
        vrpn_gettimeofday(&_timestamp, NULL);
        send_text_message("Unknown report type", _timestamp, vrpn_TEXT_WARNING);
    }
    // Report this event before parsing the next.
    report_changes();
  }
}
#endif

vrpn_3DConnexion_Navigator::vrpn_3DConnexion_Navigator(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR), 2, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR)
{
}

vrpn_3DConnexion_Navigator_for_Notebooks::vrpn_3DConnexion_Navigator_for_Notebooks(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR_FOR_NOTEBOOKS), 2, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR_FOR_NOTEBOOKS)
{
}

vrpn_3DConnexion_Traveler::vrpn_3DConnexion_Traveler(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_TRAVELER), 8, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_TRAVELER)
{
}

vrpn_3DConnexion_SpaceMouse::vrpn_3DConnexion_SpaceMouse(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEMOUSE), 11, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEMOUSE)
{
}

vrpn_3DConnexion_SpaceMousePro::vrpn_3DConnexion_SpaceMousePro(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEPRO), 27, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEPRO)
{	// 15 physical buttons are numbered: 0-2, 4-5, 8, 12-15, 22-26
}

vrpn_3DConnexion_SpaceMouseCompact::vrpn_3DConnexion_SpaceMouseCompact(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSECOMPACT), 2, name, c, vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSECOMPACT)
{
}

vrpn_3DConnexion_SpaceMouseWireless::vrpn_3DConnexion_SpaceMouseWireless(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEWIRELESS), 2, name, c, vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEWIRELESS)
{
}

vrpn_3DConnexion_SpaceExplorer::vrpn_3DConnexion_SpaceExplorer(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEEXPLORER), 15, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEEXPLORER)
{
}

vrpn_3DConnexion_SpaceBall5000::vrpn_3DConnexion_SpaceBall5000(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEBALL5000), 12, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEBALL5000)
{
}

vrpn_3DConnexion_SpacePilot::vrpn_3DConnexion_SpacePilot(const char *name, vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEPILOT), 21, name, c, vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEPILOT)
{
}

vrpn_3DConnexion_SpacePilotPro::vrpn_3DConnexion_SpacePilotPro(const char *name,
                                                         vrpn_Connection *c)
    : vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR,
                                                   vrpn_3DCONNEXION_SPACEPILOTPRO),
                       31, name, c, vrpn_3DCONNEXION_VENDOR,
                       vrpn_3DCONNEXION_SPACEPILOTPRO)
{
}

vrpn_3DConnexion_SpaceMouseProWireless::vrpn_3DConnexion_SpaceMouseProWireless(const char *name, vrpn_Connection *c)
	: vrpn_3DConnexion(new vrpn_HidProductAcceptor(vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEPROWIRELESS), 32, name, c, vrpn_SPACEMOUSEWIRELESS_VENDOR, vrpn_3DCONNEXION_SPACEMOUSEPROWIRELESS)
{
}

void vrpn_3DConnexion_SpaceMouseProWireless::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // under windows anyway, reports are always 13 bytes long
	if ((bytes % 13) != 0) {
		return;
	}

	for (size_t i = 0; i < bytes / 13; i++) {

		vrpn_uint8 *report = buffer + (i * 13);
		char report_type = report[0];
		vrpn_uint8 *bufptr = &report[1];
		const float scale = 1.0f / 350.f;		// max value observed is 0x15e or 350 (signed)

		switch (report_type) {
    // Report type 1 includes both position and rotation on this device.
		case 0x1:
		{
			for (int c = 0; c < 6; c++) {

				channel[c] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * scale;

				//clamp to sane range
				if (channel[c] < -1.0) { channel[c] = -1.0; }
				if (channel[c] > 1.0) { channel[c] = 1.0; }
			}
			break;
		}

		case 0x3: // Button report - hw only seems to have 15 buttons, but they aren't tightly packed
		{
			for (int btn = 0; btn < vrpn_Button::num_buttons; btn++) {
				vrpn_uint8 *location = report + 1 + (btn / 8);
				vrpn_uint8 mask = 1 << (btn % 8);
				buttons[btn] = ((*location) & mask) != 0;
			}
			break;
		}

		case 0x17:	// don't know what this is, seems to get sent at the end of some data after maybe a timeout?
			break;

		default:
			vrpn_gettimeofday(&_timestamp, NULL);
			send_text_message("Unknown report type", _timestamp,
				vrpn_TEXT_WARNING);
		}
		// Report this event before parsing the next.
		report_changes();
	}
}

