// vrpn_Zaber.C
//	This is a driver for the Zaber T-LA linear actuators.
// They plug in a daisy chain into a serial
// line and communicates using RS-232 (this is a raw-mode driver).
// You can find out more at www.zaber.com.  They have a manual with
// a section on the serial interface; this code is based on that.
// It was written in May 2002 by Russ Taylor.

// INFO about how the device communicates:
//  There are 6-byte commands.  Byte 0 is the device number (with
// device 0 meaning "all devices."  Byte 1 is the command number.
// Byte 2 is the LSB of the 4-byte signed integer data, byte 5 is
// the MSB (most-significant byte) of the data.


#include <stdio.h>                      // for sprintf, fprintf, stderr, etc

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"                // for vrpn_flush_input_buffer, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_unbuffer, etc
#include "vrpn_Zaber.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define TIMEOUT_TIME_INTERVAL   (2000000L) // max time between reports (usec)
#define POLL_INTERVAL		(1000000L)	  // time to poll if no response in a while (usec)


// This creates a vrpn_Zaber and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.

vrpn_Zaber::vrpn_Zaber (const char * name, vrpn_Connection * c,
			const char * port):
		vrpn_Serial_Analog(name, c, port),
        vrpn_Analog_Output(name, c)
{
  d_last_poll.tv_sec = 0;
  d_last_poll.tv_usec = 0;

  num_channel = 0;	// This is an estimate; will change when the reset happens
  o_num_channel = 0;

  // Set the mode to reset
  status = STATUS_RESETTING;

  // Register to receive the message to request changes and to receive connection
  // messages.
  if (d_connection != NULL) {
    if (register_autodeleted_handler(request_m_id, handle_request_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Zaber: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(request_channels_m_id, handle_request_channels_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Zaber: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(d_ping_message_id, handle_connect_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_Zaber: can't register handler\n");
	  d_connection = NULL;
    }
  } else {
	  fprintf(stderr,"vrpn_Zaber: Can't get connection!\n");
  }    

}

bool  vrpn_Zaber::send_command(unsigned char devicenum, unsigned char cmd, vrpn_int32 data)
{
  unsigned char command[128];

  // Fill in the device number and command number
  command[0] = devicenum;
  command[1] = cmd;

  // Fill in the data in the correct byte order
  command[2] = static_cast<unsigned char>(data & 0x000000FF);
  command[3] = static_cast<unsigned char>((data >> 8) & 0x000000FF);
  command[4] = static_cast<unsigned char>((data >> 16) & 0x000000FF);
  command[5] = static_cast<unsigned char>((data >> 24) & 0x000000FF);

  // Send the command to the serial port
  return (vrpn_write_characters(serial_fd, command, 6) == 6);
}

bool  vrpn_Zaber::send_command(unsigned char devnum, unsigned char cmd, unsigned char d0,
  unsigned char d1, unsigned char d2, unsigned char d3)
{
  unsigned char command[128];

  // Fill in the device number and command number
  command[0] = devnum;
  command[1] = cmd;

  // Fill in the data in the correct byte order
  command[2] = d0;
  command[3] = d1;
  command[4] = d2;
  command[5] = d3;

  // Send the command to the serial port
  return (vrpn_write_characters(serial_fd, command, 6) == 6);
}

/** Convert the four bytes that have been read into a signed integer value. */
vrpn_int32  vrpn_Zaber::convert_bytes_to_reading(const unsigned char *buf)
{
  vrpn_int32  data;

    data   = ((buf[0])       & 0x000000FF)
	   + ((buf[1] <<  8) & 0x0000FF00)
           + ((buf[2] << 16) & 0x00FF0000)
           + ((buf[3] << 24) & 0xFF000000);

    return data;
}


// This routine will reset the Zebers, asking them to renumber themselves and then polling
// to see how many there are in the chain.
//   Commands		  Responses	      Meanings
//   [0][2][0][0][0][0]	  None		      Renumber the devices on the chain (wait 1 second)
//   [X][23][0][0][0][0]  [X][23][Position]   Tell each to stop and see if get a reply (tells where they are)
//   [X][40][16][0][0][0]  [X][40][16][0][0][0] Set it to report position when moving linearly

int	vrpn_Zaber::reset(void)
{
	struct	timeval	timeout;
	unsigned char	inbuf[128];
	int	ret;
	char	errmsg[256];

	//-----------------------------------------------------------------------
	// Sleep a second and then drain the input buffer to make sure we start
	// with a fresh slate.
	vrpn_SleepMsecs(1000);
	vrpn_flush_input_buffer(serial_fd);

	//-----------------------------------------------------------------------
	// Send the command to request that the units renumber themselves.  Then
	// wait 1 second to give them time to do so.
	if (!send_command(0,2,0)) {
	  fprintf(stderr,"vrpn_Zaber::reset(): Cannot send renumber command, trying again\n");
	  return -1;
	}
	vrpn_SleepMsecs(1000);

	//-----------------------------------------------------------------------
	// Go one by one and request the units to stop.  This will cause them to
	// say where they are.  When one doesn't respond, we've figured out how
	// many we have!  Also, set it to report position when moving at constant
	// linear velocity and set the minimum step size and maximum step size
	// to be slower than the defaults to hopefully give more torque.

	num_channel = 0;
	o_num_channel = 0;
	do {
	  int expected_chars = 4*6;
	  vrpn_flush_input_buffer(serial_fd);
	  unsigned char channel_id = static_cast<unsigned char>(num_channel+1);
	  send_command(channel_id, 23, 0); vrpn_SleepMsecs(10);
	  send_command(channel_id, 40, 16, 0, 0, 0); vrpn_SleepMsecs(10);
	  send_command(channel_id, 41, 128, 0, 0, 0); vrpn_SleepMsecs(10);
	  send_command(channel_id, 42, 128, 0, 0, 0);

	  timeout.tv_sec = 0;
	  timeout.tv_usec = 30000;
	  ret = vrpn_read_available_characters(serial_fd, inbuf, expected_chars, &timeout);
	  if (ret < 0) {
		  perror("vrpn_Zaber::reset(): Error reading position from device");
		  return -1;
	  }
	  if (ret == 0) {
	    break;  //< No more devices found
	  }
	  if (ret != expected_chars) {
		  sprintf(errmsg,"reset: Got %d of %d expected characters for position\n",ret, expected_chars);
		  VRPN_MSG_ERROR(errmsg);
		  return -1;
	  }

	  // Make sure the string we got back is what we expected and set the value
	  // for this channel to what we got from the device.
	  if ( (inbuf[0] != num_channel+1) || (inbuf[1] != 23) ) {
	      VRPN_MSG_ERROR("reset: Bad response to device # request");
	      return -1;
	  }
	  channel[num_channel] = convert_bytes_to_reading(&inbuf[2]);

	  num_channel++;
	  o_num_channel++;

// Yes, I know the conditional expression is a constant!
#ifdef	_WIN32
#pragma warning ( disable : 4127 )
#endif
	} while (true);
#ifdef	_WIN32
#pragma warning ( default : 4127 )
#endif
	sprintf(errmsg,"found %d devices",num_channel);
	VRPN_MSG_WARNING(errmsg);

	// We're now waiting for any responses from devices
	status = STATUS_SYNCING;

	VRPN_MSG_WARNING("reset complete (this is good)");

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}

//   This function will read characters until it has a full report, then
// put that report into analog fields and call the report methods on these.
//   The time stored is that of the first character received as part of the
// report.
   
int vrpn_Zaber::get_report(void)
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

      d_expected_chars = 6;

      //XXX How do we know when we're out of sync?

      // Got the first character of a report -- go into READING mode
      // and record that we got one character at this time. The next
      // bit of code will attempt to read the rest of the report.
      // The time stored here is as close as possible to when the
      // report was generated.
      d_bufcount = 1;
      vrpn_gettimeofday(&timestamp, NULL);
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
		d_expected_chars-d_bufcount);
   if (ret == -1) {
	VRPN_MSG_ERROR("Error reading");
	status = STATUS_RESETTING;
	return 0;
   }
   d_bufcount += ret;
#ifdef	VERBOSE
   if (ret != 0) printf("... got %d characters (%d total)\n",ret, d_bufcount);
#endif
   if (d_bufcount < d_expected_chars) {	// Not done -- go back for more
	return 0;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check to make
   // sure that its format matches what we expect. If it does, the next
   // section will parse it. If it does not, we need to go back into
   // synch mode and ignore this report. A well-formed report has
   // either 23 or 10 as its command.  Also accept command number 1
   // (reset).  Also accept command 20 (go to absolute position).  Also
   // parse command 255 (out of range setting requested -- it puts the
   // actual position in place).
   //--------------------------------------------------------------------

   if ( (d_buffer[1] != 10) && (d_buffer[1] != 23) && (d_buffer[1] != 1)
       && (d_buffer[1] != 20) && (d_buffer[1] != 255) ) {
	   status = STATUS_SYNCING;
	   char msg[1024];
	   sprintf(msg,"Bad command type (%d) in report (ignoring this report)", d_buffer[1]);
      	   VRPN_MSG_WARNING(msg);
	   vrpn_flush_input_buffer(serial_fd);
	   return 0;
   }
   if (d_buffer[1] == 255) {
     VRPN_MSG_WARNING("Requested value out of range");
   }

#ifdef	VERBOSE
   printf("got a complete report (%d of %d)!\n", d_bufcount, d_expected_chars);
#endif

   //--------------------------------------------------------------------
   // Decode the report and store the values in it into the analog value
   //--------------------------------------------------------------------

   unsigned char chan = static_cast<unsigned char>(d_buffer[0] - 1);
   vrpn_int32 value = convert_bytes_to_reading(&d_buffer[2]);
   if (chan >= num_channel) {	// Unsigned, so can't be < 0
     char msg[1024];
     sprintf(msg,"Invalid channel (%d of %d), resetting", chan, num_channel);
     VRPN_MSG_ERROR(msg);
     status = STATUS_RESETTING;
   }
   channel[chan] = value;

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
   status = STATUS_SYNCING;
   d_bufcount = 0;

   return 1;
}

int vrpn_Zaber::handle_request_message(void *userdata, vrpn_HANDLERPARAM p)
{
    const char	  *bufptr = p.buffer;
    vrpn_int32	  chan_num;
    vrpn_int32	  pad;
    vrpn_float64  value;
    vrpn_Zaber *me = (vrpn_Zaber *)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the position to the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      char msg[1024];
      sprintf(msg,"vrpn_Zaber::handle_request_message(): Index out of bounds (%d of %d), value %lg\n",
	chan_num, me->num_channel, value);
      me->send_text_message(msg, me->timestamp, vrpn_TEXT_ERROR);
      return 0;
    }
    me->channel[chan_num] = value;
    me->send_command(static_cast<unsigned char>(chan_num+1),20,(vrpn_int32)value);

    return 0;
}

int vrpn_Zaber::handle_request_channels_message(void* userdata, vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_Zaber* me = (vrpn_Zaber *)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) {
      char msg[1024];
      sprintf(msg,"vrpn_Zaber::handle_request_channels_message(): Index out of bounds (%d of %d), clipping\n",
	num, me->o_num_channel);
      me->send_text_message(msg, me->timestamp, vrpn_TEXT_ERROR);
      num = me->o_num_channel;
    }
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
	me->send_command(static_cast<unsigned char>(i+1),20,(vrpn_int32)me->o_channel[i]);
    }

    return 0;
}

/** When we get a connection request from a remote object, send our state so
    they will know it to start with. */
int vrpn_Zaber::handle_connect_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Zaber *me = (vrpn_Zaber *)userdata;

    me->report(vrpn_CONNECTION_RELIABLE);
    return 0;
}

void	vrpn_Zaber::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
}

void	vrpn_Zaber::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;

	vrpn_Analog::report(class_of_service);
}

/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds
*/

void	vrpn_Zaber::mainloop()
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

	      if (vrpn_TimevalDuration(current_time, d_last_poll) > TIMEOUT_TIME_INTERVAL) {
		// Tell unit 1 to stop, which will cause it to respond.
		send_command(1,23,0);
	        vrpn_gettimeofday(&d_last_poll, NULL);
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
