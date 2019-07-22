//	This is a driver for the 5dt Data Glove
// Look at www.5dt.com for more informations about this product
// Manuals are avalaible freely from this site
// This code was written by Philippe DAVID with the help of Yves GAUVIN
// naming convention used int this file:
//   l_ is the prefixe for local variables
//   g_ is the prefixe for global variables
//   p_ is the prefixe for parameters

#include <stdio.h>                      // for sprintf, printf

#include "vrpn_Analog_5dt.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)


/******************************************************************************
 * NAME      : vrpn_5DT::vrpn_5DT
 * ROLE      : This creates a vrpn_5DT and sets it to reset mode. It opens
 *             the serial device using the code in the vrpn_Serial_Analog constructor.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_5dt::vrpn_5dt (const char * p_name, vrpn_Connection * p_c, const char * p_port, int p_baud, int p_mode, bool tenbytes):
  vrpn_Serial_Analog (p_name, p_c, p_port, p_baud, 8, vrpn_SER_PARITY_NONE),
  _announced(false),	// Not yet announced our warning.
  _wireless (p_baud == 9600), // 9600 baud implies a wireless glove.
  _gotInfo (false),  // used to know if we have gotten a first full wireless report.
  _numchannels (8),	// This is an estimate; will change when reports come
  _tenbytes (tenbytes)	// Do we expect ten-byte messages?
{
  if (_wireless) {
    // All wireless gloves continually send 10 byte reports and ignore
    // all requests.
    _tenbytes = true;
    p_mode = 2;
  }
  // Set the parameters in the parent classes
  vrpn_Analog::num_channel = _numchannels;

  // Set the status of the buttons and analogs to 0 to start
  clear_values();
  vrpn_gettimeofday(&timestamp, NULL);

  // Set the mode to reset
  _status = STATUS_RESETTING;

  //Init the mode
  _mode =  p_mode;

  // Sebastien Kuntz reports the following about the driver they have
  // written:  It works great except for one small detail :
  // for unknown reasons, there's an extra byte hanging around in the data
  // stream sent by the 5DT glove in stream mode.
  // We have the header, finger infos, pitch and roll, checksum and after
  // that we receive a 0x55 Byte.
  // The official doc (http://www.5dt.com/downloads/5DTDataGlove5Manual.pdf)
  // says we should get only 9 bytes, so I don't know if it's a response
  // from a command somewhere; but the fact is we receive 10 bytes.
  if (_tenbytes) {
    _expected_chars = 10;
  } else {
    _expected_chars = 9;
  }
}


/******************************************************************************
 * NAME      : vrpn_5dt::clear_values
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
void
vrpn_5dt::clear_values (void)
{
  int i;

  for (i = 0; i < _numchannels; i++)
    {
      vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
}


/******************************************************************************
 * NAME      : vrpn_5dt::
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 *             int   len : Length of the command to be sent
 * RETURN    : 0 on success, -1 on failure.
 ******************************************************************************/
int
vrpn_5dt::send_command (const unsigned char *p_cmd, int p_len)
{
  int            l_ret;

  // Send the command
  l_ret = vrpn_write_characters (serial_fd, p_cmd, p_len);
  // Tell if this all worked.
  if (l_ret == p_len) {
    return 0;
  } else {
    return -1;
  }
}


/******************************************************************************
 * NAME      : vrpn_5dt::reset
 * ROLE      : This routine will reset the 5DT
 * ARGUMENTS : 
 * RETURN    : 0 else -1 in case of error
 ******************************************************************************/
