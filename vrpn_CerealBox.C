// vrpn_CerealBox.C
//	This is a driver for the BG Systems CerealBox controller.
// This box is a serial-line device that allows the user to connect
// analog inputs, digital inputs, digital outputs, and digital
// encoders and read from them over RS-232. You can find out more
// at www.bgsystems.com. This code is written for version 3.07 of
// the EEPROM code.
//	This code is based on their driver code, which was posted
// on their web site as of the summer of 1999. This code reads the
// characters as they arrive, rather than waiting "long enough" for
// them to get here; this should allow the CerealBox to be used at
// the same time as a tracking device without slowing the tracker
// down.

#include <stdio.h>                      // for fprintf, stderr, perror, etc
#include <string.h>                     // for NULL, strlen, strncmp

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_CerealBox.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday

#undef VERBOSE

static const char	offset = 0x21;	// Offset added to some characters to avoid ctl chars
static const double	REV_PER_TICK = 1.0/4096;	// How many revolutions per encoder tick?

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the box
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

// This creates a vrpn_CerealBox and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box seems to autodetect the baud rate when the "T" command is sent
// to it.
vrpn_CerealBox::vrpn_CerealBox (const char * name, vrpn_Connection * c,
			const char * port, int baud,
			const int numbuttons, const int numchannels, const int numencoders):
		vrpn_Serial_Analog(name, c, port, baud),
		vrpn_Button_Filter(name, c),
		vrpn_Dial(name, c),
		_numbuttons(numbuttons),
		_numchannels(numchannels),
		_numencoders(numencoders)
{
	// Verify the validity of the parameters
	if (_numbuttons > 24) {
		fprintf(stderr,"vrpn_CerealBox: Can only support 24 buttons, not %d\n",
			_numbuttons);
		_numbuttons = 24;
	}
	if (_numchannels > 8) {
		fprintf(stderr,"vrpn_CerealBox: Can only support 8 analogs, not %d\n",
			_numchannels);
		_numchannels = 8;
	}
	if (_numencoders > 8) {
		fprintf(stderr,"vrpn_CerealBox: Can only support 8 encoders, not %d\n",
			_numencoders);
		_numencoders = 8;
	}

	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = _numbuttons;
	vrpn_Analog::num_channel = _numchannels;
	vrpn_Dial::num_dials = _numencoders;

        vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

	// Set the status of the buttons, analogs and encoders to 0 to start
	clear_values();
  _bufcount = 0;

	// Set the mode to reset
	_status = STATUS_RESETTING;
}

