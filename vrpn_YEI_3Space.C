//	This is a driver for the YEI 3Space serial-port devices.

#include <stdio.h>                      // for sprintf, printf

#include "vrpn_YEI_3Space.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR
#include "quat.h"

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_READING		(0)	// Looking for the a report

static const int REPORT_LENGTH = 16 + 16 + 36 + 4 + 4;
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
                                  , double frames_per_second
                                  , double red_LED_color
                                  , double green_LED_color
                                  , double blue_LED_color
                                  , int LED_mode)
  : vrpn_Tracker_Server (p_name, p_c, 2)
  , vrpn_Serial_Analog (p_name, p_c, p_port, p_baud, 8, vrpn_SER_PARITY_NONE)
  , d_frames_per_second(frames_per_second)
{
  // Set the parameters in the parent classes
  vrpn_Analog::num_channel = 11;

  // Configure LED mode.
  unsigned char set_LED_mode[2] = { 0xC4, 0 };
  set_LED_mode[1] = static_cast<unsigned char>(LED_mode);
  if (!send_command (set_LED_mode, sizeof(set_LED_mode))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor: Unable to send set-LED-mode command\n");
  }

  // Configure LED color.
  unsigned char set_LED_color[13] = { 0xEE, 0,0,0,0,0,0,0,0,0,0,0,0 };
  unsigned char *bufptr = &set_LED_color[1];
  vrpn_int32 buflen = 12;
  vrpn_float32  LEDs[3];
  LEDs[0] = static_cast<vrpn_float32>(red_LED_color);
  LEDs[1] = static_cast<vrpn_float32>(green_LED_color);
  LEDs[2] = static_cast<vrpn_float32>(blue_LED_color);
  vrpn_buffer(&bufptr, &buflen, LEDs[0]);
  vrpn_buffer(&bufptr, &buflen, LEDs[1]);
  vrpn_buffer(&bufptr, &buflen, LEDs[2]);
  if (!send_command (set_LED_color, sizeof(set_LED_color))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor: Unable to send set-LED-color command\n");
  }

  // If we're supposed to calibrate the gyros on startup, do so now.
  if (calibrate_gyros_on_setup) {
    unsigned char begin_gyroscope_calibration[1] = { 0xA5 };
    if (!send_command (begin_gyroscope_calibration, sizeof(begin_gyroscope_calibration))) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor: Unable to send set-gyroscope-calibration command\n");
    }
  }

  // If we're supposed to tare on startup, do so now.
  if (tare_on_setup) {
    unsigned char tare[1] = { 0x60 };
    if (!send_command (tare, sizeof(tare))) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor: Unable to send tare command\n");
    }
  }

  vrpn_gettimeofday(&timestamp, NULL);

  // Set the mode to reset
  d_status = STATUS_RESETTING;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::send_command
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 *             int   len : Length of the command to be sent
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
//  0xF7 (start of packet)
//  Command byte (command value)
//  Command data (0 or more bytes of parameters, in big-endian format)
//  Checksum (sum of all bytes in the packet % 256)
bool vrpn_YEI_3Space_Sensor::send_command (const unsigned char *p_cmd, int p_len)
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

  // Send the command and return whether it worked.
  int l_ret;
  l_ret = vrpn_write_characters (serial_fd, buffer, p_len+2);
  // Tell if this all worked.
  if (l_ret == p_len+2) {
    return true;
  } else {
    return false;
  }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::receive_LED_mode_response
 * ROLE      : 
 * ARGUMENTS : struct timeval *timeout; how long to wait, NULL for forever
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
// Single byte: value 0 or 1.
bool vrpn_YEI_3Space_Sensor::receive_LED_mode_response (struct timeval *timeout)
{
  unsigned char value;
  int ret = vrpn_read_available_characters (serial_fd, &value, sizeof(value), timeout);
  if (ret != sizeof(value)) {
    return false;
  }
  d_LED_mode = value;
  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::receive_LED_values_response
 * ROLE      : 
 * ARGUMENTS : struct timeval *timeout; how long to wait, NULL for forever
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
// Three four-byte float responses, each big-endian.
bool vrpn_YEI_3Space_Sensor::receive_LED_values_response (struct timeval *timeout)
{
  unsigned char buffer[3*sizeof(vrpn_float32)];
  int ret = vrpn_read_available_characters (serial_fd, buffer, sizeof(buffer), timeout);
  if (ret != sizeof(buffer)) {
    return false;
  }
  vrpn_float32 value;
  unsigned char *bufptr = buffer;
  vrpn_unbuffer(&bufptr, &value);
  d_LED_color[0] = value;
  vrpn_unbuffer(&bufptr, &value);
  d_LED_color[1] = value;
  vrpn_unbuffer(&bufptr, &value);
  d_LED_color[2] = value;
  return true;
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

  // Ask the device to stop streaming and then wait a little and flush the
  // input buffer and then ask it for the LED mode and make sure we get a response.
  unsigned char stop_streaming[1] = { 0x56 };
  if (!send_command (stop_streaming, sizeof(stop_streaming))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send stop-streaming command\n");
    return -1;
  }
  vrpn_SleepMsecs(50);
  vrpn_flush_input_buffer (serial_fd);
  unsigned char get_led_mode[1] = { 0xC8 };
  if (!send_command (get_led_mode, sizeof(get_led_mode))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send get-led-mode command\n");
    return -1;
  }
  l_timeout.tv_sec = 2;
  l_timeout.tv_usec = 0;
  if (!receive_LED_mode_response(&l_timeout)) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to read get-led-mode response\n");
    return -1;
  }

  // Request the 3 LED colors and set our internal values to match them.
  unsigned char get_led_values[1] = { 0xEF };
  if (!send_command (get_led_values, sizeof(get_led_values))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send get-led-values command\n");
    return -1;
  }
  if (!receive_LED_values_response(&l_timeout)) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to read get-led-mode response\n");
    return -1;
  }

  // Configure streaming speed based on the requested frames/second value.
  unsigned char set_streaming_timing[13] = { 0x52, 0,0,0,0,0,0,0,0,0,0,0,0 };
  vrpn_uint32 interval;
  if (d_frames_per_second <= 0) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Bad frames/second value, setting to maximum\n");
    interval = 0;
  } else {
    interval = static_cast<vrpn_uint32>(1e6 * 1 / d_frames_per_second);
  }
  vrpn_uint32 duration = 0xFFFFFFFF;
  vrpn_uint32 delay = 0;
  unsigned char *bufptr = &set_streaming_timing[1];
  vrpn_int32 buflen = 12;
  vrpn_buffer(&bufptr, &buflen, interval);
  vrpn_buffer(&bufptr, &buflen, duration);
  vrpn_buffer(&bufptr, &buflen, delay);
  if (!send_command (set_streaming_timing, sizeof(set_streaming_timing))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send set-streaming-timing command\n");
    return -1;
  }

  // Configure the set of values we want to have streamed.
  // The value 0xFF means "nothing."  We want to stream the untared
  // orientiation as a quaternion, the tared orientation as a quaternion,
  // all corrected sensor data, the temperature in Celsius, and the
  // confidence factor.
  unsigned char set_streaming_slots[9] = { 0x50, 0x06,0x00,0x25,0x2B,0x2D,0xFF,0xFF,0xFF };
  if (!send_command (set_streaming_slots, sizeof(set_streaming_slots))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send set-streaming-slots command\n");
    return -1;
  }

  // Start streaming mode.
  unsigned char start_streaming[1] = { 0x55 };
  if (!send_command (start_streaming, sizeof(start_streaming))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::reset: Unable to send start-streaming command\n");
    return -1;
  }

  // We're now entering the reading mode with no characters.
  d_expected_characters = REPORT_LENGTH;
  d_characters_read = 0;
  d_status = STATUS_READING;

  vrpn_gettimeofday (&timestamp, NULL);	// Set watchdog now
  return 0;
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

  //--------------------------------------------------------------------
  // Read as many bytes of this report as we can, storing them
  // in the buffer.  We keep track of how many have been read so far
  // and only try to read the rest.
  //--------------------------------------------------------------------

  l_ret = vrpn_read_available_characters (serial_fd, &d_buffer [d_characters_read],
                                          d_expected_characters - d_characters_read);
  if (l_ret == -1) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::get_report(): Error reading the glove");
      d_status = STATUS_RESETTING;
      return;
  }
