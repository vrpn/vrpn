// vrpn_WiiMote.C:  Drive for the WiiMote, based on the WiiUse library.
//
// Russ Taylor, revised December 2008

#include "vrpn_WiiMote.h"

#if defined(VRPN_USE_WIIUSE)
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
	#include <unistd.h>
#endif
#include VRPN_WIIUSE_H

// Opaque class to hold WiiMote device information so we don't have
// to include wiimote.h in the vrpn_WiiMote.h file.
class vrpn_Wiimote_Device {
public:
  struct wiimote_t *device;
  unsigned  which;
  bool      found;
  bool      connected;
};

// Helper routines.
#ifndef	min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

void vrpn_WiiMote::handle_event()
{
  //-------------------------------------------------------------------------
  // Set the status of the buttons.  The first 16 buttons come from
  // the main contoller.  If there is an expansion controller plugged in,
  // then there is a second set of up to 16 buttons that can be read from it.
  unsigned i;
  for (i = 0; i < 16; i++) {
    buttons[i] = ( wiimote->device->btns & (1 << i) ) != 0;
  }

  if (wiimote->device->exp.type == EXP_NUNCHUK) {
    for (i = 0; i < 16; i++) {
      buttons[16+i] = ( wiimote->device->exp.nunchuk.btns & (1 << i) ) != 0;
    }
  }
  
  if (wiimote->device->exp.type == EXP_CLASSIC) {
    for (i = 0; i < 16; i++) {
      buttons[32+i] = ( wiimote->device->exp.classic.btns & (1 << i) ) != 0;
    }
  }

  if (wiimote->device->exp.type == EXP_GUITAR_HERO_3) {
    for (i = 0; i < 16; i++) {
      buttons[48+i] = ( wiimote->device->exp.gh3.btns & (1 << i) ) != 0;
    }
  }

  //-------------------------------------------------------------------------
  // Read the status of the analog values.  There are six of them for the
  // base unit and extras for expansion units.
  channel[0] = wiimote->device->battery_level;
  if (WIIUSE_USING_ACC(wiimote->device)) {
    channel[1] = wiimote->device->gforce.x;
    channel[2] = wiimote->device->gforce.y;
    channel[3] = wiimote->device->gforce.z;
  }
  if (WIIUSE_USING_IR(wiimote->device)) {
    unsigned dot;
    for (dot = 0; dot < 3; dot++) {
      if (wiimote->device->ir.dot[dot].visible) {
        channel[4 + 3*dot + 0] = wiimote->device->ir.dot[dot].rx;
        channel[4 + 3*dot + 1] = wiimote->device->ir.dot[dot].ry;
        channel[4 + 3*dot + 2] = wiimote->device->ir.dot[dot].size;
      } else {
        channel[4 + 3*dot + 0] = -1;
        channel[4 + 3*dot + 1] = -1;
        channel[4 + 3*dot + 2] = -1;
      }
    }
  }

  // See which secondary controller is installed and report
  if (wiimote->device->exp.type == EXP_NUNCHUK) {
    channel[16 + 0] = wiimote->device->exp.nunchuk.gforce.x;
    channel[16 + 1] = wiimote->device->exp.nunchuk.gforce.y;
    channel[16 + 2] = wiimote->device->exp.nunchuk.gforce.z;
    channel[16 + 3] = wiimote->device->exp.nunchuk.js.ang;
    channel[16 + 4] = wiimote->device->exp.nunchuk.js.mag;
  }

  if (wiimote->device->exp.type == EXP_CLASSIC) {
    channel[32 + 0] = wiimote->device->exp.classic.l_shoulder;
    channel[32 + 1] = wiimote->device->exp.classic.r_shoulder;
    channel[32 + 2] = wiimote->device->exp.classic.ljs.ang;
    channel[32 + 3] = wiimote->device->exp.classic.ljs.mag;
    channel[32 + 4] = wiimote->device->exp.classic.rjs.ang;
    channel[32 + 5] = wiimote->device->exp.classic.rjs.mag;
  }

  if (wiimote->device->exp.type == EXP_GUITAR_HERO_3) {
    channel[48 + 0] = wiimote->device->exp.gh3.whammy_bar;
    channel[48 + 1] = wiimote->device->exp.gh3.js.ang;
    channel[48 + 2] = wiimote->device->exp.gh3.js.mag;
  }

  //-------------------------------------------------------------------------
  // Read the state of the Infrared sensors.
}

