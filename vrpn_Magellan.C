// vrpn_Magellan.C
//	This is a driver for the LogiCad 3D Magellan controller.
// This is a 6DOF motion controller with 9 buttons and a 6DOF
// puck that can be translated and rotated.  It plugs into a serial
// line and communicated using RS-232 (this is a raw-mode driver).
// You can find out more at www.logicad3d.com; from there you can
// download the programming guide, which was used to generate this
// driver. This is a driver for the standard Magellan/Space Mouse
// (the Turbo Magellan/Space Mouse uses 19200 baud communications
// and a compressed data format -- it would only require determining
// which mode and implementing a new data translation part to make
// the driver compatible with the Turbo version; we don't have one
// here to check).  The driver was written and tested on a Magellan
// version 5.49.

// INFO about how the device communicates:
//	It sends 4-bit nybbles packed into bytes such that there is
// even parity: in particular, the following values are sent:
//   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
//  30H 41H 42H 33H 44H 35H 36H 47H 48H 39H 3AH 4BH 3CH 4DH 4EH 3FH
//  '0' 'A' 'B' '3' 'D' '5' '6' 'G' 'H' '9' ':' 'K' '<' 'M' 'N' '?'
//	 Commands are sent starting with a character, then followed
// by data, then always terminated by a carriage return '\r'. Useful
// commands to send to the device include z\r (zero), vQ\r (query
// version), b<\r (beep for half a second), 
//	Julien Brisset found a version of the Magellan that did not
// understand the reset command that works on our version, so sent an
// alternate reset string that works on his.  This is sent if the
// 'altreset' parameter is true in the constructor.

#include <stdio.h>                      // for fprintf, stderr
#include <string.h>                     // for strlen, NULL, strcmp

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Magellan.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc

#undef VERBOSE

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

// This creates a vrpn_Magellan and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box seems to autodetect the baud rate when the "T" command is sent
// to it.
vrpn_Magellan::vrpn_Magellan (const char * name, vrpn_Connection * c,
			const char * port, int baud, bool altreset):
		vrpn_Serial_Analog(name, c, port, baud),
		vrpn_Button_Filter(name, c),
		_altreset(altreset),
		_numbuttons(9),
		_numchannels(6),
		_null_radius(8)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = _numbuttons;
	vrpn_Analog::num_channel = _numchannels;

        vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

	// Set the status of the buttons and analogs to 0 to start
	clear_values();
  _bufcount = 0;

	// Set the mode to reset
	_status = STATUS_RESETTING;

	// Wait before the first time we attempt a reset - seems to be a race condition
	// with the device needing time between opening of the serial connection and
	// receiving the reset commands. (SpaceMouse Plus XT Serial, version 6.60)
	vrpn_SleepMsecs(1000);
}