#ifdef  VERBOSE
  if (l_ret != 0) printf("... got %d characters (%d total)\n",l_ret, d_characters_read);
#endif

  //--------------------------------------------------------------------
  // The time of the report is the time at which the first character for
  // the report is read.
  //--------------------------------------------------------------------

  if ( (l_ret > 0) && (d_characters_read == 0) ) {
	vrpn_gettimeofday(&timestamp, NULL);
  }

  //--------------------------------------------------------------------
  // We keep track of how many characters we have received and keep
  // going back until we get as many as we expect.
  //--------------------------------------------------------------------

  d_characters_read += l_ret;
  if (d_characters_read < d_expected_characters) {    // Not done -- go back for more
      return;
  }

  //--------------------------------------------------------------------
  // We now have enough characters to make a full report.  Parse each
  // input in order and check to make sure they work, then send the results.
  //--------------------------------------------------------------------
  vrpn_float32 value;
  unsigned char *bufptr = d_buffer;

  // Read the two orientations and report them
  q_vec_type  pos;
  pos[Q_X] = pos[Q_Y] = pos[Q_Z] = 0;
  q_type  quat;

  for (int i = 0; i < 2; i++) {
    vrpn_unbuffer(&bufptr, &value);
    quat[Q_X] = value;
    vrpn_unbuffer(&bufptr, &value);
    quat[Q_Y] = value;
    vrpn_unbuffer(&bufptr, &value);
    quat[Q_Z] = value;
    vrpn_unbuffer(&bufptr, &value);
    quat[Q_W] = value;
    if (0 != report_pose(i, timestamp, pos, quat)) {
        VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::get_report(): Error sending sensor report");
        d_sensor = STATUS_RESETTING;
    }
  }

  // Read the analog values and put them into the channels.
  for (int i = 0; i < vrpn_Analog::getNumChannels(); i++) {
    vrpn_unbuffer(&bufptr, &value);
    channel[i] = value;
  }

  //--------------------------------------------------------------------
  // Done with the decoding, send the analog reports and clear our counts
  //--------------------------------------------------------------------

  vrpn_Analog::report_changes();
  d_expected_characters = REPORT_LENGTH;
  d_characters_read = 0;
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

  vrpn_Tracker_Server::mainloop();
  server_mainloop();

  switch (d_status)
    {
    case STATUS_RESETTING:
      if (reset()== -1) {
	  VRPN_MSG_ERROR ("vrpn_Analog_YEI_3Space: Cannot reset!");
      }
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
          d_status = STATUS_RESETTING;
        }
      }
      break;

    default:
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::mainloop: Unknown mode (internal error)");
      break;
    }
}
