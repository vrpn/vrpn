// vrpn_IDEA.C

// See http://www.haydonkerk.com/LinkClick.aspx?fileticket=LEcwYeRmKVg%3d&tabid=331
// for the software manual for this device.

#include <string.h>
#include "vrpn_IDEA.h"
#include "vrpn_Shared.h"
#include "vrpn_Serial.h"

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define	IDEA_INFO(msg)	    { send_text_message(msg, d_timestamp, vrpn_TEXT_NORMAL) ; if (d_connection) d_connection->send_pending_reports(); }
#define	IDEA_WARNING(msg)    { send_text_message(msg, d_timestamp, vrpn_TEXT_WARNING) ; if (d_connection) d_connection->send_pending_reports(); }
#define	IDEA_ERROR(msg)	    { send_text_message(msg, d_timestamp, vrpn_TEXT_ERROR) ; if (d_connection) d_connection->send_pending_reports(); }

#define TIMEOUT_TIME_INTERVAL   (2000000L) // max time between reports (usec)
#define POLL_INTERVAL		(1000000L) // time to poll if no response in a while (usec)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


// This creates a vrpn_IDEA and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.

vrpn_IDEA::vrpn_IDEA (const char * name, vrpn_Connection * c,
			const char * port):
	vrpn_Serial_Analog(name, c, port, 57600),
        vrpn_Analog_Output(name, c)
{
  num_channel = 1;
  channel[0] = 0;

  o_num_channel = 1;

  // Set the mode to reset
  status = STATUS_RESETTING;

  // Register to receive the message to request changes and to receive connection
  // messages.
  if (d_connection != NULL) {
    if (register_autodeleted_handler(request_m_id, handle_request_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_IDEA: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(request_channels_m_id, handle_request_channels_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_IDEA: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(d_ping_message_id, handle_connect_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_IDEA: can't register handler\n");
	  d_connection = NULL;
    }
  } else {
	  fprintf(stderr,"vrpn_IDEA: Can't get connection!\n");
  }
}

// Add a newline-termination to the command and then send it.
bool vrpn_IDEA::send_command(const char *cmd)
{
  char buf[128];
  size_t len = strlen(cmd);
  if (len > sizeof(buf)-2) { return false; }
  strcpy(buf, cmd);
  buf[len] = '\r';
  buf[len+1] = '\0';
  return (vrpn_write_characters(serial_fd, 
    (const unsigned char *)((void*)(buf)), strlen(buf)) == strlen(buf) );
}

//   Commands		  Responses	           Meanings
//    M                     None                      Move to position
// Params:
//  distance: distance in 1/64th steps
//  run speed: steps/second
//  start speed: steps/second
//  end speed: steps/second
//  accel rate: steps/second/second
//  decel rate: steps/second/second
//  run current: milliamps
//  hold current: milliamps
//  accel current: milliamps
//  decel current: milliamps
//  delay: milliseconds, waiting to drop to hold current
//  step mode: inverse step size: 4 is 1/4 step.

bool  vrpn_IDEA::send_move_request(vrpn_float64 location_in_steps)
{
  char  cmd[512];
  long steps_64th = static_cast<long>(location_in_steps*64);
  int run_speed = 3200;
  int start_speed = 1200;
  int end_speed = 2000;
  int accel_rate = 40000;
  int decel_rate = 100000;
  int run_current = 290;
  int hold_current = 290;
  int accel_current = 290;
  int decel_current = 290;
  int delay = 50;
  int step = 8;
  sprintf(cmd, "M%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    steps_64th,
    run_speed,
    start_speed,
    end_speed,
    accel_rate,
    decel_rate,
    run_current,
    hold_current,
    accel_current,
    decel_current,
    delay,
    step);
  return send_command(cmd);
}

// This routine will reset the IDEA drive.
//   Commands		  Responses	            Meanings
//    l                     `l<value>[cr]`l#[cr]      Location of the drive

bool  vrpn_IDEA::convert_report_to_value(unsigned char *buf, vrpn_float64 *value)
{
  int  data;
  if (sscanf((char *)(buf), "`l%d\r`l#\r", &data) != 1) {
    return false;
  }

  // The location of the drive is in 64th-of-a-tick units, so need
  // to divide by 64 to find the actual location.
  (*value) = data/64.0;
  return true;
}


// This routine will reset the IDEA drive.
//   Commands		  Responses	           Meanings
//    R                     None                      Software reset
//    f                     `f<value>[cr]`f#[cr]      Request fault status
//    l                     `l<value>[cr]`l#[cr]      Location of the drive

int	vrpn_IDEA::reset(void)
{
	struct	timeval	timeout;
	unsigned char	inbuf[128];
	int	ret;

	//-----------------------------------------------------------------------
	// Drain the input buffer to make sure we start with a fresh slate.
	// Also wait a bit to let things clear out.
        vrpn_SleepMsecs(250);
	vrpn_flush_input_buffer(serial_fd);

	//-----------------------------------------------------------------------
        // Reset the driver, then wait briefly to let it reset.
        if (!send_command("R")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not send reset command\n");
          return -1;
        }
        vrpn_SleepMsecs(250);

	//-----------------------------------------------------------------------
        // Ask for the fault status of the drive.  This should cause it to respond
        // with an "f" followed by a number and a carriage return.  We want the
        // number to be zero.
        if (!send_command("f")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not request fault status\n");
          return -1;
        }

        timeout.tv_sec = 0;
	timeout.tv_usec = 30000;

        ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
        if (ret < 0) {
          perror("vrpn_IDEA::reset(): Error reading fault status from device");
	  return -1;
        }
        if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
          inbuf[ret] = '\0';
          fprintf(stderr,"vrpn_IDEA::reset(): Bad fault status report (length %d): %s\n", ret, inbuf);
          IDEA_ERROR("Bad fault status report");
          return -1;
        }
        inbuf[ret] = '\0';

        int fault_status;
        if (sscanf((char *)(inbuf), "`f%d\r`f#\r", &fault_status) != 1) {
          fprintf(stderr,"vrpn_IDEA::reset(): Bad fault status report: %s\n", inbuf);
          IDEA_ERROR("Bad fault status report");
          return -1;
        }
        if (fault_status != 0) {
          IDEA_ERROR("Drive reports a fault");
          return -1;
        }

	//-----------------------------------------------------------------------
        // Ask for the position of the drive and make sure we can read it.
        if (!send_command("l")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not request position\n");
          return -1;
        }

        timeout.tv_sec = 0;
	timeout.tv_usec = 30000;

        ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
        if (ret < 0) {
          perror("vrpn_IDEA::reset(): Error reading position from device");
	  return -1;
        }
        if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
          inbuf[ret] = '\0';
          fprintf(stderr,"vrpn_IDEA::reset(): Bad position report: %s\n", inbuf);
          IDEA_ERROR("Bad position report");
          return -1;
        }
        inbuf[ret] = '\0';

        vrpn_float64 position;
        if (!convert_report_to_value(inbuf, &position)) {
          fprintf(stderr,"vrpn_IDEA::reset(): Bad position report: %s\n", inbuf);
          IDEA_ERROR("Bad position report");
          return -1;
        }
        channel[0] = position;

	//-----------------------------------------------------------------------
        // Ask for the position of the drive so that it will start sending.
        // them to us.  Each time we finish getting a report, we request
        // another one.
        if (!send_command("l")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not request position\n");
          return -1;
        }

	// We're now waiting for any responses from devices
	IDEA_WARNING("reset complete (this is good)");
	vrpn_gettimeofday(&d_timestamp, NULL);	// Set watchdog now
	status = STATUS_SYNCING;
	return 0;
}

//   This function will read characters until it has a full report, then
// put that report into analog fields and call the report methods on these.
//   The time stored is that of the first character received as part of the
// report.
   
int vrpn_IDEA::get_report(void)
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
      if (vrpn_read_available_characters(serial_fd, d_buffer, 1) != 1) {
      	return 0;
      }

      // Got the first character of a report -- go into READING mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the rest of the report.
      // The time stored here is as close as possible to when the
      // report was generated.
      d_bufcount = 1;
      vrpn_gettimeofday(&d_timestamp, NULL);
      status = STATUS_READING;
#ifdef	VERBOSE
      printf("... Got the 1st char\n");
#endif
   }

   //--------------------------------------------------------------------
   // Read as many bytes of this report as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.
   //--------------------------------------------------------------------

   ret = vrpn_read_available_characters(serial_fd, &d_buffer[d_bufcount],
		sizeof(d_buffer)-d_bufcount);
   if (ret == -1) {
	IDEA_ERROR("Error reading");
	status = STATUS_RESETTING;
	return 0;
   }
   d_bufcount += ret;