void	vrpn_CerealBox::clear_values(void)
{
	int	i;

	for (i = 0; i < _numbuttons; i++) {
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
	}
	for (i = 0; i < _numchannels; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
	for (i = 0; i < _numencoders; i++) {
		vrpn_Dial::dials[i] = 0.0;
	}
}

// This routine will reset the CerealBox, asking it to send the requested number
// of analogs, buttons and encoders. It verifies that it can communicate with the
// device and checks the version of the EPROMs in the device is 3.07.

int	vrpn_CerealBox::reset(void)
{
	struct	timeval	timeout;
	unsigned char	inbuf[45];
	const char	*Cpy = "Copyright (c), BG Systems";
	int	major, minor, bug;	// Version of the EEPROM
	unsigned char	reset_str[32];		// Reset string sent to box
	int	ret;

	//-----------------------------------------------------------------------
	// Set the values back to zero for all buttons, analogs and encoders
	clear_values();

	//-----------------------------------------------------------------------
	// Check that the box exists and has the correct EEPROM version. The
	// "T" command to the box should cause the 44-byte EEPROM string to be
	// returned. This string defines the version and type of the box.
	// Give it a reasonable amount of time to finish (2 seconds), then timeout
	vrpn_flush_input_buffer(serial_fd);
	vrpn_write_characters(serial_fd, (const unsigned char *)"T", 1);
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, 44, &timeout);
	inbuf[44] = 0;		// Make sure string is NULL-terminated
	if (ret < 0) {
		perror("vrpn_CerealBox: Error reading from box\n");
		return -1;
	}
	if (ret == 0) {
		fprintf(stderr,"vrpn_CerealBox: No response from box\n");
		return -1;
	}
	if (ret != 44) {
		fprintf(stderr,"vrpn_CerealBox: Got %d of 44 expected characters\n",ret);
		return -1;
	}

	// Parse the copyright string for the version and other information
	// Code here is similar to check_rev() function in the BG example code.
	if (strncmp((char *)inbuf, Cpy, strlen(Cpy))) {
		fprintf(stderr,"vrpn_CerealBox: Copyright string mismatch: %s\n",inbuf);
		return -1;
	}
	major = inbuf[38] - '0';
	minor = inbuf[40] - '0';
	bug = inbuf[41] - '0';
	if ( (3 != major) || (0 != minor) || (7 != bug) ) {
		fprintf(stderr, "vrpn_CerealBox: Bad EEPROM version (want 3.07, got %d.%d%d)\n",
			major, minor, bug);
		return -1;
	}
	printf("vrpn_CerealBox: Box of type %c found\n",inbuf[42]);

	//-----------------------------------------------------------------------
	// Compute the proper string to initialize the device based on how many
	// of each type of input/output that is selected. This follows init_cereal()
	// in BG example code.
	{	int	i;
		char	ana1_4 = 0;	// Bits 1-4 do analog channels 1-4
		char	ana5_8 = 0;	// Bits 1-4 do analog channels 5-8
		char	dig1_3 = 0;	// Bits 1-3 enable groups of 8 inputs
		char	enc1_4 = 0;	// Bits 1-4 enable encoders 1-4
		char	enc5_8 = 0;	// Bits 1-4 enable encoders 5-8

		// Figure out which analog channels to use and set them
		for (i = 0; i < 4; i++) {
			if (i < _numchannels) {
				ana1_4 |= (1<<i);
			}
		}
		for (i = 0; i < 4; i++) {
			if (i+4 < _numchannels) {
				ana5_8 |= (1<<i);
			}
		}

		// Figure out which banks of digital inputs to use and set them
		for (i = 0; i < _numbuttons; i++) {
			dig1_3 |= (1 << (i/8));
		}

		// Figure out which encoder channels to use and set them
		for (i = 0; i < 4; i++) {
			if (i < _numencoders) {
				enc1_4 |= (1<<i);
			}
		}
		for (i = 0; i < 4; i++) {
			if (i+4 < _numencoders) {
				enc5_8 |= (1<<i);
			}
		}

		reset_str[0] = 'c';
		reset_str[1] = (unsigned char)(ana1_4 + offset);	// Hope we don't need to set baud rate
		reset_str[2] = (unsigned char)((ana5_8 | (dig1_3 << 4)) + offset);
		reset_str[3] = (unsigned char)(0 + offset);
		reset_str[4] = (unsigned char)(0 + offset);
		reset_str[5] = (unsigned char)(enc1_4 + offset);
		reset_str[6] = (unsigned char)(enc1_4 + offset);	// Set encoders 1-4 for incremental
		reset_str[7] = (unsigned char)(enc5_8 + offset);
		reset_str[8] = (unsigned char)(enc5_8 + offset);	// Set encoders 5-8 for incremental
		reset_str[9] = '\n';
		reset_str[10] = 0;
	}

	// Send the string and then wait for an acknowledgement from the box.
	vrpn_write_characters(serial_fd, reset_str, 10);
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = vrpn_read_available_characters(serial_fd, inbuf, 2, &timeout);
	if (ret < 0) {
		perror("vrpn_CerealBox: Error reading ack from box\n");
		return -1;
	}
	if (ret == 0) {
		fprintf(stderr,"vrpn_CerealBox: No ack from box\n");
		return -1;
	}
	if (ret != 2) {
		fprintf(stderr,"vrpn_CerealBox: Got %d of 2 expected ack characters\n",ret);
		return -1;
	}
	if (inbuf[0] != 'a') {
		fprintf(stderr,"vrpn_CerealBox: Bad ack: wanted 'a', got '%c'\n",inbuf[0]);
		return -1;
	}

	//-----------------------------------------------------------------------
	// Ask the box to send a report, and go into SYNCING mode to get it.
	vrpn_write_characters(serial_fd, (const unsigned char *)"pE", 2);
	status = STATUS_SYNCING;
	printf("CerealBox reset complete.\n");

	//-----------------------------------------------------------------------
	// Figure out how many characters to expect in each report from the device
	// There is a 'p' to start and '\n' to finish, two bytes for each analog
	// value, 4 bytes for each encoder. Buttons are enabled in banks of 8,
	// but each bank of 8 is returned in 2 bytes (4 bits each).
	_expected_chars = 2 + 2*_numchannels + _numencoders*4 +
		((_numbuttons+7) / 8) * 2;

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
}