void	vrpn_Magellan::clear_values(void)
{
	int	i;

	for (i = 0; i < _numbuttons; i++) {
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
	}
	for (i = 0; i < _numchannels; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
}

// This routine will reset the Magellan, zeroing the position, setting the compress
// mode and making the device beep for half a second.  It then verifies that the
// commands were received and processed by the device.
//   Commands	   Responses		Meanings
//	z\r	    z\r		    Set current position and orientation to be zero
//	m3\r	    m3\r	    Set 3D mode, send trans+rot
//	c30\r	    c30\r	    Sets to not compress the return data
//	nH\r	    nH\r	    Sets the NULL radius for the device to 8
//	bH\r	    b\r		    Beep (< means for 32 milliseconds)

int	vrpn_Magellan::reset(void)
{
	struct	timeval	timeout, now;
	unsigned char	inbuf[45];
	const	char	*reset_str = "z\rm3\rc30\rnH\rbH\r";	// Reset string sent to box
	const	char	*expect_back = "z\rm3\rc30\rnH\rb\r";	// What we expect back
	int	ret;

	//-----------------------------------------------------------------------
	// See if we should be using the alternative reset string.
	// XXX The "expect_back" string here is almost certainly wrong.  Waiting
	// to hear back what the correct one should be.
	if (_altreset) {
	  reset_str = "z\rm3\rnH\rp?0\rq00\r";
	  expect_back = "z\rm3\rnH\rp?0\rq00\r";
	}

	//-----------------------------------------------------------------------
	// Set the values back to zero for all buttons, analogs and encoders
	clear_values();

	//-----------------------------------------------------------------------
	// Send the list of commands to the device to cause it to reset and beep.
	// Read back the response and make sure it matches what we expect.
	// Give it a reasonable amount of time to finish, then timeout
	vrpn_flush_input_buffer(serial_fd);
	vrpn_write_slowly(serial_fd, (const unsigned char *)reset_str, strlen(reset_str), 5);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, strlen(expect_back), &timeout);
	inbuf[strlen(expect_back)] = 0;		// Make sure string is NULL-terminated

	vrpn_gettimeofday(&now, NULL);
	if (ret < 0) {
		send_text_message("vrpn_Magellan reset: Error reading from device", now);
		return -1;
	}
	if (ret == 0) {
		send_text_message("vrpn_Magellan reset: No response from device", now);
		return -1;
	}
	if (ret != (int)strlen(expect_back)) {
            send_text_message("vrpn_Magellan reset: Got less than expected number of characters", now);
                             //,ret,    strlen(expect_back));
		return -1;
	}

	// Make sure the string we got back is what we expected
	if ( strcmp((char *)inbuf, expect_back) != 0 ) {
		send_text_message("vrpn_Magellan reset: Bad reset string", now);
                //(want %s, got %s)\n",	expect_back, inbuf);
		return -1;
	}

	// The NULL radius is now set to 8
	_null_radius = 8;

	// We're now waiting for a response from the box
	status = STATUS_SYNCING;

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}

//   This function will read characters until it has a full report, then
// put that report into the time, analog, or button fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report.
//   Reports start with different characters, and the length of the report
// depends on what the first character of the report is.  We switch based
// on the first character of the report to see how many more to expect and
// to see how to handle the report.
//   Returns 1 if there is a complete report found, 0 otherwise.  This is
// so that the calling routine can know to check again at the end of complete
// reports to see if there is more than one report buffered up.
   
