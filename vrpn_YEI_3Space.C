//	This is a driver for the YEI 3Space serial-port devices.

#include <stdio.h>                      // for sprintf, printf

#include "vrpn_YEI_3Space.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR
#include "quat.h"

//#define VERBOSE

// Defines the modes in which the device can find itself.
#define STATUS_NOT_INITIALIZED  (-2)    // Not yet initialized
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_READING		(0)	// Looking for the a report

// The wired USB connection can stream up to 256 bytes, but the wireless
// can stream a maximum of 96 bytes per sensor.
static const int REPORT_LENGTH = 16 + 16 + 12 + 36 + 4 + 4 + 1;
#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)


/******************************************************************************
 * NAME      : vrpn_YEI_3Space::vrpn_YEI_3Space
 * ROLE      : This creates a vrpn_YEI_3Space and sets it to reset mode.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space::vrpn_YEI_3Space (const char * p_name
                                  , vrpn_Connection * p_c
                                  , double frames_per_second
                                  , const char *reset_commands[])
  : vrpn_Tracker_Server (p_name, p_c, 2)
  , vrpn_Analog (p_name, p_c)
  , vrpn_Button_Filter(p_name, p_c)
  , d_frames_per_second(frames_per_second)
{
  // Count the reset commands and allocate an array to store them in.
  const char **ptr = reset_commands;
  d_reset_commands = NULL;
  d_reset_command_count = 0;
  if (ptr != NULL) while ((*ptr) != NULL) {
    d_reset_command_count++;
    ptr++;
  }
  if (d_reset_command_count > 0) {
    d_reset_commands = new char *[d_reset_command_count];
  }

  // Copy any reset commands.
  ptr = reset_commands;
  for (int i = 0; i < d_reset_command_count; i++) {
    d_reset_commands[i] = new char[strlen(reset_commands[i]) + 1];
    strcpy(d_reset_commands[i], reset_commands[i]);
  }

  // Set the parameters in the parent classes
  vrpn_Analog::num_channel = 11;

  // Initialize the state of all the buttons
  vrpn_Button::num_buttons = 8;
  memset(buttons, 0, sizeof(buttons));
  memset(lastbuttons, 0, sizeof(lastbuttons));

  vrpn_gettimeofday(&timestamp, NULL);

  // We're constructed, but not yet initialized.
  d_status = STATUS_NOT_INITIALIZED;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space::~vrpn_YEI_3Space
 * ROLE      : This destroys a vrpn_YEI_3Space and frees its memory.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space::~vrpn_YEI_3Space()
{
  // Free the space used to store the additional reset commands,
  // then free the array used to store the pointers.
  try {
    for (int i = 0; i < d_reset_command_count; i++) {
      delete[] d_reset_commands[i];
    }
    if (d_reset_commands != NULL) {
      delete[] d_reset_commands;
      d_reset_commands = NULL;
    }
  } catch (...) {
    fprintf(stderr, "vrpn_YEI_3Space::~vrpn_YEI_3Space(): delete failed\n");
    return;
  }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space::vrpn_YEI_3Space
 * ROLE      : This creates a vrpn_YEI_3Space and sets it to reset mode.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
void vrpn_YEI_3Space::init (bool calibrate_gyros_on_setup
                       , bool tare_on_setup
                       , double red_LED_color
                       , double green_LED_color
                       , double blue_LED_color
                       , int LED_mode)
{
  // Configure LED mode.
  unsigned char set_LED_mode[2] = { 0xC4, 0 };
  set_LED_mode[1] = static_cast<unsigned char>(LED_mode);
  if (!send_binary_command(set_LED_mode, sizeof(set_LED_mode))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::init: Unable to send set-LED-mode command\n");
  }
#ifdef  VERBOSE
  printf("LED mode set\n");
#endif

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
  if (!send_binary_command (set_LED_color, sizeof(set_LED_color))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::init: Unable to send set-LED-color command\n");
  }

  // If we're supposed to calibrate the gyros on startup, do so now.
  if (calibrate_gyros_on_setup) {
    unsigned char begin_gyroscope_calibration[1] = { 0xA5 };
    if (!send_binary_command (begin_gyroscope_calibration, sizeof(begin_gyroscope_calibration))) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space::init: Unable to send set-gyroscope-calibration command\n");
    }
  }

  // If we're supposed to tare on startup, do so now.
  if (tare_on_setup) {
    unsigned char tare[1] = { 0x60 };
    if (!send_binary_command (tare, sizeof(tare))) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space::init: Unable to send tare command\n");
    }
  }

  // Set the mode to reset
  vrpn_gettimeofday(&timestamp, NULL);
  d_status = STATUS_RESETTING;
#ifdef  VERBOSE
  printf("init() complete\n");
#endif
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space::reset
 * ROLE      : This routine will reset the YEI_3Space
 * ARGUMENTS : 
 * RETURN    : 0 else -1 in case of error
 ******************************************************************************/