#ifdef	VERBOSE
   if (ret != 0) printf("... got %d characters (%d total)\n",ret, d_bufcount);
#endif

   //--------------------------------------------------------------------
   // See if we can parse this report.  If not, we assume that we do not
   // have a complete report yet and so return.
   //--------------------------------------------------------------------

   vrpn_float64 value;
   if (!convert_report_to_value(d_buffer, &value)) {
     return 0;
   }

   //--------------------------------------------------------------------
   // Store the value in it into the analog value.  Request another report
   // so we can keep them coming.
   //--------------------------------------------------------------------

   channel[0] = value;

#ifdef	VERBOSE
   printf("got a complete report (%d)!\n", d_bufcount);
#endif
    if (!send_command("l")) {
	IDEA_ERROR("Could not send position request, resetting");
	status = STATUS_RESETTING;
	return 0;
    }


   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
   status = STATUS_SYNCING;
   d_bufcount = 0;

   return 1;
}

int vrpn_IDEA::handle_request_message(void *userdata, vrpn_HANDLERPARAM p)
{
    const char	  *bufptr = p.buffer;
    vrpn_int32	  chan_num;
    vrpn_int32	  pad;
    vrpn_float64  value;
    vrpn_IDEA *me = (vrpn_IDEA *)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the position to the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      char msg[1024];
      sprintf(msg,"vrpn_IDEA::handle_request_message(): Index out of bounds (%d of %d), value %lg\n",
	chan_num, me->num_channel, value);
      me->send_text_message(msg, me->d_timestamp, vrpn_TEXT_ERROR);
      return 0;
    }
    // This will get set when we read from the motoer me->channel[chan_num] = value;
    me->send_move_request(value);

    return 0;
}