int
vrpn_5dt::reset (void)
{
  struct timeval l_timeout;
  unsigned char	l_inbuf [45];
  int            l_ret;
  char           l_errmsg[256];

  if (_wireless) {
    // Wireless gloves can't be reset, but we do need to wait for a header byte.
    // Then, syncing will wait to see if we get a capability byte as expected
    // at the end of a report.
    _gotInfo = false;
    vrpn_SleepMsecs (100);  //Give it time to respond

    // Will wait at most 2 seconds
    l_timeout.tv_sec = 2;
    l_timeout.tv_usec = 0;
    l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 1, &l_timeout);
    if (l_ret != 1) {
      VRPN_MSG_ERROR ("vrpn_5dt: Unable to read from the glove\n");
      return -1;
    }
    if (l_inbuf[0] == 0x80) {
      _status = STATUS_SYNCING;
      _buffer[0] = l_inbuf[0];
      _bufcount = 1;
      vrpn_gettimeofday (&timestamp, NULL);	// Set watchdog now
      VRPN_MSG_INFO ("vrpn_5dt: Got a possible header byte!");
      return 0;
    }
    return 0;
  }
  vrpn_flush_input_buffer (serial_fd);
  send_command ((const unsigned char *) "A", 1); // Command to init the glove
  vrpn_SleepMsecs (100);  //Give it time to respond
  l_timeout.tv_sec = 2;
  l_timeout.tv_usec = 0;
  l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 1, &l_timeout);

  if (l_ret != 1) {
    VRPN_MSG_ERROR ("vrpn_5dt: Unable to read from the glove\n");
    return -1;
  }

  if (l_inbuf[0] != 85) {
    VRPN_MSG_ERROR ("vrpn_5dt: Cannot get response on init command");
    return -1;
  } else {
    vrpn_flush_input_buffer (serial_fd);
    send_command ( (const unsigned char *) "G", 1); //Command to Query informations from the glove
    vrpn_SleepMsecs (100);  //Give it time to respond
    l_timeout.tv_sec = 2;
    l_timeout.tv_usec = 0;
    l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 32, &l_timeout);

    if (l_ret != 32) {
      VRPN_MSG_ERROR ("vrpn_5dt: Cannot get info. from the glove");
      return -1;
    }
    if ( (l_inbuf[0] != 66) || (l_inbuf[1] != 82)) {
      VRPN_MSG_ERROR ("vrpn_5dt: Cannot get good header on info command");
      return -1;
    }

    sprintf (l_errmsg, "vrpn_5dt: glove \"%s\"version %d.%d\n", &l_inbuf [16], l_inbuf [2], l_inbuf [3]);
    VRPN_MSG_INFO (l_errmsg);

    if (l_inbuf[4] | 1) {
      VRPN_MSG_INFO ("A right glove is ready");
    } else {
      VRPN_MSG_INFO ("A left glove is ready");
    }
    if (l_inbuf[5] | 16) {
      VRPN_MSG_INFO ("Pitch and Roll are available");
    } else {
      VRPN_MSG_INFO ("Pitch and Roll are not available");
    }
  }

  // If we're in continuous mode, request continuous sends
  if (_mode == 2) {
    send_command ( (const unsigned char *) "D", 1); // Command to query streaming data from the glove
  }

  // We're now entering the syncing mode which send the read command to the glove
  _status = STATUS_SYNCING;

  vrpn_gettimeofday (&timestamp, NULL);	// Set watchdog now
  return 0;
}


