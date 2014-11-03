// vrpn_GlobalHapticsOrb.C
//	This is a driver for the Global Haptics "Orb" device.
// This box is a serial-line device that provides a sphere
// with many buttons, a trackball, and a spinning valuator that
// is treated here as a dial.
//	This code is based on their driver code, which they
// sent to Russ Taylor to help get a public-domain driver
// written for the device.

#include <stdio.h>                      // for perror, sprintf

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_GlobalHapticsOrb.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday

#undef VERBOSE

static const double	POLL_INTERVAL = 5e+6;		// If we haven't heard, ask.
static const double	TIMEOUT_INTERVAL = 10e+6;	// If we haven't heard, complain.

static const double	REV_PER_TICK_WHEEL = 1.0/15;	// How many revolutions per encoder tick (checked)
static const double	REV_PER_TICK_BALL = 1.0/164;	// How many revolutions per encoder tick (guess)

static const unsigned char	reset_char = 0x81;    // Reset string sent to Orb

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the box
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

// static
int vrpn_GlobalHapticsOrb::handle_firstConnection(void * userdata,
                                                 vrpn_HANDLERPARAM)
{
  ((vrpn_GlobalHapticsOrb *) userdata)->clear_values();

  // Always return 0 here, because nonzero return means that the input data
  // was garbage, not that there was an error. If we return nonzero from a
  // vrpn_Connection handler, it shuts down the connection.
  return 0;
}


// This creates a vrpn_GlobalHapticsOrb and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
vrpn_GlobalHapticsOrb::vrpn_GlobalHapticsOrb(const char * name, vrpn_Connection * c,
					     const char * port, int baud) :
		vrpn_Serial_Analog(name, c, port, baud),
		vrpn_Button_Filter(name, c),
		vrpn_Dial(name, c)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = 30;
	vrpn_Analog::num_channel = 3;
	vrpn_Dial::num_dials = 3;

	// Set the status of the buttons, analogs and encoders to 0 to start
	clear_values();

	// Set a callback handler for when the first connection is made so
	// that it can reset the analogs and buttons whenever this happens.
	register_autodeleted_handler(
		      d_connection->register_message_type(vrpn_got_first_connection),
		      handle_firstConnection, this);

	// Set the mode to reset
	d_status = STATUS_RESETTING;
}

void	vrpn_GlobalHapticsOrb::clear_values(void)
{
	int	i;

	for (i = 0; i < num_buttons; i++) {
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
	}
	for (i = 0; i < num_channel; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
	for (i = 0; i < num_dials; i++) {
		vrpn_Dial::dials[i] = 0.0;
	}
}

// This routine will reset the GlobalHapticsOrb. It verifies that it can
// communicate with the device by sending it the "enter direct mode"
// command (0x81) and waiting for it to respond with the single-byte
// message 0xFC.

int	vrpn_GlobalHapticsOrb::reset(void)
{
	struct	timeval	timeout;
	unsigned char	inbuf[1];	      // Response from the Orb
	int	ret;

	//-----------------------------------------------------------------------
	// Set the values back to zero for all buttons, analogs and encoders
	clear_values();

	//-----------------------------------------------------------------------
	// Clear the input buffer to make sure we're starting with a clean slate.
	// Send the "reset" command to the box, then wait for a response and
	// make sure it matches what we expect.
	vrpn_flush_input_buffer(serial_fd);
	vrpn_write_characters(serial_fd, &reset_char, 1);
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, 1, &timeout);
	if (ret < 0) {
	  perror("vrpn_GlobalHapticsOrb::reset(): Error reading from Orb\n");
	  return -1;
	}
	if (ret == 0) {
	  send_text_message("vrpn_GlobalHapticsOrb::reset(): No response from Orb", d_timestamp, vrpn_TEXT_ERROR);
	  return -1;
	}
	if (inbuf[0] != 0xfc) {
	  char	message[1024];
	  sprintf(message, "vrpn_GlobalHapticsOrb::reset(): Bad response from Orb (%02X)", inbuf[0]);
	  send_text_message(message, d_timestamp, vrpn_TEXT_ERROR);
	  return -1;
	}

	//-----------------------------------------------------------------------
	// Figure out how many characters to expect in each report from the device,
	// which is just 1 for the Orb.
	d_expected_chars = 1;

	vrpn_gettimeofday(&d_timestamp, NULL);	// Set watchdog now

	send_text_message("vrpn_GlobalHapticsOrb::reset(): Reset complete (this is good)", d_timestamp, vrpn_TEXT_ERROR);

	// Set the mode to synchronizing
	d_status = STATUS_SYNCING;
	return 0;
}

