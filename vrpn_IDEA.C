// vrpn_IDEA.C

// See http://www.haydonkerk.com/LinkClick.aspx?fileticket=LEcwYeRmKVg%3d&tabid=331
// for the software manual for this device.

#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, stderr, sprintf, etc
#include <string.h>                     // for NULL, strlen, strchr, etc

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_IDEA.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_unbuffer, etc

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

vrpn_IDEA::vrpn_IDEA (const char * name, vrpn_Connection * c,const char * port
                      , int run_speed_tics_sec
                      , int start_speed_tics_sec
                      , int end_speed_tics_sec
                      , int accel_rate_tics_sec_sec
                      , int decel_rate_tics_sec_sec
                      , int run_current
                      , int hold_current
                      , int accel_current
                      , int decel_current
                      , int delay
                      , int step
                      , int high_limit_index
                      , int low_limit_index
                      , int output_1_setting
                      , int output_2_setting
                      , int output_3_setting
                      , int output_4_setting
                      , double initial_move
                      , double fractional_c_a):
	vrpn_Serial_Analog(name, c, port, 57600)
        , vrpn_Analog_Output(name, c)
        , d_run_speed_tics_sec(run_speed_tics_sec)
        , d_start_speed_tics_sec(start_speed_tics_sec)
        , d_end_speed_tics_sec(end_speed_tics_sec)
        , d_accel_rate_tics_sec_sec(accel_rate_tics_sec_sec)
        , d_decel_rate_tics_sec_sec(decel_rate_tics_sec_sec)
        , d_run_current(run_current)
        , d_hold_current(hold_current)
        , d_accel_current(accel_current)
        , d_decel_current(decel_current)
        , d_delay(delay)
        , d_step(step)
        , d_high_limit_index(high_limit_index)
        , d_low_limit_index(low_limit_index)
        , d_output_1_setting(output_1_setting)
        , d_output_2_setting(output_2_setting)
        , d_output_3_setting(output_3_setting)
        , d_output_4_setting(output_4_setting)
        , d_initial_move(initial_move)
        , d_fractional_c_a(fractional_c_a)
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

// Helper function to scale int by a double and get an int.
static inline int scale_int(int val, double scale)
{
  return static_cast<int>(val*scale);
}

//   Commands		  Responses	           Meanings
//    M                     None                      Move to position
//    i                     None                      Interrupt configure
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
//  scale: Between 0 and 1.  Scales the acceleration and currents down.

bool  vrpn_IDEA::send_move_request(vrpn_float64 location_in_steps, double scale)
{
  char  cmd[512];

  //-----------------------------------------------------------------------
  // Configure input interrupts.  We want a falling-edge trigger for the
  // inputs, calling the appropriate subroutine.  We only enable the
  // high limit switch when moving forward and only the low one when moving
  // backwards.  If neither limit switch is set, we don't ever need to
  // send anything.
  if ( (d_high_limit_index > 0) || (d_low_limit_index > 0) ) {
    int edge_masks[4] = { 0, 0, 0, 0 };
    int address_masks[4] = { 0, 0, 0, 0 };
    int priority_masks[4] = { 0, 0, 0, 0 };

    // If we're moving forward and there is a high limit switch, set
    // up to use it.
    if ( (location_in_steps > channel[0]) && (d_high_limit_index > 0) ) {
      edge_masks[d_high_limit_index - 1] = 2;       // Falling edge
      address_masks[d_high_limit_index - 1] = 40;   // Address of program
    }

    // If we're moving backwards and there is a low limit switch, set
    // up to use it.
    if ( (location_in_steps > channel[0]) && (d_low_limit_index > 0) ) {
      edge_masks[d_low_limit_index - 1] = 2;        // Falling edge
      address_masks[d_low_limit_index - 1] = 80;    // Address of program
    }

    if (sprintf(cmd, "i,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
      edge_masks[0], edge_masks[1], edge_masks[2], edge_masks[3],
      address_masks[0], address_masks[1], address_masks[2], address_masks[3],
      priority_masks[0], priority_masks[1], priority_masks[2], priority_masks[3]
    ) <= 0) {
      IDEA_ERROR("vrpn_IDEA::send_move_request(): Could not configure interrupt command");
      status = STATUS_RESETTING;
      return false;
    }
    if (!send_command(cmd)) {
      IDEA_ERROR("vrpn_IDEA::send_move_request(): Could not configure interrupts");
      status = STATUS_RESETTING;
      return false;
    }        
  }

  long steps_64th = static_cast<long>(location_in_steps*64);
  sprintf(cmd, "M%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    steps_64th,
    d_run_speed_tics_sec,
    scale_int(d_start_speed_tics_sec,scale),
    scale_int(d_end_speed_tics_sec, scale),
    scale_int(d_accel_rate_tics_sec_sec, scale),
    scale_int(d_decel_rate_tics_sec_sec, scale),
    scale_int(d_run_current, scale),
    scale_int(d_hold_current, scale),
    scale_int(d_accel_current, scale),
    scale_int(d_decel_current, scale),
    d_delay,
    d_step);
  return send_command(cmd);
}

