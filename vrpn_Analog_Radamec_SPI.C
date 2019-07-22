// vrpn_Radamec_SPI.C
//	This is a driver for the Radamec Serial Position Interface
// "virtual set" camera tracking rig.  It plugs into a serial
// line and communicates using RS-232 (this is a raw-mode driver).
// You can find out more at www.radamec.com.  They have a manual with
// a section on the serial interface; this code is based on that and
// communications with the vendor and experimentation. It was written in
// August 2000 by Russ Taylor.

// INFO about how the device communicates:
//	Reports coming from the device have a 1-byte header that tells
// which of 7 possible commands is being sent (0xA0-0xA6). Each then has
// a 1-byte Camera ID, then other parameters specific to the type, then
// a CRC checksum.
//	Most of the report types can be sent to the instrument to set
// various parameters.  Report type 4 is mainly a command/query message,
// which the device echoes.


#include <stdio.h>                      // for sprintf
#include <string.h>                     // for NULL, memcpy

#include "vrpn_Analog_Radamec_SPI.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_unbuffer, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

// This creates a vrpn_Radamec_SPI and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.

vrpn_Radamec_SPI::vrpn_Radamec_SPI (const char * name, vrpn_Connection * c,
			const char * port, int baud):
		vrpn_Serial_Analog(name, c, port, baud, 8, vrpn_SER_PARITY_ODD),
		_camera_id(-1),		// Queried from the controller during reset
		_numchannels(4)	// This is an estimate; will change when reports come
{
	// Set the parameters in the parent classes
	vrpn_Analog::num_channel = _numchannels;

        vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

	// Set the status of the buttons and analogs to 0 to start
	clear_values();
  _bufcount = 0;

	// Set the mode to reset
	_status = STATUS_RESETTING;
}