/******************************************************************************
 * NAME      : vrpn_5dt::syncing
 * ROLE      : Send the "C" command to ask for new data from the glove
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt::syncing (void)
{

  if (_wireless) {
    // For a wireless glove, syncing means we got a header byte and need
    // to wait for the end of the report to see if we guessed right and
    // will get a capability byte.
    int l_ret;
    l_ret = vrpn_read_available_characters (serial_fd, &_buffer [_bufcount],
                                            _expected_chars - _bufcount);
    if (l_ret == -1) {
      VRPN_MSG_ERROR ("Error reading the glove");
      _status = STATUS_RESETTING;
      return;
    }
    _bufcount += l_ret;
    if (_bufcount < _expected_chars) {	// Not done -- go back for more
      return;
    }
    if (_buffer[_bufcount - 1] == 0x40 || _buffer[_bufcount - 1] == 0x01) {
      VRPN_MSG_INFO ("Got capability byte as expected - switching into read mode.");
      _bufcount = 0;
      _status = STATUS_READING;
    } else {
      VRPN_MSG_WARNING ("Got a header byte, but capability byte not found - resetting.");
      _status = STATUS_RESETTING;
    }
    return;
  }

  switch (_mode)
    {
    case 1:
      send_command ((const unsigned char *) "C", 1);  // Command to query data from the glove
      break;
    case 2:
      // Nothing to be done here -- continuous mode was requested in the reset.
      break;
    default :
      VRPN_MSG_ERROR ("vrpn_5dt::syncing : internal error : unknown state");
      printf ("mode %d\n", _mode);
      break;
    }
  _bufcount = 0;
  _status = STATUS_READING;
}


/******************************************************************************
 * NAME      : vrpn_5dt::get_report
 * ROLE      : This function will read characters until it has a full report, then
 *             put that report into analog fields and call the report methods on these.   
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt::get_report (void)
{
  int  l_ret;		// Return value from function call to be checked

  // XXX This should be called when the first character of a report is read.
  vrpn_gettimeofday(&timestamp, NULL);

  //--------------------------------------------------------------------
  // Read as many bytes of this report as we can, storing them
  // in the buffer.  We keep track of how many have been read so far
  // and only try to read the rest.
  //--------------------------------------------------------------------

  l_ret = vrpn_read_available_characters (serial_fd, &_buffer [_bufcount],
                                          _expected_chars - _bufcount);
  if (l_ret == -1) {
      VRPN_MSG_ERROR ("Error reading the glove");
      _status = STATUS_RESETTING;
      return;
  }
#ifdef  VERBOSE
  if (l_ret != 0) printf("... got %d characters (%d total)\n",l_ret, _bufcount);
#endif

  //--------------------------------------------------------------------
  // The time of the report is the time at which the first character for
  // the report is read.
  //--------------------------------------------------------------------

  if ( (l_ret > 0) && (_bufcount == 0) ) {
	vrpn_gettimeofday(&timestamp, NULL);
  }

  //--------------------------------------------------------------------
  // We keep track of how many characters we have received and keep
  // going back until we get as many as we expect.
  //--------------------------------------------------------------------

  _bufcount += l_ret;
  if (_bufcount < _expected_chars) {    // Not done -- go back for more
      return;
  }

  //--------------------------------------------------------------------
  // We now have enough characters to make a full report.  First check to
  // make sure that the first one is what we expect.

  if (_buffer[0] != 128) {
    VRPN_MSG_WARNING ("Unexpected first character in report, resetting");
    _status = STATUS_RESETTING;
    _bufcount = 0;
    return;
  }

  if (_wireless) {
    if (_buffer[_bufcount - 1] != 0x40 && _buffer[_bufcount - 1] != 0x01) {
      // The last byte wasn't a capability byte, so this report is invalid.
      // Reset!
      VRPN_MSG_WARNING ("Unexpected last character in report, resetting");
      _status = STATUS_RESETTING;
      _bufcount = 0;
      return;
    }
  }

#ifdef	VERBOSE
  printf ("Got a complete report (%d of %d)!\n", _bufcount, _expected_chars);
#endif

  //--------------------------------------------------------------------
  // Decode the report and store the values in it into the analog values
  // if appropriate.
  //--------------------------------------------------------------------

  channel[1] = _buffer[1] / 255.0; //Thumb
  channel[2] = _buffer[2] / 255.0;
  channel[3] = _buffer[3] / 255.0;
  channel[4] = _buffer[4] / 255.0;
  channel[5] = _buffer[5] / 255.0; // Pinkie
  channel[6] = 180 * _buffer[6] / 255.0; // Pitch
  channel[7] = 180 * _buffer[7] / 255.0; // Roll

  if (_wireless && !_gotInfo) {
    _gotInfo = true;
    // Bit 0 set in the capability byte implies a right-hand glove.
    if (_buffer[9] == 0x01) {
      VRPN_MSG_INFO ("A 'wireless-type' right glove is ready and reporting");
    } else {
      VRPN_MSG_INFO ("A 'wireless-type' left glove is ready and reporting");
    }
  }

  //--------------------------------------------------------------------
  // Done with the decoding, send the reports and go back to syncing
  //--------------------------------------------------------------------

  report_changes();
  switch (_mode) {
    case 1:
      _status = STATUS_SYNCING;
      break;
    case 2:           // Streaming Mode, just go back for the next report.
      _bufcount = 0;
      break;
    default :
      VRPN_MSG_ERROR ("vrpn_5dt::get_report : internal error : unknown state");
      break;
  }
}


/******************************************************************************
 * NAME      : vrpn_5dt::report_changes
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt::report_changes (vrpn_uint32 class_of_service)
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Analog::report_changes(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_5dt::report
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt::report (vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Analog::report(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_5dt::mainloop
 * ROLE      :  This routine is called each time through the server's main loop. It will
 *              take a course of action depending on the current status of the device,
 *              either trying to reset it or trying to get a reading from it.  It will
 *              try to reset the device if no data has come from it for a couple of
 *              seconds
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt::mainloop ()
{
  char l_errmsg[256];

  server_mainloop();
  if (_wireless) {
    if (!_announced) {
      VRPN_MSG_INFO ("Will connect to a receive-only 'wireless-type' glove - there may be a few warnings before we succeed.");
      _announced = true;
    }
  }
  switch (_status)
    {
    case STATUS_RESETTING:
      if (reset()== -1)
	{
	  VRPN_MSG_ERROR ("vrpn_Analog_5dt: Cannot reset the glove!");
	}
      break;

    case STATUS_SYNCING:
      syncing ();
      break;

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
        get_report();
        struct timeval current_time;
        vrpn_gettimeofday (&current_time, NULL);
        if (vrpn_TimevalDuration (current_time, timestamp) > MAX_TIME_INTERVAL) {
          sprintf (l_errmsg, "vrpn_5dt::mainloop: Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
                   current_time.tv_sec,
                   static_cast<long> (current_time.tv_usec),
                   timestamp.tv_sec,
                   static_cast<long> (timestamp.tv_usec));
          VRPN_MSG_ERROR (l_errmsg);
          _status = STATUS_RESETTING;
        }
      }
      break;

    default:
      VRPN_MSG_ERROR ("vrpn_5dt::mainloop: Unknown mode (internal error)");
      break;
    }
}