// This routine will reset the IDEA drive.
//   Commands		  Responses	            Meanings
//    l                     `l<value>[cr]`l#[cr]      Location of the drive

bool  vrpn_IDEA::convert_report_to_value(unsigned char *buf, vrpn_float64 *value)
{
  // Make sure that the last character is [cr] and that we
  // have another [cr] in the record somewhere (there should be
  // two).  This makes sure that we have a complete report.
  char *firstindex = strchr((char*)(buf), '\r');
  char *lastindex = strrchr((char*)(buf), '\r');
  if (buf[strlen((char*)(buf))-1] != '\r') { return false; }
  if (firstindex == lastindex) { return false; }

  // See if we can convert the number.
  int  data;
  if (sscanf((char *)(buf), "`l%d\r`l#\r", &data) != 1) {
    char msg[2048];
    sprintf(msg, "Could not convert value: %s", (char*)(buf));
    IDEA_ERROR(msg);
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
//    A                     None                      Abort
//    P                     `P[program size][cr]`P#[cr] (or none) Program
//    L                     None                      Goto If
//    :                     `:[value][cr]`:#[cr]      Read I/O
//    O                     None                      Set output
//    Z                     None                      Set position value
//    o                     `oYES[cr]`o#[cr] (or NO)  Is the motor moving?

int	vrpn_IDEA::reset(void)
{
	struct	timeval	timeout;
	unsigned char	inbuf[128];
        char            cmd[512];
	int	        ret;

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
        // Set the outputs to the desired state.  If a setting is -1, then we
        // don't change the value.  If it is 0 or 1 then we set the value to
        // what was requested.
        int value = 0;
        if (d_output_1_setting >= 0) {
          value |= 1 << (4 + 0);
          value |= (d_output_1_setting != 0) << (0);
        }
        if (d_output_2_setting >= 0) {
          value |= 1 << (4 + 1);
          value |= (d_output_2_setting != 0) << (1);
        }
        if (d_output_3_setting >= 0) {
          value |= 1 << (4 + 2);
          value |= (d_output_3_setting != 0) << (2);
        }
        if (d_output_4_setting >= 0) {
          value |= 1 << (4 + 3);
          value |= (d_output_4_setting != 0) << (3);
        }
        if (sprintf(cmd, "O%d", value) <= 0) {
          IDEA_ERROR("vrpn_IDEA::send_move_request(): Could not configure output command");
          status = STATUS_RESETTING;
          return -1;
        }
        if (!send_command(cmd)) {
          IDEA_ERROR("vrpn_IDEA::send_move_request(): Could not configure outputs");
          status = STATUS_RESETTING;
          return -1;
        }
	//printf("XXX Sending output command: %s\n", cmd);
        vrpn_SleepMsecs(100);

	//-----------------------------------------------------------------------
	// Read the input/output values from the drive (debugging).  This is how
	// we'd read them to fill in values for the buttons if we want to do that.
        if (!send_command(":")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not request I/O status\n");
          return -1;
        }

        timeout.tv_sec = 0;
	timeout.tv_usec = 30000;

        ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
        if (ret < 0) {
          perror("vrpn_IDEA::reset(): Error reading I/O status from device");
	  return -1;
        }
        if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
          inbuf[ret] = '\0';
          fprintf(stderr,"vrpn_IDEA::reset(): Bad I/O status report (length %d): %s\n", ret, inbuf);
          IDEA_ERROR("Bad I/O status report");
          return -1;
        }
        inbuf[ret] = '\0';

        int io_status;
        if (sscanf((char *)(inbuf), "`:%d\r`:#\r", &io_status) != 1) {
          fprintf(stderr,"vrpn_IDEA::reset(): Bad I/O status report: %s\n", inbuf);
          IDEA_ERROR("Bad I/O status report");
          return -1;
        }
	//printf("XXX I/O status: %02x\n", io_status);

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
        // If we are using the high-limit index, we should abort a move when
        // the drive is moving in a positive direction and we get a falling
        // edge trigger on the associated input.  If the index of the input
        // to use is -1, we aren't using this.  The motion command will set
        // the interrupt handlers appropriately, given the direction of travel.
        // Here, we write the programs for the high limit switch and the low
        // limit switch that will be called when they are triggered.

        if (d_high_limit_index > 0) {

          // First, we write the program that will be run when the high limit
          // switch goes low.  The program will abort the move.

          // The program name must be exactly ten characters.
          // We put this program at memory location 40 (must be multiple of 4?).
          if (!send_command("Phighlimit_,40,1")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not start high limit program\n");
            return -1;
          }

          // The program should cause an abort instruction when run
          if (!send_command("A")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not send abort to high limit program\n");
            return -1;
          }

          // The program description is done
          if (!send_command("P")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not finish high limit program\n");
            return -1;
          }

          timeout.tv_sec = 0;
	  timeout.tv_usec = 300000;

          // Get a response saying that the program has been received; it will
          // say what the size of the program is.
          ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
          if (ret < 0) {
            perror("vrpn_IDEA::reset(): Error reading high-limit program response from device");
	    return -1;
          }
          if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
            inbuf[ret] = '\0';
            fprintf(stderr,"vrpn_IDEA::reset(): Bad high-limit program response report: %s\n", inbuf);
            IDEA_ERROR("Bad position report");
            return -1;
          }
          inbuf[ret] = '\0';

          int program_length;
          if (sscanf((char *)(inbuf), "`P%d\r`P#\r", &program_length) != 1) {
            fprintf(stderr,"vrpn_IDEA::reset(): Bad high program report: %s\n", inbuf);
            IDEA_ERROR("Bad high program report");
            return -1;
          }
        }

        if (d_low_limit_index > 0) {

          // Next, we write the program that will be run when the low limit
          // switch goes low.  The program will abort the move.

          // The program name must be exactly ten characters.
          // We put this program at memory location 80 (must be multiple of 4?).
          if (!send_command("Plow_limit_,80,1")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not start low limit program\n");
            return -1;
          }

          // The program should cause an abort instruction when run
          if (!send_command("A")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not send abort to low limit program\n");
            return -1;
          }

          // The program description is done
          if (!send_command("P")) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not finish low limit program\n");
            return -1;
          }

          timeout.tv_sec = 0;
	  timeout.tv_usec = 300000;

          // Get a response saying that the program has been received; it will
          // say what the size of the program is.
          ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
          if (ret < 0) {
            perror("vrpn_IDEA::reset(): Error reading low-limit program response from device");
	    return -1;
          }
          if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
            inbuf[ret] = '\0';
            fprintf(stderr,"vrpn_IDEA::reset(): Bad low-limit program response report: %s\n", inbuf);
            IDEA_ERROR("Bad low-limit program response report");
            return -1;
          }
          inbuf[ret] = '\0';

          int program_length;
          if (sscanf((char *)(inbuf), "`P%d\r`P#\r", &program_length) != 1) {
            fprintf(stderr,"vrpn_IDEA::reset(): Bad low program report: %s\n", inbuf);
            IDEA_ERROR("Bad low program report");
            return -1;
          }
        }

	//-----------------------------------------------------------------------
        // If we have an initial-move value (to run into the rails), then execute
        // a move with acceleration and currents scaled by the fractional_c_a
        // value used in the constructor.  Wait until the motor stops moving
        // after this command before proceeding.

        if (d_initial_move != 0) {

          // Send a move command, scaled by the fractional current and
          // acceleration values.
          if (!send_move_request(d_initial_move, d_fractional_c_a)) {
            fprintf(stderr,"vrpn_IDEA::reset(): Could not do initial move\n");
            IDEA_ERROR("Could not do initial move");
            return -1;
          }

          // Keep asking whether the motor is moving until it says that it
          // is not.
          bool moving = true;
          do {
            if (!send_command("o")) {
              fprintf(stderr,"vrpn_IDEA::reset(): Could not request movement status\n");
              IDEA_ERROR("Could not request movement status");
              return -1;
            }

            timeout.tv_sec = 0;
	    timeout.tv_usec = 30000;
            ret = vrpn_read_available_characters(serial_fd, inbuf, sizeof(inbuf), &timeout);
            if (ret < 0) {
              perror("vrpn_IDEA::reset(): Error reading movement status from device");
              IDEA_ERROR("Error reading movement status");
	      return -1;
            }
            if ( (ret < 8) || (inbuf[ret-1] != '\r') ) {
              inbuf[ret] = '\0';
              fprintf(stderr,"vrpn_IDEA::reset(): Bad movement status report (length %d): %s\n", ret, inbuf);
              IDEA_ERROR("Bad movement status report");
              return -1;
            }
            inbuf[ret] = '\0';

            if ( (inbuf[0] != '`') || (inbuf[1] != 'o') ) {
              fprintf(stderr,"vrpn_IDEA::reset(): Bad movement status report: %s\n", inbuf);
              IDEA_ERROR("Bad movement status report");
              return -1;
            }
            moving = (inbuf[2] == 'Y');
          } while (moving);
        }

	//-----------------------------------------------------------------------
        // Reset the drive count at the present location to zero.
        if (!send_command("Z0")) {
          fprintf(stderr,"vrpn_IDEA::reset(): Could not set position to 0\n");
          return -1;
        }

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
   d_buffer[d_bufcount] = '\0';
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
