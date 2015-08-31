// vrpn_BiosciencesTools.C
//	This is a driver for the BiosciencesTools temperature controller.
// It was written in October 2012 by Russ Taylor.

// INFO about how the device communicates, taken from the user manual:
// Note: They say that the device uses <cr>, but they also say that it
// uses \n for this character.  In fact, the \r character is <cr> and \n
// is newline.  In fact, \r is what works in the end.

/*
Using a standard DB-9 cable (female-female connectors on both ends with
straight-through connections from each pin)
connect the controller (middle DB-9 connector) to a serial port of your computer.
Set the serial port at 115,200 speed, 8 bits, 1 stop bit,
NONE parity, and Hardware flow control.
The following is the list of text commands supported.
NOTE: Each command should follow by \r <CR> code:
(The following notes LIE: There is no space before the . or before the C,
 and sometimes the C is an E.  Also, the order is incorrect.  The actual
 order is stage 1, bath 1, external 1, stage 2, bath 2, external 2.)
T1<CR> returns temperature readings from STAGE1 sensor: 37 .1 C
T2<CR> returns temperature readings from BATH1 sensor: 36 .9 C
T5<CR> returns SET temperature: 37 .0 C
T3<CR> returns temperature readings from STAGE2 sensor: 37 .1 C
T4<CR> returns temperature readings from BATH2 sensor: 36 .9 C
T6<CR> returns SET temperature: 37 .0 C
CTn<CR> returns readings from n (n=1 - STAGE1, 2 - BATH1, 3 - STAGE2, 4 - BATH2) sensor: 37 .1 C
ON<CR> turns temperature control ON
OFF<CR> turns temperature control OFF
S1 037 0<CR> sets reference temperature for channel I (NOTE: all four digits should be sent to the controller)
S2 037 0<CR> sets reference temperature for channel II
*/

#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for sprintf, fprintf, stderr, etc
#include <string.h>                     // for strlen, NULL

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_BiosciencesTools.h"
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_unbuffer, timeval, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define TIMEOUT_TIME_INTERVAL   (2000000L) // max time between reports (usec)


// This creates a vrpn_BiosciencesTools. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// It uses hardware flow control.