void vrpn_WiiMote::initialize_wiimote_state(void)
{
  if ( !wiimote->device || !wiimote->found || !wiimote->connected) {
    return;
  }

  // Turn on a light so we know which device we are.
  switch (wiimote->which) {
    case 0:
      wiiuse_set_leds(wiimote->device, WIIMOTE_LED_1);
      break;
    case 1:
      wiiuse_set_leds(wiimote->device, WIIMOTE_LED_2);
      break;
    case 2:
      wiiuse_set_leds(wiimote->device, WIIMOTE_LED_3);
      break;
    case 3:
      wiiuse_set_leds(wiimote->device, WIIMOTE_LED_4);
      break;
    default:
      struct timeval now; 
      vrpn_gettimeofday(&now, NULL); 
      char msg[1024];
      sprintf(msg, "Too-large remote %d (0-3 available)", wiimote->which);
      send_text_message(msg, now, vrpn_TEXT_ERROR);
      break;
  }

  // Ask to look for motion sensing
  wiiuse_motion_sensing(wiimote->device, 1);

  // Turn off rumbling
  wiiuse_rumble(wiimote->device, 0);

  // Turn on IR sensing
  wiiuse_set_ir(wiimote->device, 1);
}

// Device constructor.
// Parameters:
// - name: VRPN name to assign to this server
// - c: VRPN connection this device should be attached to
vrpn_WiiMote::vrpn_WiiMote(const char *name, vrpn_Connection *c, unsigned which):
	vrpn_Analog(name, c),
	vrpn_Button(name, c),
        vrpn_Analog_Output(name, c),
        wiimote(new vrpn_Wiimote_Device)
{
        int i;
        char  msg[1024];

	vrpn_Analog::num_channel = min(64, vrpn_CHANNEL_MAX);
        for (i = 0; i < vrpn_Analog::num_channel; i++) {
          channel[i] = 0;
        }

        // There are bits for up to 16 buttons on the main remote, and for
        // up to 16 more on an expansion pack.
	vrpn_Button::num_buttons = min(64, vrpn_BUTTON_MAX_BUTTONS);
        for (i = 0; i < vrpn_Button::num_buttons; i++) {
          buttons[i] = 0;
        }

	vrpn_Analog_Output::o_num_channel = 1;

	// Register a handler for the request channel change message
	if (register_autodeleted_handler(request_m_id,
	  handle_request_message, this, d_sender_id)) {
		FAIL("vrpn_WiiMote: can't register change channel request handler");
		return;
	}

	// Register a handler for the request channels change message
	if (register_autodeleted_handler(request_channels_m_id,
	  handle_request_channels_message, this, d_sender_id)) {
		FAIL("vrpn_WiiMote: can't register change channels request handler");
		return;
	}

	// Register a handler for the no-one's-connected-now message
	if (register_autodeleted_handler(
	  d_connection->register_message_type(vrpn_dropped_last_connection), 
	  handle_last_connection_dropped, this)) {
		FAIL("Can't register self-destruct handler");
		return;
	}

        // Get a list of available devices and select the one we want.
        // Look for up to 4 motes.  Timeout in 5 seconds if one not found.
        wiimote->which = which;
        wiimote_t **available_wiimotes = wiiuse_init(4);
        unsigned num_available = wiiuse_find(available_wiimotes, 4, 5);
        if (num_available < (wiimote->which + 1)) {
          struct timeval now; 
          vrpn_gettimeofday(&now, NULL); 
          sprintf(msg, "Could not open remote %d (%d found)", wiimote->which, num_available);
          send_text_message(msg, now, vrpn_TEXT_ERROR);
          wiimote->found = false;
        } else {
          wiimote->found = true;

          // Make a list containing just the one we want, and then connect to it.
          wiimote_t *selected_one[1];
          wiimote->device = selected_one[0] = available_wiimotes[wiimote->which];
          wiimote->connected = (wiiuse_connect(selected_one, 1) == 1);
          if (!wiimote->connected) {
            struct timeval now; 
            vrpn_gettimeofday(&now, NULL); 
            sprintf(msg, "Could not connect to remote %d", wiimote->which);
            send_text_message(msg, now, vrpn_TEXT_ERROR);
          }
        }

        if (wiimote->connected) {
          initialize_wiimote_state();
        }
}