void	vrpn_Radamec_SPI::clear_values(void)
{
	int	i;

	for (i = 0; i < _numchannels; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
}


/** This routine will compute the CRC for the message that starts at head
    and is of length len.  This needs to be added to each message sent to the
    device, and should be checked for each command received by the device.

    The manual states that the CRC is "calculated by subtracting the total sum
    of each byte of the block from 40H."
*/

unsigned char vrpn_Radamec_SPI::compute_crc(const unsigned char *head, int len)
{
    int		    i;
    unsigned char   sum;

    // Sum up the bytes, allowing them to overflow the unsigned char
    sum = 0;
    for (i = 0; i < len; i++) {
	sum = (unsigned char)( sum + head[i] );
    }

    // Unsigned subtraction from 40H, again allowing the subtraction to overflow
    return (unsigned char)(0x40 - sum);
}

/** Compute the CRC for the command that is being sent, append that to the
    message, and send the message.

    Returns 0 on success, -1 on failure.
*/

int vrpn_Radamec_SPI::send_command(const unsigned char *cmd, int len)
{
    int		ret;
    unsigned	char	*outbuf;
    try { outbuf = new unsigned char[len + 1]; } // Leave room for the CRC
    catch (...) { return -1; }

    // Put the command into the output buffer
    memcpy(outbuf, cmd, len);

    // Add the CRC
    outbuf[len] = compute_crc(cmd, len);

    // Send the command
    ret = vrpn_write_characters(serial_fd, outbuf, len+1);

    // Tell if this all worked.
    if (ret == len+1) {
	return 0;
    } else {
	return -1;
    }
}


/** Convert a 24-bit value from a buffer into an unsigned integer value.
    The value has the most significant byte first in the buffer.
*/

vrpn_uint32 vrpn_Radamec_SPI::convert_24bit_unsigned(const unsigned char *buf)
{
    vrpn_uint32	retval;
    unsigned char bigend_buf[4];
    const unsigned char *bufptr = bigend_buf;

    // Store the three values into three bytes of a big-endian 32-bit integer
    bigend_buf[0] = 0;
    bigend_buf[1] = buf[0];
    bigend_buf[2] = buf[1];
    bigend_buf[3] = buf[2];

    // Convert the value to an integer
    vrpn_unbuffer((const char **)&bufptr, &retval);
    return retval;
}


/** Convert a 16-bit unsigned value from a buffer into an integer value.
    The value has the most significant byte first in the buffer.
*/

vrpn_int32 vrpn_Radamec_SPI::convert_16bit_unsigned(const unsigned char *buf)
{
    vrpn_int32	retval;
    unsigned char bigend_buf[4];
    const unsigned char *bufptr = bigend_buf;

    // Store the three values into two bytes of a big-endian 32-bit integer
    bigend_buf[0] = 0;
    bigend_buf[1] = 0;
    bigend_buf[2] = buf[0];
    bigend_buf[3] = buf[1];

    // Convert the value to an integer
    vrpn_unbuffer((const char **)&bufptr, &retval);
    return retval;
}

/** ------------------- Conversion of encoder indices to values ----------------
    Pan and tilt axis have an encoder index pulse which resets the value to
    7FFFF hex each time the head passes through 0 degrees, the centre of range
    of the head.  This is indicated by two red dots on the pan axis, which when
    aligned show the head is at 0 degrees.  Tilt 0 position is when the camera
    is horizontal.  There are 900 encoder counts per degree and the direction
    corresponding to an increase in count depends on the mechanical orientation
    of the head and camera but is configurable using the configuration block
    anyway.

    Zoom and focus return an encoder count corresponding to the barrel
    rotation of the lens.  You will have to calibrate this to give a value for
    field of view for the virtual set.  This is not an easy task as lens
    characteristics are non-linear.  All virtual studio system suppliers have
    varying methods of doing lens calibration, including our own proprietary
    method used for Radamec Virtual Scenario.
*/

double	vrpn_Radamec_SPI::int_to_pan(vrpn_uint32 val)
{
    return (((int)val) - 0x7ffff) / 900.0;
}

double	vrpn_Radamec_SPI::int_to_zoom(vrpn_uint32 val)
{
    //XXX Unknown conversion, return the raw value
    return val;
}

double	vrpn_Radamec_SPI::int_to_focus(vrpn_uint32 val)
{
    //XXX Unknown conversion, return the raw value
    return val;
}

double	vrpn_Radamec_SPI::int_to_height(vrpn_uint32 val)
{
    //XXX Unknown conversion, send the integer along unchanged
    return val;
}

/** Convert from the millimeter and fraction-of-millimeter
    values returned by the device into meters.
*/
double	vrpn_Radamec_SPI::int_to_X(vrpn_uint32 mm, vrpn_uint32 frac)
{
    return 0.001 * (mm + frac/65536.0);
}

/** Convert from the 1/100 degree increments into degrees. */

double	vrpn_Radamec_SPI::int_to_orientation(vrpn_uint32 val)
{
    return 0.01 * val;
}


// This routine will reset the Radamec_SPI, requesting the camera number from the
// device and then turning on stream mode. XXX The device never seems to return a
// response to these queries, but always seems to go ahead and spew reports, even
// when not genlocked.  If you find a device that doesn't send reports, this may
// be a good place to look for the problem.
//   Commands		Responses	    Meanings
//   <A4><FF><02><CRC>  <A4><##><02><CRC>   Request camera number; returns camera number in ##
//   <A4><##><01><CRC>  <A4><##><01><CRC>   Start stream mode on the camera we were told we are 

int	vrpn_Radamec_SPI::reset(void)
{
	unsigned char	command[128];
/* XXX commented out, since the response doesn't come, but the device still works.
	struct	timeval	timeout;
	unsigned char	inbuf[128];
	int	ret;
	char	errmsg[256];
*/
	//-----------------------------------------------------------------------
	// Send the command to request the camera ID and then read the response.
	// Give it a reasonable amount of time to finish (2 seconds), then timeout

	vrpn_flush_input_buffer(serial_fd);
	sprintf((char *)command, "%c%c%c", 0xa4, 0xff, 0x02);
	send_command((unsigned char *)command, 3);
/* XXX commented out, since the response doesn't come, but the device still works.
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, 4, &timeout);
	inbuf[4] = 0;		// Make sure string is NULL-terminated
	if (ret < 0) {
		perror("vrpn_Radamec_SPI reset: Error reading camera ID from device\n");
		return -1;
	}
	if (ret == 0) {
		VRPN_MSG_ERROR("reset: No response to camera ID from device");
		return -1;
	}
	if (ret != 4) {
		sprintf(errmsg,"reset: Got %d of %d expected characters for camera ID\n",ret, 4);
		VRPN_MSG_ERROR(errmsg);
		return -1;
	}

	// Make sure the string we got back is what we expected and then find out the camera ID
	if ( (inbuf[0] != 0xa4) || (inbuf[2] != 0x02) || (inbuf[3] != compute_crc(inbuf,3)) ) {
	    VRPN_MSG_ERROR("reset: Bad response to camera # request");
	    return -1;
	}
	_camera_id = inbuf[1];
*/
	//-----------------------------------------------------------------------
	// Send the command to put the camera into stream mode and then read back
	// to make sure we got a response.

	sprintf((char *)command, "%c%c%c", 0xa4, _camera_id, 0x01);
	send_command(command, 3);
/* XXX commented out, since the response doesn't come, but the device still works.
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, 4, &timeout);
	inbuf[4] = 0;		// Make sure string is NULL-terminated
	if (ret < 0) {
		VRPN_MSG_ERROR("reset: Error reading from device");
		return -1;
	}
	if (ret == 0) {
		VRPN_MSG_ERROR("reset: No response from device");
		return -1;
	}
	if (ret != 4) {
		sprintf(errmsg,"vrpn_Radamec_SPI reset: Got %d of %d expected characters\n",ret, 4);
		VRPN_MSG_ERROR(errmsg);
		return -1;
	}

	// Make sure the string we got back is what we expected
	if ( (inbuf[0] != 0xa4) || (inbuf[1] != _camera_id) || (inbuf[2] != 0x01) ||
		(inbuf[3] != compute_crc(inbuf,3)) ) {
	    VRPN_MSG_ERROR("reset: Bad response to start stream mode command");
	    return -1;
	}
*/
	// We're now waiting for a response from the box
	status = STATUS_SYNCING;

	VRPN_MSG_WARNING("reset complete (this is good)");

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}

//   This function will read characters until it has a full report, then
// put that report into analog fields and call the report methods on these.
//   The time stored is that of the first character received as part of the
// report.
//   Reports start with different characters, and the length of the report
// depends on what the first character of the report is.  We switch based
// on the first character of the report to see how many more to expect and
// to see how to handle the report.
   
int vrpn_Radamec_SPI::get_report(void)
{
   int ret;		// Return value from function call to be checked
   char errmsg[256];

   //--------------------------------------------------------------------
   // If we're SYNCing, then the next character we get should be the start
   // of a report.  If we recognize it, go into READing mode and tell how
   // many characters we expect total. If we don't recognize it, then we
   // must have misinterpreted a command or something; reset the Magellan
   // and start over
   //--------------------------------------------------------------------

   if (status == STATUS_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, _buffer, 1) != 1) {
      	return 0;
      }

      switch (_buffer[0]) {
	  case 0xa0:
	      _expected_chars = 15; status = STATUS_READING; break;
	  case 0xa1:
	      _expected_chars = 18; status = STATUS_READING; break;
	  case 0xa2:
	      _expected_chars = 30; status = STATUS_READING; break;
	  case 0xa3:
	      _expected_chars = 18; status = STATUS_READING; break;
	  case 0xa4:
	      _expected_chars = 4; status = STATUS_READING; break;
	  case 0xa5:
	      _expected_chars = 5; status = STATUS_READING; break;
	  case 0xa6:
	      _expected_chars = 26; status = STATUS_READING; break;

	  default:
	      // Not a recognized command, keep looking
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
#ifdef	VERBOSE
      printf("... Got the 1st char\n");
#endif
   }

   //--------------------------------------------------------------------
   // Read as many bytes of this report as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.
   //--------------------------------------------------------------------

   ret = vrpn_read_available_characters(serial_fd, &_buffer[_bufcount],
		_expected_chars-_bufcount);
   if (ret == -1) {
	VRPN_MSG_ERROR("Error reading");
	status = STATUS_RESETTING;
	return 0;
   }
   _bufcount += ret;
#ifdef	VERBOSE
   if (ret != 0) printf("... got %d characters (%d total)\n",ret, _bufcount);
#endif
   if (_bufcount < _expected_chars) {	// Not done -- go back for more
	return 0;
   }

   //--------------------------------------------------------------------
   // We now have enough characters to make a full report. Check to make
   // sure that its format matches what we expect. If it does, the next
   // section will parse it. If it does not, we need to go back into
   // synch mode and ignore this report. A well-formed report has the
   // correct CRC as its last character, and has the camera ID matching
   // what we expect as its second character.
   //--------------------------------------------------------------------

   if (_buffer[_expected_chars-1] != compute_crc(_buffer, _expected_chars-1) ) {
	   status = STATUS_SYNCING;
      	   VRPN_MSG_WARNING("Bad CRC in report (ignoring this report)");
	   return 0;
   }
   _camera_id = _buffer[1];

#ifdef	VERBOSE
   printf("got a complete report (%d of %d)!\n", _bufcount, _expected_chars);
#endif

   //--------------------------------------------------------------------
   // Decode the report and store the values in it into the analog values
   // if appropriate.
   //--------------------------------------------------------------------

   switch ( _buffer[0] ) {
     case 0xa0:	// Pan, Tilt, Zoom, Focus
	 _numchannels = 4;
	 channel[0] = int_to_pan(convert_24bit_unsigned(&_buffer[2]));
	 channel[1] = int_to_tilt(convert_24bit_unsigned(&_buffer[5]));
	 channel[2] = int_to_zoom(convert_24bit_unsigned(&_buffer[8]));
	 channel[3] = int_to_focus(convert_24bit_unsigned(&_buffer[11]));
	 break;

     case 0xa1:	// Pan, Tilt, Zoom, Focus, Height
	 _numchannels = 5;
	 channel[0] = int_to_pan(convert_24bit_unsigned(&_buffer[2]));
	 channel[1] = int_to_tilt(convert_24bit_unsigned(&_buffer[5]));
	 channel[2] = int_to_zoom(convert_24bit_unsigned(&_buffer[8]));
	 channel[3] = int_to_focus(convert_24bit_unsigned(&_buffer[11]));
	 channel[4] = int_to_height(convert_24bit_unsigned(&_buffer[14]));
	 break;

     case 0xa2:	// Pan, Tilt, Zoom, Focus, Height, X, Y, Orientation
	 _numchannels = 8;
	 channel[0] = int_to_pan(convert_24bit_unsigned(&_buffer[2]));
	 channel[1] = int_to_tilt(convert_24bit_unsigned(&_buffer[5]));
	 channel[2] = int_to_zoom(convert_24bit_unsigned(&_buffer[8]));
	 channel[3] = int_to_focus(convert_24bit_unsigned(&_buffer[11]));
	 channel[4] = int_to_height(convert_24bit_unsigned(&_buffer[14]));
	 // Note that fraction is first, and is unsigned, in the buffer
	 channel[5] = int_to_X(convert_16bit_unsigned(&_buffer[19]),
			convert_16bit_unsigned(&_buffer[17]));
	 // Note that fraction is first, and is unsigned, in the buffer
	 channel[6] = int_to_X(convert_16bit_unsigned(&_buffer[23]),
			convert_16bit_unsigned(&_buffer[21]));
	 channel[7] = int_to_orientation(convert_16bit_unsigned(&_buffer[25]));
	 break;

     case 0xa4: // Response to our reset commands -- ignore them
	 break;

    // Note that case 0xa3 should never happen, since we don't send this.
    // We'll let the "default" case complain about getting it.

    // Note that 0xa5 should not happen, since we don't send it.
    // We'll let the "default" case complain about getting it.


     case 0xa6:	// Pan, Tilt, Zoom, Focus, Height, X, Y, Orientation
		// The same as A2, except that fractional X,Y not included.
	 _numchannels = 8;
	 channel[0] = int_to_pan(convert_24bit_unsigned(&_buffer[2]));
	 channel[1] = int_to_tilt(convert_24bit_unsigned(&_buffer[5]));
	 channel[2] = int_to_zoom(convert_24bit_unsigned(&_buffer[8]));
	 channel[3] = int_to_focus(convert_24bit_unsigned(&_buffer[11]));
	 channel[4] = int_to_height(convert_24bit_unsigned(&_buffer[14]));
	 // Note that fraction is not in the buffer
	 channel[5] = int_to_X(convert_16bit_unsigned(&_buffer[17]), 0);
	 // Note that fraction is not in the buffer
	 channel[6] = int_to_X(convert_16bit_unsigned(&_buffer[19]), 0);
	 channel[7] = int_to_orientation(convert_16bit_unsigned(&_buffer[21]));
	 break;

     default:
	sprintf(errmsg,"vrpn_Radamec_SPI: Unhandled command (0x%02x), resetting\n", _buffer[0]);
	VRPN_MSG_ERROR(errmsg);
	status = STATUS_RESETTING;
	return 0;
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report();	// Report, rather than report_changes(), since it is an absolute device
   status = STATUS_SYNCING;
   _bufcount = 0;

   return 1;
}

void	vrpn_Radamec_SPI::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
}

void	vrpn_Radamec_SPI::report(vrpn_uint32 class_of_service)
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

void	vrpn_Radamec_SPI::mainloop()
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
	    if ( vrpn_TimevalDuration(current_time,timestamp) > MAX_TIME_INTERVAL) {
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