int vrpn_YEI_3Space::reset (void)
{
  struct timeval l_timeout;

  // Ask the device to stop streaming and then wait a little and flush the
  // input buffer and then ask it for the LED mode and make sure we get a response.
  unsigned char stop_streaming[1] = { 0x56 };
  if (!send_binary_command (stop_streaming, sizeof(stop_streaming))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send stop-streaming command\n");
    return -1;
  }
  vrpn_SleepMsecs(50);
  flush_input();
  unsigned char get_led_mode[1] = { 0xC8 };
  if (!send_binary_command (get_led_mode, sizeof(get_led_mode))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send get-led-mode command\n");
    return -1;
  }
  l_timeout.tv_sec = 2;
  l_timeout.tv_usec = 0;
  if (!receive_LED_mode_response(&l_timeout)) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to read get-led-mode response\n");
    return -1;
  }

  // Request the 3 LED colors and set our internal values to match them.
  unsigned char get_led_values[1] = { 0xEF };
  if (!send_binary_command (get_led_values, sizeof(get_led_values))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send get-led-values command\n");
    return -1;
  }
  if (!receive_LED_values_response(&l_timeout)) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to read get-led-mode response\n");
    return -1;
  }

  // Change into the x-right, z-up right handed CS we want
  unsigned char set_rh_system[] = { 0x74, 0x01 };
  if (!send_binary_command(set_rh_system, sizeof(set_rh_system))) {
      VRPN_MSG_ERROR("vrpn_YEI_3Space::reset: Unable to send coordinate system selection command\n");
      return -1;
  }

  // Configure streaming speed based on the requested frames/second value.
  unsigned char set_streaming_timing[13] = { 0x52, 0,0,0,0,0,0,0,0,0,0,0,0 };
  vrpn_uint32 interval;
  if (d_frames_per_second <= 0) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Bad frames/second value, setting to maximum\n");
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
  if (!send_binary_command (set_streaming_timing, sizeof(set_streaming_timing))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send set-streaming-timing command\n");
    return -1;
  }

  // Configure the set of values we want to have streamed.
  // The value 0xFF means "nothing."  We want to stream the untared
  // orientiation as a quaternion, the tared orientation as a quaternion,
  // all corrected sensor data, the temperature in Celsius, and the
  // confidence factor.
  unsigned char set_streaming_slots[9] = { 0x50,
    0x06, // untared quat
    0x00, // tared quat
    0x29, // tared corrected linear acceleration with gravity removed
    0x25, // all corrected sensor data (3D vectors: rate gyro in rad/s, accel in g, and compass in gauss)
    0x2B, // temperature C
    0x2D, // confidence factor
    0xFA, // button state
    0xFF }; // followed by empty streaming spots.
  if (!send_binary_command (set_streaming_slots, sizeof(set_streaming_slots))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send set-streaming-slots command\n");
    return -1;
  }

  // Send any additional ACII reset commands that were passed into the
  // constructor.
  for (int i = 0; i < d_reset_command_count; i++) {
    if (!send_ascii_command(d_reset_commands[i])) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send additional reset command\n");
      return -1;
    }
  }

  // Start streaming mode.
  unsigned char start_streaming[1] = { 0x55 };
  if (!send_binary_command (start_streaming, sizeof(start_streaming))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::reset: Unable to send start-streaming command\n");
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
 * NAME      : vrpn_YEI_3Space::handle_report
 * ROLE      : This function will parse  a full report, then
 *             put that report into analog fields and call the report methods on these.   
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space::handle_report(unsigned char *report)
{
  vrpn_float32 value;
  unsigned char *bufptr = report;

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
        VRPN_MSG_ERROR ("vrpn_YEI_3Space::handle_report(): Error sending sensor report");
        d_status = STATUS_RESETTING;
    }
  }

  // XXX Linear rate gyros into orientation change?

  // Read the three values for linear acceleration in tared
  // space with gravity removed.  Convert it into units of
  // meters/second/second.  Put it into the tared sensor.
  q_vec_type acc;
  q_type  acc_quat;
  acc_quat[Q_X] = acc_quat[Q_Y] = acc_quat[Q_Z] = 0; acc_quat[Q_W] = 1;
  vrpn_unbuffer(&bufptr, &value);
  static const double GRAVITY = 9.80665;  // Meters/second/second
  acc[Q_X] = value * GRAVITY;
  vrpn_unbuffer(&bufptr, &value);
  acc[Q_Y] = value * GRAVITY;
  vrpn_unbuffer(&bufptr, &value);
  acc[Q_Z] = value * GRAVITY;
  int sensor = 1;
  double interval = 1;
  if (0 != report_pose_acceleration(sensor, timestamp, acc, acc_quat, interval)) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space::handle_report(): Error sending acceleration report");
      d_status = STATUS_RESETTING;
  }

  // Read the analog values and put them into the channels.
  for (int i = 0; i < vrpn_Analog::getNumChannels(); i++) {
    vrpn_unbuffer(&bufptr, &value);
    channel[i] = value;
  }

  // Check the confidence factor to make sure it is between 0 and 1.
  // If not, there is trouble parsing the report.  Sometimes the wired
  // unit gets the wrong number of bytes in the report, causing things
  // to wrap around.  This catches that case.
  if ((channel[10] < 0) || (channel[10] > 1)) {
	  VRPN_MSG_ERROR("vrpn_YEI_3Space::handle_report(): Invalid confidence, resetting");
      d_status = STATUS_RESETTING;
  }

  // Read the button values and put them into the buttons.
  vrpn_uint8 b;
  vrpn_unbuffer(&bufptr, &b);
  for (int i = 0; i < 8; i++) {
    buttons[i] = (b & (1 << i)) != 0;
  }

  //--------------------------------------------------------------------
  // Done with the decoding, send the analog and button reports.
  //--------------------------------------------------------------------

  report_changes();
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space::report_changes
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space::report_changes (vrpn_uint32 class_of_service)
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Analog::report_changes(class_of_service);
  vrpn_Button::timestamp = timestamp;
  vrpn_Button::report_changes();
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space::report
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space::report (vrpn_uint32 class_of_service)
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Analog::report(class_of_service);
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space::mainloop
 * ROLE      :  This routine is called each time through the server's main loop. It will
 *              take a course of action depending on the current status of the device,
 *              either trying to reset it or trying to get a reading from it.  It will
 *              try to reset the device if no data has come from it for a couple of
 *              seconds
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_YEI_3Space::mainloop ()
{
  char l_errmsg[256];

  server_mainloop();

  switch (d_status)
    {
    case STATUS_NOT_INITIALIZED:
      break;

    case STATUS_RESETTING:
      if (reset()== -1) {
	  VRPN_MSG_ERROR ("vrpn_YEI_3Space: Cannot reset!");
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
        while (get_report()) {}
        struct timeval current_time;
        vrpn_gettimeofday (&current_time, NULL);
        if (vrpn_TimevalDuration (current_time, timestamp) > MAX_TIME_INTERVAL) {
          sprintf (l_errmsg, "vrpn_YEI_3Space::mainloop: Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
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
      VRPN_MSG_ERROR ("vrpn_YEI_3Space::mainloop: Unknown mode (internal error)");
      break;
    }
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor
 * ROLE      : This creates a vrpn_YEI_3Space_Sensor and sets it to reset mode. It opens
 *             the serial port to use to communicate to it.
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
                                  , int LED_mode
                                  , const char *reset_commands[])
  : vrpn_YEI_3Space (p_name, p_c, frames_per_second, reset_commands)
{
  // Open the serial port we're going to use to talk with the device.
  if ((d_serial_fd = vrpn_open_commport(p_port, p_baud,
    8, vrpn_SER_PARITY_NONE)) == -1) {
      perror("vrpn_YEI_3Space_Sensor::vrpn_YEI_3Space_Sensor: Cannot open serial port");
      fprintf(stderr," (port %s)\n", p_port);
  }

  // Initialize the state of the device, now that we've established a
  // way to talk with it.
  init(calibrate_gyros_on_setup
       , tare_on_setup, red_LED_color
       , green_LED_color, blue_LED_color, LED_mode);
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::~vrpn_YEI_3Space_Sensor
 * ROLE      : This destroys a vrpn_YEI_3Space_Sensor and closes its ports.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space_Sensor::~vrpn_YEI_3Space_Sensor ()
{
    // Ask the device to stop streaming.
    unsigned char stop_streaming[1] = { 0x56 };
    if (!send_binary_command(stop_streaming, sizeof(stop_streaming))) {
        VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::~vrpn_YEI_3Space_Sensor_Wireless: Unable to send stop-streaming command\n");
    }

    // Close com port when destroyed.
    if (d_serial_fd != -1) {
        vrpn_close_commport(d_serial_fd);
    }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::send_binary_command
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
bool vrpn_YEI_3Space_Sensor::send_binary_command (const unsigned char *p_cmd, int p_len)
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
  l_ret = vrpn_write_characters (d_serial_fd, buffer, p_len+2);
  // Tell if this all worked.
  if (l_ret == p_len+2) {
    return true;
  } else {
    return false;
  }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor::send_ascii_command
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Ascii-mode command packet format:
//  ':' (start of packet)
//  Command value (decimal)
//  ',' Command data (0 or more bytes of parameters
//  '\n'
bool vrpn_YEI_3Space_Sensor::send_ascii_command (const char *p_cmd)
{
  // Check to make sure we have a non-empty command
  if (strlen(p_cmd) == 0) {
    return false;
  }

  // Allocate space for the command plus padding and zero terminator
  int buflen = static_cast<int>(strlen(p_cmd) + 3);
  unsigned char *buffer;
  try { buffer = new unsigned char[buflen]; }
  catch (...) {
    return false;
  }

  // Fill in the command
  buffer[0] = ':';
  memcpy(&buffer[1], p_cmd, strlen(p_cmd));
  buffer[buflen-2] = '\n';
  buffer[buflen-1] = 0;

  // Send the command and see if it worked.
  int l_ret = vrpn_write_characters (d_serial_fd, buffer, buflen);

  // Free the buffer.
  try {
    delete[] buffer;
  } catch (...) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor::send_ascii_command(): delete failed\n");
    return false;
  }

  // Tell if sending worked.
  if (l_ret == buflen) {
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
  int ret = vrpn_read_available_characters (d_serial_fd, &value, sizeof(value), timeout);
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
  int ret = vrpn_read_available_characters (d_serial_fd, buffer, sizeof(buffer), timeout);
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
 * NAME      : vrpn_YEI_3Space_Sensor::get_report
 * ROLE      : This function will read characters until it has a full report, then
 *             call handle_report() with the report and clear the counts.   
 * ARGUMENTS : void
 * RETURN    : bool: Did I get a complete report?
 ******************************************************************************/
bool vrpn_YEI_3Space_Sensor::get_report (void)
{
  int  l_ret;		// Return value from function call to be checked

  //--------------------------------------------------------------------
  // Read as many bytes of this report as we can, storing them
  // in the buffer.  We keep track of how many have been read so far
  // and only try to read the rest.
  //--------------------------------------------------------------------

  l_ret = vrpn_read_available_characters (d_serial_fd, &d_buffer [d_characters_read],
                                          d_expected_characters - d_characters_read);
  if (l_ret == -1) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor::get_report(): Error reading the sensor, resetting");
      d_status = STATUS_RESETTING;
      return false;
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
      return false;
  }

  //--------------------------------------------------------------------
  // We now have enough characters to make a full report.  Parse each
  // input in order and check to make sure they work, then send the results.
  //--------------------------------------------------------------------
  handle_report(d_buffer);

  //--------------------------------------------------------------------
  // Clear our counts to be ready for the next report
  //--------------------------------------------------------------------
  d_expected_characters = REPORT_LENGTH;
  d_characters_read = 0;
  return true;
}

void vrpn_YEI_3Space_Sensor::flush_input(void)
{
    vrpn_flush_input_buffer (d_serial_fd);
}


/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless
 * ROLE      : This creates a vrpn_YEI_3Space_Sensor_Wireless and sets it to
 *             reset mode. This constructor is for the first sensor on a
 *             given dongle, so it opens the serial port and configures the
 *             dongle.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless (const char * p_name
                                  , vrpn_Connection * p_c
                                  , int logical_id
                                  , const char * p_port
                                  , int p_baud
                                  , bool calibrate_gyros_on_setup
                                  , bool tare_on_setup
                                  , double frames_per_second
                                  , double red_LED_color
                                  , double green_LED_color
                                  , double blue_LED_color
                                  , int LED_mode
                                  , const char *reset_commands[])
  : vrpn_YEI_3Space (p_name, p_c, frames_per_second, reset_commands)
  , d_i_am_first(true)
  , d_logical_id(-1)
{
  // Open the serial port we're going to use to talk with the device.
  if ((d_serial_fd = vrpn_open_commport(p_port, p_baud,
    8, vrpn_SER_PARITY_NONE)) == -1) {
      perror("vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless: Cannot open serial port");
      fprintf(stderr," (port %s)\n", p_port);
  }
#ifdef  VERBOSE
  printf("Serial port opened\n");
#endif

  // Initialize the dongle state, since we are the first device to connect
  // to it.
  if (!configure_dongle()){
    fprintf(stderr,"vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless: Could not configure dongle\n");
    vrpn_close_commport(d_serial_fd);
    d_serial_fd = -1;
    return;
  }
#ifdef  VERBOSE
  printf("Dongle configured\n");
#endif

  d_logical_id = logical_id;

#ifdef  VERBOSE
  printf("Logical ID set\n");
#endif
  // Initialize the state of the device, now that we've established a
  // way to talk with it.
  init(calibrate_gyros_on_setup
       , tare_on_setup, red_LED_color
       , green_LED_color, blue_LED_color, LED_mode);
#ifdef  VERBOSE
  printf("Constructor done\n");
#endif
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless
 * ROLE      : This creates a vrpn_YEI_3Space_Sensor_Wireless and sets it to
 *             reset mode. This constructor is for not the first sensor on a
 *             given dongle.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space_Sensor_Wireless::vrpn_YEI_3Space_Sensor_Wireless (const char * p_name
                                  , vrpn_Connection * p_c
                                  , int logical_id
                                  , int serial_file_descriptor
                                  , bool calibrate_gyros_on_setup
                                  , bool tare_on_setup
                                  , double frames_per_second
                                  , double red_LED_color
                                  , double green_LED_color
                                  , double blue_LED_color
                                  , int LED_mode
                                  , const char *reset_commands[])
  : vrpn_YEI_3Space (p_name, p_c, frames_per_second, reset_commands)
  , d_i_am_first(false)
  , d_serial_fd(serial_file_descriptor)
  , d_logical_id(-1)
{
  d_logical_id = logical_id;

  // Initialize the state of the device, now that we've established a
  // way to talk with it.
  init(calibrate_gyros_on_setup
       , tare_on_setup, red_LED_color
       , green_LED_color, blue_LED_color, LED_mode);
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::~vrpn_YEI_3Space_Sensor_Wireless
 * ROLE      : This destroys a vrpn_YEI_3Space_Sensor_Wireless and closes its ports.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_YEI_3Space_Sensor_Wireless::~vrpn_YEI_3Space_Sensor_Wireless ()
{
  // Ask the device to stop streaming.
  unsigned char stop_streaming[1] = { 0x56 };
  if (!send_binary_command(stop_streaming, sizeof(stop_streaming))) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::~vrpn_YEI_3Space_Sensor_Wireless: Unable to send stop-streaming command\n");
  }

  // Close com port when destroyed, if I am the first device on the dongle.
  if (d_i_am_first) if (d_serial_fd != -1) {
      vrpn_close_commport(d_serial_fd);
  }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::configure_dongle
 * ROLE      : * ARGUMENTS : 
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

bool vrpn_YEI_3Space_Sensor_Wireless::configure_dongle()
{
  // Configure the wireless streaming mode to manual flush.
  unsigned char set_mode[2] = { 0xb0, 0 };
  if (!send_binary_command_to_dongle(set_mode, sizeof(set_mode))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::configure_dongle: Unable to send set-streaming-mode command\n");
    return false;
  }

  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::configure_dongle
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

bool vrpn_YEI_3Space_Sensor_Wireless::set_logical_id(vrpn_uint8 logical_id, int serial_number)
{
  // Configure logical ID.
  unsigned char set_id[6] = { 0xd1, 0, 0,0,0,0 };
  unsigned char *bufptr = &set_id[1];
  vrpn_int32 buflen = 5;
  vrpn_buffer(&bufptr, &buflen, logical_id);
  vrpn_buffer(&bufptr, &buflen, serial_number);
  if (!send_binary_command_to_dongle(set_id, sizeof(set_id))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::set_logical_id: Unable to send set-logical-id command\n");
    return false;
  }
#ifdef  VERBOSE
  printf("... Logical id %d set to serial_number %x\n", logical_id, serial_number);
#endif

  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::send_binary_command_to_dongle
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 *             int   len : Length of the command to be sent
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// This is the same as the send_binary_command() from the wired interface.
bool vrpn_YEI_3Space_Sensor_Wireless::send_binary_command_to_dongle (const unsigned char *p_cmd, int p_len)
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
  l_ret = vrpn_write_characters (d_serial_fd, buffer, p_len+2);
  // Tell if this all worked.
  if (l_ret == p_len+2) {
    return true;
  } else {
    return false;
  }
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::send_binary_command
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 *             int   len : Length of the command to be sent
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode wireless command packet format:
//  0xF8 (start of packet)
//  Logical address
//  Command byte (command value)
//  Command data (0 or more bytes of parameters, in big-endian format)
//  Checksum (sum of all bytes in the packet including the address) % 256
// NOTE: In the wireless protocol, all commands are acknowledged, so we
// wait for the response to make sure the command was properly received.
bool vrpn_YEI_3Space_Sensor_Wireless::send_binary_command (const unsigned char *p_cmd, int p_len)
{
  const unsigned char START_OF_PACKET = 0xF8;

  // Pack the command into the buffer with the start-of-packet and checksum.
  unsigned char  buffer[256];    // Large enough buffer to hold all possible commands.
  buffer[0] = START_OF_PACKET;
  buffer[1] = d_logical_id;
  memcpy(&(buffer[2]), p_cmd, p_len);

  // Compute the checksum.  The description of the checksum implies that it should
  // include the START_OF_PACKET data, but the example provided in page 25 does not
  // include that data in the checksum, so we only include the address and
  // packet data here.
  int checksum = 0;
  for (int i = 0; i < p_len+1; i++) {
    checksum += buffer[1 + i];
  }
  checksum %= 256;
  buffer[p_len + 2] = static_cast<unsigned char>(checksum);

  // Send the command.
  int l_ret;
  l_ret = vrpn_write_characters (d_serial_fd, buffer, p_len+3);
  if (l_ret != p_len+3) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_binary_command: Could not send command\n");
    return false;
  }
#ifdef  VERBOSE
  printf("... packet of length %d sent\n", l_ret);
#endif

  // Listen for a response telling whether the command was received.
  // We do not listen for the data returned by the command, but only
  // check the header on the return to make sure the command was a
  // success.  The caller should separately look for any requested
  // data.
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 500000;
  int ret = vrpn_read_available_characters(d_serial_fd, buffer, 3, &timeout);
  if (ret == 2) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_binary_command: Error (%d) from ID %d\n", buffer[0], buffer[1]);
    return false;
  } else if (ret != 3) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_binary_command: Timeout waiting for command status (got %d chars)\n", ret);
    return false;
  }
  if (buffer[0] != 0) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_binary_command: Command failed\n");
    return false;
  }
  if (buffer[1] != d_logical_id) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_binary_command: Got response for incorrect logical ID\n");
    return false;
  }
#ifdef  VERBOSE
  printf("..... send succeded\n");
#endif

  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command
 * ROLE      : 
 * ARGUMENTS : char *cmd : the command to be sent
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Ascii-mode command packet format:
//  '>' (start of packet)
//  ',' and then logical ID (decimal)
//  ',' and then data Command value (decimal)
//  ',' Command data (0 or more bytes of parameters
//  '\n'
// NOTE: In the wireless protocol, all commands are acknowledged, so we
// wait for the response to make sure the command was properly received.
bool vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command (const char *p_cmd)
{
  // Check to make sure we have a non-empty command
  if (strlen(p_cmd) == 0) {
    return false;
  }

  // Allocate space for the longest command plus padding and zero terminator
  char buffer[256];

  // Fill in the command
  sprintf(buffer, ">%d,%s\n", d_logical_id, p_cmd);

  // Send the command and see if it worked.
  int buflen = static_cast<int>(strlen(buffer) + 1);
  unsigned char *bufptr = static_cast<unsigned char *>(
    static_cast<void*>(buffer));
  int l_ret = vrpn_write_characters(d_serial_fd, bufptr, buflen);

  // Tell if sending worked.
  if (l_ret != buflen) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command: Error sending command\n");
    return false;
  }

  // Listen for a response telling whether the command was received.
  // We parse the entire ASCII command response, including the \n at
  // the end, since we aren't programmatically handling any of these
  // in the driver but only letting the user specify them in the
  // reset routine.
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 500000;
  int ret;
  if ((ret = vrpn_read_available_characters(d_serial_fd, bufptr, sizeof(buffer)-1, &timeout)) <= 0) {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command: Timeout waiting for command status\n");
    return false;
  }
  buffer[ret] = '\0';
  if (buffer[0] != '0') {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command: Command failed: response (%s)\n", buffer);
    return false;
  }
  if (buffer[strlen(buffer)-1] != '\n') {
    fprintf(stderr, "vrpn_YEI_3Space_Sensor_Wireless::send_ascii_command: Got ill-formatted response: (%s).\n", buffer);
    return false;
  }

  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::receive_LED_mode_response
 * ROLE      : 
 * ARGUMENTS : struct timeval *timeout; how long to wait, NULL for forever
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
// Single byte: value 0 or 1.
bool vrpn_YEI_3Space_Sensor_Wireless::receive_LED_mode_response (struct timeval *timeout)
{
  unsigned char value;
  int ret = vrpn_read_available_characters (d_serial_fd, &value, sizeof(value), timeout);
  if (ret != sizeof(value)) {
    return false;
  }
  d_LED_mode = value;
  return true;
}

/******************************************************************************
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::receive_LED_values_response
 * ROLE      : 
 * ARGUMENTS : struct timeval *timeout; how long to wait, NULL for forever
 * RETURN    : true on success, false on failure.
 ******************************************************************************/

// Info from YEI 3-Space Sensor User's Manual Wireless 2.0 R21 26 March2014
// http://www.yeitechnology.com/sites/default/files/YEI_3-Space_Sensor_Users_Manual_Wireless_2.0_r21_26Mar2014.pdf
// Binary-mode command packet format:
// Three four-byte float responses, each big-endian.
bool vrpn_YEI_3Space_Sensor_Wireless::receive_LED_values_response (struct timeval *timeout)
{
  unsigned char buffer[3*sizeof(vrpn_float32)];
  int ret = vrpn_read_available_characters (d_serial_fd, buffer, sizeof(buffer), timeout);
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
 * NAME      : vrpn_YEI_3Space_Sensor_Wireless::get_report
 * ROLE      : This function will read characters until it has a full report, then
 *             call handle_report() with the report and clear the counts.  For the
 *             wireless protocol, we need to manually request release of streaming
 *             characters from our logical ID before checking to see if there are
 *             any.
 * ARGUMENTS : void
 * RETURN    : bool: Did we get a complete report?
 ******************************************************************************/
bool vrpn_YEI_3Space_Sensor_Wireless::get_report (void)
{
  int  l_ret;		// Return value from function call to be checked

  //--------------------------------------------------------------------
  // Request release of pending reports from our ID.
  unsigned char release_report[2] = { 0xB4, 0 };
  release_report[1] = d_logical_id;
  if (!send_binary_command_to_dongle(release_report, sizeof(release_report))) {
    VRPN_MSG_ERROR ("vrpn_YEI_3Space::get_report: Unable to send release-report command\n");
    return false;
  }

  //--------------------------------------------------------------------
  // Read a report if it is available.
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 1000;
  l_ret = vrpn_read_available_characters(d_serial_fd, d_buffer,
                                          REPORT_LENGTH + 3,
                                          &timeout);
  if (l_ret == -1) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::get_report(): Error reading the sensor, resetting");
      d_status = STATUS_RESETTING;
      return false;
  }
#ifdef  VERBOSE
  if (l_ret != 0) printf("... got %d characters\n",l_ret);
#endif
  // If we didn't get any reports, we're done and will look again next time.
  if (l_ret == 0) {
    return false;
  }

  // Check to be sure that the length was what we expect, we got no errors
  // (first header byte 0) and the response is from the correct device.
  if (l_ret != REPORT_LENGTH + 3) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::get_report(): Truncated report, resetting");
      d_status = STATUS_RESETTING;
      return false;
  }
  if (d_buffer[0] != 0) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::get_report(): Error reported, resetting");
      d_status = STATUS_RESETTING;
      return false;
  }
  if (d_buffer[1] != d_logical_id) {
      VRPN_MSG_ERROR ("vrpn_YEI_3Space_Sensor_Wireless::get_report(): Report from wrong sensor received, resetting");
      d_status = STATUS_RESETTING;
      return false;
  }

  //--------------------------------------------------------------------
  // Parse and handle the report.
  vrpn_gettimeofday(&timestamp, NULL);
  handle_report(&d_buffer[3]);
  return true;
}

void vrpn_YEI_3Space_Sensor_Wireless::flush_input(void)
{
    vrpn_flush_input_buffer(d_serial_fd);
}