// Device destructor
vrpn_WiiMote::~vrpn_WiiMote() {
  // Close the device and 

  if (wiimote->connected) {
    wiiuse_disconnect(wiimote->device);
  }
  delete wiimote;
}

// VRPN main loop
// Poll the device and let the VRPN change notifications fire
void vrpn_WiiMote::mainloop() {
	static time_t last_error = time(NULL);

        vrpn_gettimeofday(&_timestamp, NULL);

        //XXX Try to connect once a second if we have found but not connected device.

	server_mainloop();

        // Poll to get the status of the device.  When an event happens, call
        // the appropriate handler to fill in our data structures.  To do this,
        // we need a list of pointers to WiiMotes, so we create one with a single
        // entry.
        wiimote_t *selected_one[1];
        selected_one[0] = wiimote->device;
        if (wiimote->connected && wiiuse_poll(selected_one, 1) ) {
          switch (wiimote->device->event) {
            case WIIUSE_EVENT:  // A generic event
              handle_event();
              break;

            case WIIUSE_STATUS: // A status event
              // Nothing to do here, we're polling what we need to know in mainloop.
              break;

            case WIIUSE_DISCONNECT:
            case WIIUSE_UNEXPECTED_DISCONNECT:
              wiimote->connected = false;
              send_text_message("Disconnected", _timestamp, vrpn_TEXT_ERROR);
              break;

            case WIIUSE_READ_DATA:
              // Data we requested was returned.  Take a look at wiimote->device->read_req
              // for the info.
              break;

            case WIIUSE_NUNCHUK_INSERTED:
              send_text_message("Nunchuck inserted", _timestamp);
              break;

            case WIIUSE_CLASSIC_CTRL_INSERTED:
              send_text_message("Classic controller inserted", _timestamp);
              break;

            case WIIUSE_GUITAR_HERO_3_CTRL_INSERTED:
              send_text_message("Guitar Hero 3 controller inserted", _timestamp);
              break;

            case WIIUSE_NUNCHUK_REMOVED:
            case WIIUSE_CLASSIC_CTRL_REMOVED:
            case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
              send_text_message("An expansion controller was removed", _timestamp,
                vrpn_TEXT_WARNING);
              break;

            default:
              send_text_message("unknown event", _timestamp);
              break;
          }
        }

	// Send any changes out over the connection.
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

void vrpn_WiiMote::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_WiiMote::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

/* static */
int vrpn_WiiMote::handle_request_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_WiiMote* me = (vrpn_WiiMote*)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      fprintf(stderr,"vrpn_WiiMote::handle_request_message(): Index out of bounds\n");
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message):  channel %d is not active.  Squelching.", chan_num );
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
      return 0;
    }
    me->o_channel[chan_num] = value;
    if (value >= 0.5) {
      wiiuse_rumble(me->wiimote->device, 1);
    } else {
      wiiuse_rumble(me->wiimote->device, 0);
    }

    return 0;
}

/* static */
int vrpn_WiiMote::handle_request_channels_message(void* userdata,
    vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_WiiMote* me = (vrpn_WiiMote*)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  channels above %d not active; "
              "bad request up to channel %d.  Squelching.", me->o_num_channel, num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         num = me->o_num_channel;
    }
    if (num < 0) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  invalid channel %d.  Squelching.", num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         return 0;
    }

    // Pull only channel 0 from the buffer, no matter how many values we received.
    vrpn_float64 value;
    vrpn_unbuffer(&bufptr, &value);
    if (value >= 0.5) {
      wiiuse_rumble(me->wiimote->device, 1);
    } else {
      wiiuse_rumble(me->wiimote->device, 0);
    }
    
    return 0;
}

int VRPN_CALLBACK vrpn_WiiMote::handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM data) {
  //XXX Turn off any rumble.
  return 0;
}

#endif  // VRPN_USE_WIIUSE
