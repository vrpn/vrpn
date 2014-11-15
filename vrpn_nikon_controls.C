//XXXX Change this to use relative motion commands but still use
// absolute position requests.  This will both reduce the number
// of characters to send (and time) and avoid the problem of the
// device being reset at an out-of-range place.

#include <stdio.h>                      // for fprintf, stderr, sprintf, etc
#include <stdlib.h>                     // for atoi
#include <string.h>                     // for strlen, NULL, strncmp, etc

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"
#include "vrpn_nikon_controls.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

//#define	VERBOSE
//#define VERBOSE2

/// Characters that can start a report of the type we recognize.
const char *VALID_REPORT_CHARS = "anqo";

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define TIMEOUT_TIME_INTERVAL   (10000000L)  // max time between reports (usec)
#define POLL_INTERVAL		(1000000L)  // time to poll if no response in a while (usec)

vrpn_Nikon_Controls::vrpn_Nikon_Controls(const char *device_name, vrpn_Connection *con, const char *port_name)
: vrpn_Serial_Analog(device_name, con, port_name), 
  vrpn_Analog_Output(device_name, con)
{
  num_channel = 1;	// Focus control
  o_num_channel = 1;	// Focus control

  last_poll.tv_sec = 0;
  last_poll.tv_usec = 0;

  // Set the mode to reset
  _status = STATUS_RESETTING;

  // Register to receive the message to request changes and to receive connection
  // messages.
  if (d_connection != NULL) {
    if (register_autodeleted_handler(request_m_id, handle_request_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Nikon_Controls: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(request_channels_m_id, handle_request_channels_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Nikon_Controls: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(d_ping_message_id, handle_connect_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Nikon_Controls: can't register handler\n");
	  d_connection = NULL;
    }
  } else {
	  fprintf(stderr,"vrpn_Nikon_Controls: Can't get connection!\n");
  }    
}

/// Looks up the response from the table in the appendix of the Nikon
/// control manual.
static	const char *error_code_to_string(const int code)
{
  switch (code) {
    case 1: return "Command error";
    case 2: return "Operand error";
    case 3: return "Procedding timout error";
    case 4: return "Receive buffer overflow error";
    case 5: return "Device not mounted";
    case 6: return "Drive count overflow error";
    case 7: return "Processing prohibited error";
    default: return "unknown error";
  }
}

/// Parses the response position from the Nikon found in inbuf.  If a report
/// is found that sets the response, then 1 is returned from
/// the function call.  If no response is found, 0 is returned both from
/// the function and in response_pos.  If there is an error response found
/// in the buffer, -1 is returned response_pos is set to the error code that
/// was found.  If a report is found that does not set the response but that
/// indicates completion of a command, then 2 is returned and response_pos is
/// not set.  If a report is found that indicates it is in the process of
/// doing a command, 3 is returned and response_pos is not set.

static	int parse_focus_position_response(const char *inbuf, double &response_pos)
{
  // Make sure that the command ends with [CR][LF].  All valid reports should end
  // with this.
  size_t  len = strlen((const char *)inbuf);
  if (len < 2) {
    fprintf(stderr,"parse_focus_position_response(): String too short\n");
    response_pos = 0; return 0;
  }
  if ( (inbuf[len-2] != '\r') || (inbuf[len-1] != '\n') ) {
    fprintf(stderr,"parse_focus_position_response(): (%s) Not [CR][LF] at end\n",inbuf);
    response_pos = 0; return 0;
  }

  //-------------------------------------------------------------------------
  // Parse the report based on the first part of the report.

#ifdef	VERBOSE
  printf("Nikon control: parsing [%s]\n", inbuf);
#endif
  // Reported focus
  if (strncmp(inbuf, "aSPR", strlen("aSPR")) == 0) {
    response_pos = atoi(&inbuf[4]);
    return 1;

  // Error message
  } else if (strncmp(inbuf, "nSPR", strlen("nSPR")) == 0) {
    response_pos = atoi(&inbuf[4]);
    return -1;

  // In the process of moving focus where it was requested
  } else if (strncmp(inbuf, "qSMVA", strlen("qSMVA")) == 0) {
    return 3;

  // Done moving focus where it was requested
  } else if (strncmp(inbuf, "oSMV", strlen("oSMV")) == 0) {
    return 2;

  // Error moving focus where it was requested
  } else if (strncmp(inbuf, "nSMV", strlen("nSMV")) == 0) {
    response_pos = atoi(&inbuf[4]);
    return -1;

  // Don't know how to parse the command.
  } else {
    fprintf(stderr,"parse_focus_position_response(): Unknown response (%s)\n", inbuf);
    response_pos = 0; return 0;      
  }
}

// This function will flush any characters in the input buffer, then ask the
// Nikon for the state of its focus control.  If it gets a valid response, all is
// well; otherwise, not.
//   Commands		  Responses		  Meanings
//   rSPR[CR]		  aSPR[value][CR][LF]	  Request focus: value tells the focus in number of ticks
//                        nSPR[errcode][CR][LF]	  errcode tells what went wrong
//   fSMV[value][CR]	  qSMVA[CR][LF]		  Set focus to value: q command says in progress
//                        oSMV[CR][LF]		  o says that the requested position has been obtained

int	vrpn_Nikon_Controls::reset(void)
{
	unsigned char	inbuf[256];
	int	ret;
	char	errmsg[256];
	char	cmd[256];
	double	response_pos; //< Where the focus says it is.

	//-----------------------------------------------------------------------
	// Sleep a second and then drain the input buffer to make sure we start
	// with a fresh slate.
	vrpn_SleepMsecs(1000);
	vrpn_flush_input_buffer(serial_fd);

	//-----------------------------------------------------------------------
	// Send the command to request the focus.  Then wait 1 second for a response
	sprintf(cmd, "rSPR\r");
	if (vrpn_write_characters(serial_fd, (unsigned char *)cmd, strlen(cmd)) != (int)strlen(cmd)) {
	  fprintf(stderr,"vrpn_Nikon_Controls::reset(): Cannot send focus request\n");
	  return -1;
	}
	vrpn_SleepMsecs(1000);

	//-----------------------------------------------------------------------
	// Read the response from the camera and then see if it is a good response,
	// an error message, or nothing.

	ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf));
	if (ret < 0) {
	  perror("vrpn_Nikon_Controls::reset(): Error reading position from device");
	  return -1;
	}
	if (ret == 0) {
	  fprintf(stderr, "vrpn_Nikon_Controls::reset(): No characters when reading position from device\n");
	  return -1;
	}
	inbuf[ret] = '\0';  //< Null-terminate the input string
	ret = parse_focus_position_response((char *)inbuf, response_pos);
	if (ret < 0) {
	  fprintf(stderr,"vrpn_Nikon_Controls::reset(): Error reading focus: %s\n",
	    error_code_to_string((int)(response_pos)));
	  return -1;
	}
	if (ret != 1) {
	  fprintf(stderr,"vrpn_Nikon_Controls::reset(): Unexpected response to focus request\n");
	  return -1;
	}
	channel[0] = response_pos;

	sprintf(errmsg,"Focus reported (this is good)");
	VRPN_MSG_WARNING(errmsg);

	// We're now waiting for any responses from devices
	status = STATUS_SYNCING;

	VRPN_MSG_WARNING("reset complete (this is good)");

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}


//   This function will read characters until it has a full report, then
// put that report into analog field and call the report method on that.
//   The time stored is that of the first character received as part of the
// report.
   
int vrpn_Nikon_Controls::get_report(void)
{
   int ret;		// Return value from function call to be checked

   //--------------------------------------------------------------------
   // If we're SYNCing, then the next character we get should be the start
   // of a report.  If we recognize it, go into READing mode and tell how
   // many characters we expect total. If we don't recognize it, then we
   // must have misinterpreted a command or something; reset
   // and start over
   //--------------------------------------------------------------------

   if (status == STATUS_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, _buffer, 1) != 1) {
      	return 0;
      }

      // See if we recognize the character as one of the ones making
      // up a report.  If not, then we need to continue syncing.
      if (strchr(VALID_REPORT_CHARS, _buffer[0]) == NULL) {
	VRPN_MSG_WARNING("Syncing");
	return 0;
      }

      // Got the first character of a report -- go into READING mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the rest of the report.
      // The time stored here is as close as possible to when the
      // report was generated.
      _bufcount = 1;
      vrpn_gettimeofday(&timestamp, NULL);
      status = STATUS_READING;
#ifdef	VERBOSE2
      printf("... Got the 1st char\n");
#endif
   }

   //--------------------------------------------------------------------
   // Read a character at a time from the serial port, storing them
   // in the buffer.  Do this until we either run out of characters to
   // read or else find [CR][LF] along the way, which indicates the end
   // of a command.
   //--------------------------------------------------------------------

   do {
     ret = vrpn_read_available_characters(serial_fd, &_buffer[_bufcount], 1);
     if (ret == -1) {
	  VRPN_MSG_ERROR("Error reading");
	  status = STATUS_RESETTING;
	  return 0;
     }
     _bufcount += ret;
#ifdef	VERBOSE2
     if (ret != 0) printf("... got %d characters (%d total)\n",ret, _bufcount);
#endif

     // See if the last characters in the buffer are [CR][LF].  If so, then
     // we have a full report so null-terminate and go ahead and parse
     if ( (_bufcount > 2) &&
	  (_buffer[_bufcount-2] == '\r') &&
	  (_buffer[_bufcount-1] == '\n') ) {
       _buffer[_bufcount] = '\0';
       break;
     }

     // If we have a full buffer, then we've gotten into a bad way.
     if (_bufcount >= sizeof(_buffer) - 1) {
       VRPN_MSG_ERROR("Buffer full when reading");
       status = STATUS_RESETTING;
       return 0;
     }
   } while (ret != 0);
   if (ret == 0) {
     return 0;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check it by
   // trying to parse it.  If it is not valid, then we return to
   // synch mode and ignore this report. A well-formed report has
   // on of VALID_REPORT_CHARS as its command.  We expect these responses
   // either set the response position (due to a query) or to be valid
   // but not set the response position (due to a position move request,
   // for which we store the requested position into the value).
   //--------------------------------------------------------------------

   double response_pos;
   ret = parse_focus_position_response((const char *)_buffer, response_pos);
   if (ret <= 0) {
     fprintf(stderr,"Bad response from nikon [%s], syncing\n", _buffer);
     fprintf(stderr,"  (%s)\n", error_code_to_string((int)response_pos));
     status = STATUS_SYNCING;
     return 0;
   }
   // Type 3 returns mean "in progress" for some function.  Ignore this
   // report.
   if (ret == 3) {
     status = STATUS_SYNCING;
     _bufcount = 0;
     return 1;
   }

   // Type 2 means that the command we issued before has completed -- put
   // the value we requested for the focus into the response since that is
   // where we are now.
   if (ret == 2) {  // We must have gotten where we are going.
     response_pos = _requested_focus;
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   channel[0] = response_pos;
   report_changes();
   status = STATUS_SYNCING;
   _bufcount = 0;

   return 1;
}

/** Set the channel associated with the index to the specified value.
    Channel 0 controls the focus.
    **/

int vrpn_Nikon_Controls::set_channel(unsigned chan_num, vrpn_float64 value)
{
  // Ask to change the focus if this was channel 0.  Also, store away where we
  // asked the focus to go to so that we can know where we are when we get there
  // (the response from the microscope just says "we got here!).
  if ( (int)chan_num >= o_num_channel ) {
    char msg[1024];
    sprintf(msg,"vrpn_Nikon_Controls::set_channel(): Index out of bounds (%d of %d), value %lg\n",
      chan_num, o_num_channel, value);
    vrpn_gettimeofday(&o_timestamp, NULL);
    send_text_message(msg, o_timestamp, vrpn_TEXT_ERROR);
  }

  char  msg[256];
  sprintf(msg, "fSMV%ld\r", (long)value);
#ifdef	VERBOSE
  printf("Nikon Control: Asking to move to %ld with command %s\n", (long)value, msg);
#endif
  if (vrpn_write_characters(serial_fd, (unsigned char *)msg, strlen(msg)) != (int)strlen(msg)) {
    fprintf(stderr, "vrpn_Nikon_Controls::set_channel(): Can't write command\n");
    return -1;
  }
  if (vrpn_drain_output_buffer(serial_fd)) {
    fprintf(stderr, "vrpn_Nikon_Controls::set_channel(): Can't drain command\n");
    return -1;
  }
  _requested_focus = value;

  return 0;
}

// If the client requests a value for the focus control, then try to set the
// focus to the requested value.
int vrpn_Nikon_Controls::handle_request_message(void *userdata, vrpn_HANDLERPARAM p)
{
    const char	  *bufptr = p.buffer;
    vrpn_int32	  chan_num;
    vrpn_int32	  pad;
    vrpn_float64  value;
    vrpn_Nikon_Controls *me = (vrpn_Nikon_Controls *)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the position to the appropriate value, if the channel number is in the
    me->set_channel(chan_num, value);
    return 0;
}

int vrpn_Nikon_Controls::handle_request_channels_message(void* userdata, vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_Nikon_Controls* me = (vrpn_Nikon_Controls *)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) {
      char msg[1024];
      sprintf(msg,"vrpn_Nikon_Controls::handle_request_channels_message(): Index out of bounds (%d of %d), clipping\n",
	num, me->o_num_channel);
      me->send_text_message(msg, me->timestamp, vrpn_TEXT_ERROR);
      num = me->o_num_channel;
    }
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
	me->set_channel(i, me->o_channel[i]);
    }

    return 0;
}

