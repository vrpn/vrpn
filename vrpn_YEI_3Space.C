//	This is a driver for the YEI 3Space serial-port devices.

#include <stdio.h>                      // for sprintf, printf

#include "vrpn_YEI_3Space.h"
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
 * NAME      : vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor
 * ROLE      : This creates a vrpn_YEI_3Space_Sensor and sets it to reset mode. It opens
 *             the serial device using the code in the vrpn_Serial_Analog constructor.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor (const char * p_name
                                  , vrpn_Connection * p_c
                                  , const char * p_port
                                  , int p_baud
                                  , bool calibrate_gyros_on_setup
                                  , bool tare_on_setup
                                  , double frames_per_second)
  : vrpn_Serial_Analog (p_name, p_c, p_port, p_baud, 8, vrpn_SER_PARITY_NONE)
  , d_frames_per_second(frames_per_second)
  // XXX Remove these eventually
  , _announced(false)	// Not yet announced our warning.
  , _gotInfo (false)  // used to know if we have gotten a first full wireless report.
{
  // Set the parameters in the parent classes
  vrpn_Analog::num_channel = 14;
  // XXX Set other configs.

  // Set the mode to reset
  _status = STATUS_RESETTING;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 *             int   len : Length of the command to be sent
 * RETURN    : 0 on success, -1 on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
//  0xF7 (start of packet)
//  Command byte (command value)
//  Command data (0 or more bytes of parameters, in big-endian format)
//  Checksum (sum of all bytes in the packet % 256)
int vrpn_YEI_3Space_Sensor::send_command (const unsigned char *p_cmd, int p_len)
{
  const unsigned char START_OF_PACKET = 0xF7;

  // Compute the checksum.  The description of the checksum implies that it should
  // include the START_OF_PACKET data, but the example provided in page 28 does not
  // include that data in the checksum, so we only include the packet data here.
  int checksum = 0;
  for (int i = 0; i < p_len; i++) {
    checksum += p_cmd[i];
  }
  checksum %= 256;

  // Pack the command into the buffer with the start-of-packet and checksum.
  unsigned char  buffer[256];    // Large enough buffer to hold all possible commands.
  buffer[0] = START_OF_PACKET;
  memcpy(&(buffer[1]), p_cmd, p_len);
  buffer[p_len + 1] = static_cast<unsigned char>(checksum);

  printf("XXX Sending %d characters\n", p_len+2);
  for (int i = 0; i < p_len+2; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");

  // Send the command and return whether it worked.
  int l_ret;
  l_ret = vrpn_write_characters (serial_fd, p_cmd, p_len+2);
  // Tell if this all worked.
  if (l_ret == p_len+2) {
    return 0;
  } else {
    return -1;
  }
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::reset
 * ROLE      : This routine will reset the YEI_3Space
 * ARGUMENTS : 
 * RETURN    : 0 else -1 in case of error
 ******************************************************************************/
int vrpn_YEI_3Space_Sensor::reset (void)
{
  struct timeval l_timeout;
  unsigned char	 l_inbuf [256];
  int            l_ret;
  char           l_errmsg[256];

  // Clear out the incoming characters and then ask the device to stop
  // streaming and ask it for the LED mode to make sure we get a response.
  vrpn_flush_input_buffer (serial_fd);
  unsigned char stop_streaming[1] = { 0x56 };
  if (send_command (stop_streaming, sizeof(stop_streaming)) != 0) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send stop-streaming command\n");
    return -1;
  }
  unsigned char get_led_mode[1] = { 0xC8 };
  if (send_command (get_led_mode, sizeof(get_led_mode)) != 0) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send get-led-mode command\n");
    return -1;
  }
  unsigned char start_streaming[1] = { 0x55 };
  if (send_command (start_streaming, sizeof(start_streaming)) != 0) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send get-led-mode command\n");
    return -1;
  }
  l_timeout.tv_sec = 2;
  l_timeout.tv_usec = 0;
  l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, sizeof(l_inbuf), &l_timeout);
  printf("XXX received %d characters\n", l_ret);
  for (int i = 0; i < l_ret; i++) {
    printf("%02X ", l_inbuf[i]);
  }
  printf("\n");
  exit(0);

  // XXX We get nothing when we try binary commands, either at 115200 or at 9600
  // baud.  When we run Tera Term (open-source terminal program), :0 gives us a
  // quaternion returned at either baud rate.  Try doing ASCII and see if we get
  // a response.  If so, we're maybe a bit closer to knowing this can work.

  l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 1, &l_timeout);

  if (l_ret != 1) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor: Unable to read from the glove\n");
    return -1;
  }

  if (l_inbuf[0] != 85) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor: Cannot get response on init command");
    return -1;
  } else {
    vrpn_flush_input_buffer (serial_fd);
    send_command ( (unsigned char *) "G", 1); //Command to Query informations from the glove
    vrpn_SleepMsecs (100);  //Give it time to respond
    l_timeout.tv_sec = 2;
    l_timeout.tv_usec = 0;
    l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 32, &l_timeout);

    if (l_ret != 32) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor: Cannot get info. from the glove");
      return -1;
    }
    if ( (l_inbuf[0] != 66) || (l_inbuf[1] != 82)) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor: Cannot get good header on info command");
      return -1;
    }

    sprintf (l_errmsg, "vrpn_YEI_3Space_Sensor: glove \"%s\"version %d.%d\n", &l_inbuf [16], l_inbuf [2], l_inbuf [3]);
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
    send_command ( (unsigned char *) "D", 1); // Command to query streaming data from the glove
  }

  // We're now entering the syncing mode which send the read command to the glove
  _status = STATUS_SYNCING;

  vrpn_gettimeofday (&timestamp, NULL);	// Set watchdog now
  return 0;
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::syncing
 * ROLE      : Send the "C" command to ask for new data from the glove
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space_Sensor::syncing (void)
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
      send_command ((unsigned char *) "C", 1);  // Command to query data from the glove
      break;
    case 2:
      // Nothing to be done here -- continuous mode was requested in the reset.
      break;
    default :
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::syncing : internal error : unknown state");
      printf ("mode %d\n", _mode);
      break;
    }
  _bufcount = 0;
  _status = STATUS_READING;
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::get_report
 * ROLE      : This function will read characters until it has a full report, then
 *             put that report into analog fields and call the report methods on these.   
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space_Sensor::get_report (void)
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
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::get_report : internal error : unknown state");
      break;
  }
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::report_changes
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space_Sensor::report_changes (vrpn_uint32 class_of_service)
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Analog::report_changes(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::report
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space_Sensor::report (vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Analog::report(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::mainloop
 * ROLE      :  This routine is called each time through the server's main loop. It will
 *              take a course of action depending on the current status of the device,
 *              either trying to reset it or trying to get a reading from it.  It will
 *              try to reset the device if no data has come from it for a couple of
 *              seconds
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space_Sensor::mainloop ()
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
	  VRPN_MSG_ERROR ("vrpn_Analog_YEI_3Space: Cannot reset the glove!");
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
          sprintf (l_errmsg, "vrpn_YEI_3Space_Sensor::mainloop: Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
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
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::mainloop: Unknown mode (internal error)");
      break;
    }
}