// See if we have a report from the Orb.  Each report is one character.  There
// are separate characters for button up and button down for each button, and
// for the rocker switches.  There are separate characters for left and right
// for the thumbwheel going left one tick and right one tick.  There are
// separate characters for north, south, east, and west for the trackball.
// There are also a bunch of characters that are supposed to be ignored when
// they come.  Previous versions of the Orb didn't send the release messages.
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the box hasn't simply
// stopped sending characters).
// Returns 1 if got a complete report, 0 otherwise.
   
int vrpn_GlobalHapticsOrb::get_report(void)
{
   //--------------------------------------------------------------------
   // The reports are each _expected_chars characters long (for the Orb,
   // this is only one character so it is not very exciting.
   //--------------------------------------------------------------------

   if (d_status == STATUS_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, d_buffer, 1) != 1) {
      	return 0;
      }

      d_bufcount = 1;
      vrpn_gettimeofday(&d_timestamp, NULL);

      // Respond to the command, ignore it, or throw an error if it is
      // one we don't know how to deal with.
      switch (d_buffer[0]) {
	// Button press and release codes for buttons 0 through 25
	case 0x89: buttons[0] = 1; break;   case 0xb9: buttons[0] = 0; break;
	case 0x88: buttons[1] = 1; break;   case 0xb8: buttons[1] = 0; break;
	case 0x90: buttons[2] = 1; break;   case 0xc0: buttons[2] = 0; break;
	case 0x8e: buttons[3] = 1; break;   case 0xbe: buttons[3] = 0; break;
	case 0x8d: buttons[4] = 1; break;   case 0xbd: buttons[4] = 0; break;
	case 0x8c: buttons[5] = 1; break;   case 0xbc: buttons[5] = 0; break;
	case 0x8b: buttons[6] = 1; break;   case 0xbb: buttons[6] = 0; break;
	case 0x8a: buttons[7] = 1; break;   case 0xba: buttons[7] = 0; break;
	case 0x9b: buttons[8] = 1; break;   case 0xcb: buttons[8] = 0; break;
	case 0x9a: buttons[9] = 1; break;   case 0xca: buttons[9] = 0; break;
	case 0x99: buttons[10] = 1; break;  case 0xc9: buttons[10] = 0; break;
	case 0x98: buttons[11] = 1; break;  case 0xc8: buttons[11] = 0; break;
	case 0x9f: buttons[12] = 1; break;  case 0xcf: buttons[12] = 0; break;
	case 0x9e: buttons[13] = 1; break;  case 0xce: buttons[13] = 0; break;
	case 0x9d: buttons[14] = 1; break;  case 0xcd: buttons[14] = 0; break;
	case 0x9c: buttons[15] = 1; break;  case 0xcc: buttons[15] = 0; break;
	case 0x81: buttons[16] = 1; break;  case 0xb1: buttons[16] = 0; break;
	case 0x80: buttons[17] = 1; break;  case 0xb0: buttons[17] = 0; break;
	case 0x8f: buttons[18] = 1; break;  case 0xbf: buttons[18] = 0; break;
	case 0x86: buttons[19] = 1; break;  case 0xb6: buttons[19] = 0; break;
	case 0x85: buttons[20] = 1; break;  case 0xb5: buttons[20] = 0; break;
	case 0x84: buttons[21] = 1; break;  case 0xb4: buttons[21] = 0; break;
	case 0x83: buttons[22] = 1; break;  case 0xb3: buttons[22] = 0; break;
	case 0x82: buttons[23] = 1; break;  case 0xb2: buttons[23] = 0; break;
	case 0xa0: buttons[24] = 1; break;  case 0xd0: buttons[24] = 0; break;
	case 0x87: buttons[25] = 1; break;  case 0xb7: buttons[25] = 0; break;

        // Pushbuttons are mapped as buttons 26 (left) and 27 (right)
	case 0xa1: buttons[26] = 1; break;  case 0xd1: buttons[26] = 0; break;
	case 0xa2: buttons[27] = 1; break;  case 0xd2: buttons[27] = 0; break;

	// Rocker up is mapped as buttons 28; rocker down is button 29
	case 0x92: buttons[28] = 1; break;  case 0xc2: buttons[28] = 0; break;
	case 0x91: buttons[29] = 1; break;  case 0xc1: buttons[29] = 0; break;

	// Thumbwheel left is negative, right is positive for dial 0.
	// Increment/decrement by the number revolutions per tick to
	// turn it into a dial value.
	case 0xE1:
	  dials[0] -= REV_PER_TICK_WHEEL;
	  channel[0] -= REV_PER_TICK_WHEEL;
	  if (channel[0] < -1.0) { channel[0] = -1.0; };
	  break;
	case 0xE0:
	  dials[0] += REV_PER_TICK_WHEEL;
	  channel[0] += REV_PER_TICK_WHEEL;
	  if (channel[0] > 1.0) { channel[0] = 1.0; };
	  break;

	// Trackball is two analogs and two dials: analog/dial 1
	// is positive for north and negative for south.
	// Analog/dial 2 is positive for east and
	// negative for west.  Increment/decrement by the number of
	// revolutions per tick for the trackball.
	case 0xF2:
	  dials[1] += REV_PER_TICK_BALL;
	  channel[1] += REV_PER_TICK_BALL;
	  if (channel[1] > 1.0) { channel[1] = 1.0; };
	  break;
	case 0xF3:
	  dials[1] -= REV_PER_TICK_BALL;
	  channel[1] -= REV_PER_TICK_BALL;
	  if (channel[1] < -1.0) { channel[1] = -1.0; };
	  break;
	case 0xF0:
	  dials[2] += REV_PER_TICK_BALL;
	  channel[2] += REV_PER_TICK_BALL;
	  if (channel[2] > 1.0) { channel[2] = 1.0; };
	  break;
	case 0xF1:
	  dials[2] -= REV_PER_TICK_BALL;
	  channel[2] -= REV_PER_TICK_BALL;
	  if (channel[2] < -1.0) { channel[2] = -1.0; };
	  break;

	// There are several commands that are marked as "to be ignored."
	case 0xfd:
	case 0x00:
	case 0xfe:
	case 0x10:
	case 0xfb:
	case 0xfc:
	  return 1;  // We got a report, we're just not doing anything about it.

	default:
	  send_text_message("vrpn_GlobalHapticsOrb::get_report(): Unknown character", d_timestamp, vrpn_TEXT_ERROR);
	  d_status = STATUS_RESETTING;
	  return 0;
      }
   } else {
     send_text_message("vrpn_GlobalHapticsOrb::get_report(): Unknown mode, programming error.", d_timestamp, vrpn_TEXT_ERROR);
     d_status = STATUS_RESETTING;
     return 0;
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing.
   //--------------------------------------------------------------------

   report_changes();
   d_status = STATUS_SYNCING;
   d_bufcount = 0;
   return 1;
}