// This function will read characters until it has a full report, then
// put that report into the time, analog, button and encoder fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report. Reports start with
// the header "p" and end with "\n", and is of the length _expected_chars.
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize by waiting
// until the start-of-report character ('p') comes around again.
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the box hasn't simply
// stopped sending characters).
// Returns 1 if got a complete report, 0 otherwise.
   
int vrpn_CerealBox::get_report(void)
{
   int ret;		// Return value from function call to be checked
   int i;		// Loop counter
   int nextchar = 1;	// Index of the next character to read

   //--------------------------------------------------------------------
   // The reports are each _expected_chars characters long, and each start with an
   // ASCII 'p' character. If we're synching, read a byte at a time until we
   // find a 'p' character.
   //--------------------------------------------------------------------

   if (status == STATUS_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, _buffer, 1) != 1) {
      	return 0;
      }

      // If it is not a 'p', we don't want it but we
      // need to look at the next one, so just return and stay
      // in Syncing mode so that we will try again next time through.
      if ( _buffer[0] != 'p') {
      	fprintf(stderr,"vrpn_CerealBox: Syncing (looking for 'p', "
		"got '%c')\n", _buffer[0]);
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
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the device hasn't simply
   // stopped sending characters).
   //--------------------------------------------------------------------

   ret = vrpn_read_available_characters(serial_fd, &_buffer[_bufcount],
		_expected_chars-_bufcount);
   if (ret == -1) {
	send_text_message("vrpn_CerealBox: Error reading", timestamp, vrpn_TEXT_ERROR);
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
   // first character 'p', and the last character is '\n'.
   //--------------------------------------------------------------------

   if (_buffer[0] != 'p') {
	   status = STATUS_SYNCING;
      	   fprintf(stderr,"vrpn_CerealBox: Not 'p' in record\n");
	   return 0;
   }
   if (_buffer[_expected_chars-1] != '\n') {
	   status = STATUS_SYNCING;
      	   fprintf(stderr,"vrpn_CerealBox: No carriage return in record\n");
	   return 0;
   }

#ifdef	VERBOSE
   printf("got a complete report (%d of %d)!\n", _bufcount, _expected_chars);
#endif

   //--------------------------------------------------------------------
   // Ask the device to send us another report. Ideally, this would be
   // sent earlier so that we can overlap with the previous report. However,
   // when we ask after the first character we start losing parts of the
   // reports when we've turned on a lot of inputs. So, we do it here
   // after the report has come in.
   //--------------------------------------------------------------------

   vrpn_write_characters(serial_fd, (const unsigned char *)"pE", 2);

   //--------------------------------------------------------------------
   // Decode the report and store the values in it into the parent classes
   // (analog, button and encoder). This code is modelled on the routines
   // convert_serial() and unpack_encoders() in the BG systems code.
   //--------------------------------------------------------------------

   {	// Digital code. There appear to be 4 bits (four buttons) stored
	// in each byte, in the low-order 4 bits after the offset of 0x21
	// has been removed from each byte. They seem to come in highest-
	// buttons first, with the highest within each bank in the leftmost
	// bit.  This assumes we are not using MP for digital inputs.

	int i;
	int numbuttonchars;

	// Read two characters for each eight buttons that are on, from the
	// highest bank down.
	numbuttonchars = ((_numbuttons+7) / 8) * 2;
	for (i = numbuttonchars-1; i >= 0; i--) {
		// Find the four bits for these buttons by subtracting
		// the offset to get them into the low-order 4 bits
		char bits = (char)(_buffer[nextchar++] - offset);

		// Set the buttons for each bit
		buttons[ i*4 + 3 ] = ( (bits & 0x08) != 0);
		buttons[ i*4 + 2 ] = ( (bits & 0x04) != 0);
		buttons[ i*4 + 1 ] = ( (bits & 0x02) != 0);
		buttons[ i*4 + 0 ] = ( (bits & 0x01) != 0);
	}
   }

   {// Analog code. Looks like there are two characters for each
	// analog value; this conversion code grabbed right from the
	// BG code. They seem to come in lowest-numbered first.

	int	intval, i;
	double	realval;
	for (i = 0; i < _numchannels; i++) {
		intval =  (0x3f & (_buffer[nextchar++]-offset)) << 6;
		intval |= (0x3f & (_buffer[nextchar++]-offset));
		realval = -1.0 + (2.0 * intval/4095.0);
		channel[i] = realval;
	}
   }

   {	// Encoders. They come packed as 24-bit values with 6 bits in
	// each byte (offset by 0x21). They seem to come least-significant
	// part first. This decoding is valid only for incremental
	// encoders. Remember to convert the encoder values into fractions
	// of a revolution.

	for (i = 0; i < _numencoders; i++) {
		int	enc0, enc1, enc2, enc3;
		long	increment;

		enc0 = (_buffer[nextchar++]-offset) & 0x3f;
		increment = enc0;
		enc1 = (_buffer[nextchar++]-offset) & 0x3f;
		increment |= enc1 << 6;
		enc2 = (_buffer[nextchar++]-offset) & 0x3f;
		increment |= enc2 << 12;
		enc3 = (_buffer[nextchar++]-offset) & 0x3f;
		increment |= enc3 << 18;
		if ( increment & 0x800000 ) {
			dials[i] = (int)(increment - 16777216) * REV_PER_TICK;
		} else {
			dials[i] = (int)(increment) * REV_PER_TICK;
		}
	}
   }

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
   status = STATUS_SYNCING;
   _bufcount = 0;
   return 1;
}

void	vrpn_CerealBox::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;
	vrpn_Dial::timestamp = timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report_changes();
}

void	vrpn_CerealBox::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Button::timestamp = timestamp;
	vrpn_Dial::timestamp = timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
	vrpn_Dial::report();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the cerealbox,
// either trying to reset it or trying to get a reading from it.
void	vrpn_CerealBox::mainloop()
{
  // Call the generic server mainloop, since we are a server
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
		while (get_report()) {};    // Keep getting reports as long as they come
		struct timeval current_time;
		vrpn_gettimeofday(&current_time, NULL);
		if ( vrpn_TimevalDuration(current_time,timestamp) > MAX_TIME_INTERVAL) {
			fprintf(stderr,"CerealBox failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",
					current_time.tv_sec, static_cast<long>(current_time.tv_usec),
					timestamp.tv_sec, static_cast<long>(timestamp.tv_usec));
			send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
			status = STATUS_RESETTING;
		}
      }
      break;

    default:
	fprintf(stderr,"vrpn_CerealBox: Unknown mode (internal error)\n");
	break;
  }
}
