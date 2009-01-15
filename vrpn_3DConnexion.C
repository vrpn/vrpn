// vrpn_3DConnexion.C: VRPN driver for 3DConnexion
//  Space Navigator, Space Traveler, Space Explorer, Space Mouse, Spaceball 5000

#ifdef linux
#include <linux/input.h>
#endif

#include "vrpn_3DConnexion.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <string.h>

typedef struct input_devinfo {
        vrpn_uint16 bustype;
        vrpn_uint16 vendor;
        vrpn_uint16 product;
        vrpn_uint16 version;
} XXX_should_have_been_in_system_includes;

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_3DCONNEXION_VENDOR = 1133;
static const vrpn_uint16 vrpn_3DCONNEXION_TRAVELER = 50723;
static const vrpn_uint16 vrpn_3DCONNEXION_NAVIGATOR = 50726;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEEXPLORER = 0xc627;   // 50727
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEMOUSE = 50691;
static const vrpn_uint16 vrpn_3DCONNEXION_SPACEBALL5000 = 0xc621;   // 50721;

vrpn_3DConnexion::vrpn_3DConnexion(vrpn_HidAcceptor *filter, unsigned num_buttons,
                                   const char *name, vrpn_Connection *c)
  : _filter(filter)
#if defined(_WIN32) || defined(__CYGWIN__)
  , vrpn_HidInterface(_filter)
#endif
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

  // Use the Event interface to open devices looking for the one
  // we want.  Call the acceptor with all the devices we find
  // until we get one that we want.
#ifdef LINUX
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
          // is acceptible to the filter.
          struct input_devinfo ID;
          ioctl(fileno(f), EVIOCGID, &ID);
          vrpn_HIDDEVINFO info;
          info.devicePath = fname;
          info.product = ID.product;
          info.vendor = ID.vendor;
          info.version = ID.version;
          info.usagePage = 0;   // Unknown
          info.usage = 0;       // Unknown
          if (_filter->accept(info)) {
            fd = fileno(f);
            set_led(1);
            break;
          } else {
            fclose(f);
          }
        }
    }

    if(!f) {
        perror("Could not open the device");
        exit(1);
    }

    free(fname);
#endif
}

vrpn_3DConnexion::~vrpn_3DConnexion()
{
#if !(defined(_WIN32) || defined(__CYGWIN__))
	set_led(0);
#endif
        delete _filter;
}

#if defined(_WIN32) || defined(__CYGWIN__)
void vrpn_3DConnexion::reconnect()
{
	vrpn_HidInterface::reconnect();
}

void vrpn_3DConnexion::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}
#endif

void vrpn_3DConnexion::mainloop()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	update();
#elif defined(LINUX)
    struct timeval zerotime;
    fd_set fdset;
    struct input_event ev;
    int i;

    zerotime.tv_sec = 0;
    zerotime.tv_usec = 0;

    FD_ZERO(&fdset);                      /* clear fdset              */
    FD_SET(fd, &fdset);                   /* include fd in fdset      */
    vrpn_noint_select(fd + 1, &fdset, NULL, NULL, &zerotime);
    if (FD_ISSET(fd, &fdset)) {
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
#endif

    server_mainloop();
    vrpn_gettimeofday(&_timestamp, NULL);
    report_changes();
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

#if !(defined(_WIN32) || defined(__CYGWIN__))
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
  return ret < sizeof(struct input_event);
}
#endif

// Swap the endian-ness of the 2-byte entry in the buffer.
// This is used to make the little-endian int 16 values
// returned by the device into the big-endian format that is
// expected by the VRPN unbuffer routines.

static void swap_endian2(char *buffer)
{
	char c;
	c = buffer[0]; buffer[0] = buffer[1]; buffer[1] = c;
}

#if defined(_WIN32) || defined(__CYGWIN__)
void vrpn_3DConnexion::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
  // Decode all full reports.
  // Full reports for all of the pro devices are 7 bytes long.
  for (size_t i = 0; i < bytes / 7; i++) {
    vrpn_uint8 *report = buffer + (i * 7);

    // There are three types of reports.  Parse whichever type
    // this is.
    char  report_type = report[0];
    char *bufptr = static_cast<char *>(static_cast<void*>(&report[1]));
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
#endif

vrpn_3DConnexion_Navigator::vrpn_3DConnexion_Navigator(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_NAVIGATOR), 2, name, c)
{
}

vrpn_3DConnexion_Traveler::vrpn_3DConnexion_Traveler(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_TRAVELER), 8, name, c)
{
}

vrpn_3DConnexion_SpaceMouse::vrpn_3DConnexion_SpaceMouse(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEMOUSE), 11, name, c)
{
}

vrpn_3DConnexion_SpaceExplorer::vrpn_3DConnexion_SpaceExplorer(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEEXPLORER), 15, name, c)
{
}

vrpn_3DConnexion_SpaceBall5000::vrpn_3DConnexion_SpaceBall5000(const char *name, vrpn_Connection *c)
  : vrpn_3DConnexion(_filter = new vrpn_HidProductAcceptor(vrpn_3DCONNEXION_VENDOR, vrpn_3DCONNEXION_SPACEBALL5000), 12, name, c)
{
}