void	vrpn_GlobalHapticsOrb::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = d_timestamp;
	vrpn_Button::timestamp = d_timestamp;
	vrpn_Dial::timestamp = d_timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void	vrpn_GlobalHapticsOrb::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = d_timestamp;
	vrpn_Button::timestamp = d_timestamp;
	vrpn_Dial::timestamp = d_timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the Orb,
// either trying to reset it or trying to get a reading from it.
void	vrpn_GlobalHapticsOrb::mainloop()
{
  struct  timeval last_poll_sent = {0,0};

  // Call the generic server mainloop, since we are a server
  server_mainloop();

  switch(d_status) {
    case STATUS_RESETTING:
      reset();
      break;

    case STATUS_SYNCING:
      {
	// It turns out to be important to get the report before checking
	// to see if it has been too long since the last report.  This is
	// because there is the possibility that some other device running
	// in the same server may have taken a long time on its last pass
	// through mainloop().  Trackers that are resetting do this.  When
	// this happens, you can get an infinite loop -- where one tracker
	// resets and causes the other to timeout, and then it returns the
	// favor.  By checking for the report here, we reset the timestamp
	// if there is a report ready (ie, if THIS device is still operating).
	while (get_report()) {};    // Keep getting reports as long as they come
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);

	// If we haven't heard in a while (this can be normal), send a reset
	// request to the device -- this will cause a response of 0xfc, which
	// will be ignored when it arrives.  Reset the poll interval when a
	// poll is sent.
	if ( vrpn_TimevalDuration(current_time, d_timestamp) > POLL_INTERVAL ) {
	  last_poll_sent = current_time;
	  vrpn_write_characters(serial_fd, &reset_char, 1);
	}

	// If we still haven't heard from the device after a longer time,
	// fail and go into reset mode.
	if ( vrpn_TimevalDuration(current_time, d_timestamp) > TIMEOUT_INTERVAL ) {
	  send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
	  d_status = STATUS_RESETTING;
	}
      }
      break;

    default:
      send_text_message("vrpn_GlobalHapticsOrb: Unknown mode (internal error), resetting", d_timestamp, vrpn_TEXT_ERROR);
      d_status = STATUS_RESETTING;
      break;
  }
}