vrpn_BiosciencesTools::vrpn_BiosciencesTools (const char * name, vrpn_Connection * c,
			const char * port, float temp1, float temp2, bool control_on):
	vrpn_Serial_Analog(name, c, port, 115200, 8, vrpn_SER_PARITY_NONE, true),
        vrpn_Analog_Output(name, c),
        vrpn_Button_Filter(name, c)
{
  num_channel = 6;
  o_num_channel = 3;
  num_buttons = 1;
  buttons[0] = control_on;

  // Fill in the arguments to send to the device at reset time.
  o_channel[0] = temp1;
  o_channel[1] = temp2;
  o_channel[2] = control_on;

  // Set the mode to reset
  status = STATUS_RESETTING;

  // Register to receive the message to request changes and to receive connection
  // messages.
  if (d_connection != NULL) {
    if (register_autodeleted_handler(request_m_id, handle_request_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_BiosciencesTools: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(request_channels_m_id, handle_request_channels_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_BiosciencesTools: can't register handler\n");
	  d_connection = NULL;
    }
    if (register_autodeleted_handler(d_ping_message_id, handle_connect_message,
      this, d_sender_id)) {
	  fprintf(stderr,"vrpn_BiosciencesTools: can't register handler\n");
	  d_connection = NULL;
    }
  } else {
	  fprintf(stderr,"vrpn_BiosciencesTools: Can't get connection!\n");
  }

}

// Command format described in document:
// S1 037 0<CR> sets reference temperature for channel 1
// (NOTE: all four digits should be sent to the controller)
// Actual command format:
// S1 0370<CR> Sets reference temperature for channel 1 to 37.0 deg C
// S2 0421<CR> Sets reference temperature for channel 2 to 42.1 deg C
        
bool  vrpn_BiosciencesTools::set_reference_temperature(unsigned channel, float value)
{
  char command[128];

  // Fill in the command with the zero-padded integer output for
  // above the decimal and then a single value for the first point
  // past the decimal.
  int whole = static_cast<int>(value);
  int dec = static_cast<int>(value*10) - whole*10;
  sprintf(command, "S%d %03d%d\r", channel+1, whole,dec);

  // Send the command to the serial port
  return (static_cast<size_t>(vrpn_write_characters(serial_fd, (unsigned char *)(command), strlen(command))) == strlen(command));
}

// Command format:
// ON<CR> sets control on
// OFF<CR> sets control off
        
bool  vrpn_BiosciencesTools::set_control_status(bool on)
{
  char command[128];

  if (on) {
    sprintf(command, "ON\r");
  } else {
    sprintf(command, "OFF\r");
  }

  // Send the command to the serial port
  return (static_cast<size_t>(vrpn_write_characters(serial_fd, (unsigned char *)(command), strlen(command))) == strlen(command));
}

// Command format:
// T1<CR> returns temperature readings from STAGE1 sensor: 37.1C
// T2<CR> returns temperature readings from BATH1 sensor: 36.9C
// T5<CR> returns SET temperature: 37.0C
// T3<CR> returns temperature readings from STAGE2 sensor: 37.1C
// T4<CR> returns temperature readings from BATH2 sensor: 36.9C
// T6<CR> returns SET temperature: 37.0C
// NOTE: Sometimes the C is an E when there is no reading.
        
bool  vrpn_BiosciencesTools::request_temperature(unsigned channel)
{
  char command[128];

  sprintf(command, "T%d\r", channel+1);
#ifdef	VERBOSE
  printf("Sending command: %s", command);
#endif

  // Send the command to the serial port
  return (static_cast<size_t>(vrpn_write_characters(serial_fd, (unsigned char *)(command), strlen(command))) == strlen(command));
}

// Convert the four bytes that have been read into a signed integer value.
// The format (no quotes) looks like: "- 37.1C\r" or " 000.00E\r".
// I don't think that the - means a minus sign, and it has a space
// between it and the number.
// Returns -1000 if there is an error.
float  vrpn_BiosciencesTools::convert_bytes_to_reading(const char *buf)
{
  float val;
  char  c;

  // Skip any leading minus sign.
  if (*buf == '-') { buf++; }

  // Read a fractional number.
  if (sscanf(buf, "%f%c", &val, &c) != 2) {
    return -1000;
  }

  // See if we get and E or C after the number,
  // or (since E can be part of a floating-point
  // number) if we get \r.
  if ( (c != 'E') && (c != 'C') && (c != '\r') ) {
    return -1000;
  }

  return val;
}


int	vrpn_BiosciencesTools::reset(void)
{
	//-----------------------------------------------------------------------
	// Sleep less thana second and then drain the input buffer to make sure we start
	// with a fresh slate.
	vrpn_SleepMsecs(200);
	vrpn_flush_input_buffer(serial_fd);

        //-----------------------------------------------------------------------
        // Set the temperatures for channel 1 and 2 and then set the temperature
        // control to be on or off depending on what we've been asked to do.
        if (!set_reference_temperature(0, static_cast<float>(o_channel[0]))) {
	  fprintf(stderr,"vrpn_BiosciencesTools::reset(): Cannot send set ref temp 0, trying again\n");
	  return -1;
        }
        if (!set_reference_temperature(1, static_cast<float>(o_channel[1]))) {
	  fprintf(stderr,"vrpn_BiosciencesTools::reset(): Cannot send set ref temp 1, trying again\n");
	  return -1;
        }
        if (!set_control_status(o_channel[0] != 0)) {
	  fprintf(stderr,"vrpn_BiosciencesTools::reset(): Cannot send set control status, trying again\n");
	  return -1;
        }

	//-----------------------------------------------------------------------
	// Send the command to request input from the first channel, and set up
        // the finite-state machine so we know which thing to request next.
        d_next_channel_to_read = 0;
	if (!request_temperature(d_next_channel_to_read)) {
	  fprintf(stderr,"vrpn_BiosciencesTools::reset(): Cannot request temperature, trying again\n");
	  return -1;
	}

	// We're now waiting for any responses from devices
	status = STATUS_SYNCING;
	VRPN_MSG_WARNING("reset complete (this is normal)");
	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}

//   This function will read characters until it has a full report, then
// put that report into analog fields and call the report methods on these.
//   The time stored is that of the first character received as part of the
// report.
   
int vrpn_BiosciencesTools::get_report(void)
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
      if (vrpn_read_available_characters(serial_fd, (unsigned char *)(d_buffer), 1) != 1) {
      	return 0;
      }

      // Got the first character of a report -- go into READING mode
      // and record that we got one character at this time.  Clear the
      // rest of the buffer to 0's so that we won't be looking at old
      // data when we parse.
      // The time stored here is as close as possible to when the
      // report was generated.
      d_bufcount = 1;
      vrpn_gettimeofday(&timestamp, NULL);
      status = STATUS_READING;
      size_t i;
      for (i = 1; i < sizeof(d_buffer); i++) {
        d_buffer[i] = 0;
      }
#ifdef	VERBOSE
      printf("... Got the 1st char\n");
#endif
   }

   //--------------------------------------------------------------------
   // Read as many bytes of this report as we can, storing them
   // in the buffer.
   //--------------------------------------------------------------------

   while ( 1 == (ret = vrpn_read_available_characters(serial_fd, (unsigned char *)(&d_buffer[d_bufcount]), 1))) {
     d_bufcount++;
   }
   if (ret == -1) {
	VRPN_MSG_ERROR("Error reading");
	status = STATUS_RESETTING;
	return 0;
   }
#ifdef	VERBOSE
   if (ret != 0) printf("... got %d total characters\n", d_bufcount);
#endif
   if (d_buffer[d_bufcount-1] != '\r') {	// Not done -- go back for more
	return 0;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check to make
   // sure that its format matches what we expect. If it does, the next
   // section will parse it.
   // Store the report into the appropriate analog channel.
   //--------------------------------------------------------------------

#ifdef	VERBOSE
   printf("  Complete report: \n%s\n",d_buffer);
#endif
   float value = convert_bytes_to_reading(d_buffer);
   if (value == -1000) {
     char msg[256];
     sprintf(msg,"Invalid report, channel %d, resetting", d_next_channel_to_read);
     VRPN_MSG_ERROR(msg);
     status = STATUS_RESETTING;
   }
   channel[d_next_channel_to_read] = value;

#ifdef	VERBOSE
   printf("got a complete report (%d chars)!\n", d_bufcount);
#endif

   //--------------------------------------------------------------------
   // Request a reading from the next channe.
   //--------------------------------------------------------------------

   d_next_channel_to_read = (d_next_channel_to_read + 1) % 6;
   if (!request_temperature(d_next_channel_to_read)) {
     char msg[256];
     sprintf(msg,"Can't request reading, channel %d, resetting", d_next_channel_to_read);
     VRPN_MSG_ERROR(msg);
     status = STATUS_RESETTING;
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
   status = STATUS_SYNCING;
   d_bufcount = 0;

   return 1;
}

bool vrpn_BiosciencesTools::set_specified_channel(unsigned channel, vrpn_float64 value)
{
    // XXX Check return status of the set commands?
    switch (channel) {
      case 0: // Reference temperature for channels 1 and 2
      case 1: // Reference temperature for channels 1 and 2
        set_reference_temperature(channel, static_cast<float>(value));
        o_channel[channel] = value;
        break;
      case 2: // Turn on temperature control if this is nonzero.
        o_channel[2] = value;
        buttons[0] = ( value != 0 );
        set_control_status( value != 0);
        break;
      default:
        return false;
    }
    return true;
}

int vrpn_BiosciencesTools::handle_request_message(void *userdata, vrpn_HANDLERPARAM p)
{
    const char	  *bufptr = p.buffer;
    vrpn_int32	  chan_num;
    vrpn_int32	  pad;
    vrpn_float64  value;
    vrpn_BiosciencesTools *me = (vrpn_BiosciencesTools *)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      char msg[1024];
      sprintf(msg,"vrpn_BiosciencesTools::handle_request_message(): Index out of bounds (%d of %d), value %lg\n",
	chan_num, me->num_channel, value);
      me->send_text_message(msg, me->timestamp, vrpn_TEXT_ERROR);
      return 0;
    }

    me->set_specified_channel(chan_num, value);
    return 0;
}

int vrpn_BiosciencesTools::handle_request_channels_message(void* userdata, vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_BiosciencesTools* me = (vrpn_BiosciencesTools *)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) {
      char msg[1024];
      sprintf(msg,"vrpn_BiosciencesTools::handle_request_channels_message(): Index out of bounds (%d of %d), clipping\n",
	num, me->o_num_channel);
      me->send_text_message(msg, me->timestamp, vrpn_TEXT_ERROR);
      num = me->o_num_channel;
    }
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
        me->set_specified_channel(i, me->o_channel[i]);
    }

    return 0;
}

/** When we get a connection request from a remote object, send our state so
    they will know it to start with. */
int vrpn_BiosciencesTools::handle_connect_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_BiosciencesTools *me = (vrpn_BiosciencesTools *)userdata;

    me->report(vrpn_CONNECTION_RELIABLE);
    return 0;
}

void	vrpn_BiosciencesTools::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Analog::report_changes(class_of_service);
        vrpn_Button::report_changes();
}

void	vrpn_BiosciencesTools::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Analog::report(class_of_service);
        vrpn_Button::report_changes();
}

/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds
*/

void	vrpn_BiosciencesTools::mainloop()
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