int vrpn_IDEA::handle_request_channels_message(void* userdata, vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_IDEA* me = (vrpn_IDEA *)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) {
      char msg[1024];
      sprintf(msg,"vrpn_IDEA::handle_request_channels_message(): Index out of bounds (%d of %d), clipping\n",
	num, me->o_num_channel);
      me->send_text_message(msg, me->d_timestamp, vrpn_TEXT_ERROR);
      num = me->o_num_channel;
    }
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
        me->send_move_request(me->o_channel[i]);
    }

    return 0;
}

/** When we get a connection request from a remote object, send our state so
    they will know it to start with. */
int vrpn_IDEA::handle_connect_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_IDEA *me = (vrpn_IDEA *)userdata;

    me->report(vrpn_CONNECTION_RELIABLE);
    return 0;
}

void	vrpn_IDEA::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = d_timestamp;

	vrpn_Analog::report_changes(class_of_service);
}

void	vrpn_IDEA::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = d_timestamp;

	vrpn_Analog::report(class_of_service);
}

/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds
*/

void	vrpn_IDEA::mainloop()
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
	    if ( duration(current_time,d_timestamp) > POLL_INTERVAL) {
	      static struct timeval last_poll = {0, 0};

              if (duration(current_time, last_poll) > TIMEOUT_TIME_INTERVAL) {
		// Send another request to the unit, in case we've somehow
                // dropped a request.
                if (!send_command("l")) {
                  IDEA_ERROR("Could not request position");
                  status = STATUS_RESETTING;
                }
	        vrpn_gettimeofday(&last_poll, NULL);
	      } else {
		return;
	      }
	    }

	    if ( duration(current_time,d_timestamp) > TIMEOUT_TIME_INTERVAL) {
		    sprintf(errmsg,"Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
					current_time.tv_sec, static_cast<long>(current_time.tv_usec),
					d_timestamp.tv_sec, static_cast<long>(d_timestamp.tv_usec));
		    IDEA_ERROR(errmsg);
		    status = STATUS_RESETTING;
	    }
      }
        break;

    default:
	IDEA_ERROR("Unknown mode (internal error)");
	break;
  }
}