int vrpn_Magellan::get_report(void)
{
   int ret;		// Return value from function call to be checked
   int i;		// Loop counter
   int nextchar = 1;	// Index of the next character to read

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
	  case 'k':
	      _expected_chars = 5; status = STATUS_READING; break;
	  case 'b':
	      _expected_chars = 2; status = STATUS_READING; break;
	  case 'm':
	      _expected_chars = 3; status = STATUS_READING; break;
	  case 'd':
	      _expected_chars = 26; status = STATUS_READING; break;
	  case 'n':
	      _expected_chars = 3; status = STATUS_READING; break;
	  case 'q':
	      _expected_chars = 4; status = STATUS_READING; break;
	  case 'z':
	      _expected_chars = 2; status = STATUS_READING; break;
	  case 'p':
	      _expected_chars = 4; status = STATUS_READING; break;
	  case 'c':
	      _expected_chars = 4; status = STATUS_READING; break;

	  default:
	      fprintf(stderr,"vrpn_Magellan: Unknown command (%c), resetting\n", _buffer[0]);
	      status = STATUS_RESETTING;
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
	send_text_message("vrpn_Magellan: Error reading", timestamp, vrpn_TEXT_ERROR);
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
   // last character '\r'
   //--------------------------------------------------------------------

   if (_buffer[_expected_chars-1] != '\r') {
	   status = STATUS_SYNCING;
      	   send_text_message("vrpn_Magellan: No carriage return in record", timestamp, vrpn_TEXT_ERROR);
	   return 0;
   }

#ifdef	VERBOSE
   printf("got a complete report (%d of %d)!\n", _bufcount, _expected_chars);
#endif

   //--------------------------------------------------------------------
   // Decode the report and store the values in it into the parent classes
   // (analog or button) if appropriate.
   //--------------------------------------------------------------------

   switch ( _buffer[0] ) {
     case 'k':
	 // This is a button command from the device. It gives us the state
	 // of each of the buttons on the device.  Buttons 1-4 are encoded
	 // in the 4 LSBs of the first byte (key 1 in the LSB); buttons 5-8
	 // are in the 4 LSBs of the second byte; the * button (which we'll
	 // call button 0) is in the LSB of the third byte (the other 3 bits
	 // don't seem to be used).
	 buttons[0] = ( (_buffer[3] & 0x01) != 0);
	 buttons[1] = ( (_buffer[1] & 0x01) != 0);
	 buttons[2] = ( (_buffer[1] & 0x02) != 0);
	 buttons[3] = ( (_buffer[1] & 0x04) != 0);
	 buttons[4] = ( (_buffer[1] & 0x08) != 0);
	 buttons[5] = ( (_buffer[2] & 0x01) != 0);
	 buttons[6] = ( (_buffer[2] & 0x02) != 0);
	 buttons[7] = ( (_buffer[2] & 0x04) != 0);
	 buttons[8] = ( (_buffer[2] & 0x08) != 0);
	 break;

     case 'b':
	 // Beep command received. We don't care.
	 break;

     case 'm':
	 // Mode set command. We really only care that it is still in
	 // 3D mode (as opposed to Mouse mode); the other fields tell
	 // whether it is in dominant axis mode, and whether translations
	 // and rotations are being sent. We can handle any of these without
	 // incident.
	 if ( (_buffer[1] & 0x08) != 0) {
	     send_text_message("vrpn_Magellan: Was put into mouse mode, resetting", timestamp, vrpn_TEXT_ERROR);
	     status = STATUS_RESETTING;
	     return 1;
	 }
	 break;

     case 'd':
	 // Axis data is being returned (telling what the X,Y,Z and A,B,C axes
	 // are currently set to). This data is put into the range [-1,1] and
	 // put into the analog channels (0=X, 1=Y, 2=Z, 3=A, 4=B, 5=C).  It comes
	 // from the device with each axis packed into the lower nybble of 4
	 // consecutive bytes; the translation back to a signed 16-bit integer
	 // is done with (N0 * 4096) + (N1 * 256) + (N2 * 16) + (N3) - 32768
	 // for each value; this is then scaled to [-1,1].
	 nextchar = 1;	// Skip the zeroeth character (the command)
	 for (i = 0; i < _numchannels; i++) {
	    long intval;
	    intval = (0x0f & _buffer[nextchar++]) << 12;
	    intval += (0x0f & _buffer[nextchar++]) << 8;
	    intval += (0x0f & _buffer[nextchar++]) << 4;
    	    intval += (0x0f & _buffer[nextchar++]);
	    intval -= 32768;

	    // If the absolute value of the integer is <= the NULL radius, it should
	    // be set to zero.
	    if ( (intval <= _null_radius) && (intval >= - _null_radius) ) {
		intval = 0;
	    }

	    // The largest values that seem to come out of the Magellan I've got
	    // even under the maximum acceleration are absolute value 7200 or so.
	    // We'll divide by 7500 to keep it safe.
	    double realval = intval / 7500.0;
	    channel[i] = realval;
	 }
	 break;
	 
     case 'n':
	 // NULL radius set. This is the number of ticks around zero that should
	 // count as zero, to allow a "dead zone" for the user near the center.
	 // We store this for the analog parsing code.  The low nybble in the data
	 // word holds the new value
	 _null_radius = 0x0f & _buffer[1];
	 break;

     case 'q':
	 // Sensitivity set. We don't care.
	 break;

     case 'z':
	 // The device was zeroed. We don't care.
	 break;

     case 'p':
	 // The min/max periods were set.  We don't care.
	 break;

     case 'c':
	 // Some extended command was sent.  I hope we don't care.
	 // XXX Should check to make sure compression is not on.
	 break;

     default:
	fprintf(stderr,"vrpn_Magellan: Unknown [internal] command (%c), resetting\n", _buffer[0]);
	status = STATUS_RESETTING;
	return 1;
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
   status = STATUS_SYNCING;
   _bufcount = 0;

   return 1;	// We got a full report.
}

void	vrpn_Magellan::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

void	vrpn_Magellan::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the Magellan,
// either trying to reset it or trying to get a reading from it.
void	vrpn_Magellan::mainloop()
{
  server_mainloop();

  switch(status) {
    case STATUS_RESETTING:
	reset();
	break;

    case STATUS_SYNCING:
    case STATUS_READING:
	// Keep getting reports until all full reports are read.
	while (get_report()) {};
        break;

    default:
	fprintf(stderr,"vrpn_Magellan: Unknown mode (internal error)\n");
	break;
  }
}