/** When we get a connection request from a remote object, send our state so
    they will know it to start with. */
int vrpn_Nikon_Controls::handle_connect_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Nikon_Controls *me = (vrpn_Nikon_Controls *)userdata;

    me->report(vrpn_CONNECTION_RELIABLE);
    return 0;
}

void	vrpn_Nikon_Controls::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
}

void	vrpn_Nikon_Controls::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;

	vrpn_Analog::report(class_of_service);
}


/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds.
*/

void	vrpn_Nikon_Controls::mainloop()
{
  char errmsg[256];

  server_mainloop();

  switch(status) {
    case STATUS_RESETTING:
	reset();
	break;

    case STATUS_SYNCING:
    case STATUS_READING:
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
	    while (get_report()) {};	// Keep getting reports so long as there are more

	    struct timeval current_time;
	    vrpn_gettimeofday(&current_time, NULL);
	    if ( vrpn_TimevalDuration(current_time,timestamp) > POLL_INTERVAL) {

	      if (vrpn_TimevalDuration(current_time, last_poll) > TIMEOUT_TIME_INTERVAL) {
		// Ask the unit for its current focus location, which will cause it to respond.
		char  msg[256];
		sprintf(msg, "rSPR\r");
		if (vrpn_write_characters(serial_fd, (unsigned char *)msg, strlen(msg)) != (int)strlen(msg)) {
		  fprintf(stderr, "vrpn_Nikon_Controls::mainloop(): Can't write focus request command\n");
		  status = STATUS_RESETTING;
		  return;
		}
	        vrpn_gettimeofday(&last_poll, NULL);
	      } else {
		return;
	      }
	    }

	    if ( vrpn_TimevalDuration(current_time,timestamp) > TIMEOUT_TIME_INTERVAL) {
		    sprintf(errmsg,"Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
					current_time.tv_sec, static_cast<long>(current_time.tv_usec),
					timestamp.tv_sec, static_cast<long>(timestamp.tv_usec));
		    VRPN_MSG_ERROR(errmsg);
		    status = STATUS_RESETTING;
	    }
      }
        break;

    default:
	VRPN_MSG_ERROR("Unknown mode (internal error)");
	break;
  }
}
