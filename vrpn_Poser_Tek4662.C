#include <math.h>                       // for floor
#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for strlen, memcpy

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Connection.h"            // for vrpn_HANDLERPARAM, etc
// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"                // for timeval, vrpn_unbuffer, etc
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_uint16

#ifdef	_WIN32
#ifndef _WIN32_WCE
#include <io.h>
#endif
#endif

#include "vrpn_Poser_Tek4662.h"

//#define VERBOSE

const int vrpn_Poser_Tek4662_FAIL = -1;
const int vrpn_Poser_Tek4662_RESETTING = 0;
const int vrpn_Poser_Tek4662_SYNCING = 1;
const int vrpn_Poser_Tek4662_RUNNING = 2;

// Plotter motion constants
const double COUNTS_PER_METER =  1.0 / ( (15.0 / 4095.0) * ( 0.0254 / 1.0 ) );
const double MAX_X = 0.381;  // Range of the X axis is 15 inches
const double MAX_Y = 0.254;  // Range of the Y axis is 10 inches
const double VELOCITY= (1/0.06144) * (1/0.00254); // Meters per second

// Constants used as characters to communicate to the plotter
const unsigned char ESC = 27;
//const unsigned char BELL = '7';
const unsigned char DEVICE = 'A';
const unsigned char GS = 29;	  //< Puts the plotter into graphics mode
const unsigned char ZERO = 0;
const unsigned char ZEROES[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
const unsigned char PLOTTER_ON[] = { ESC, DEVICE, 'E', ZERO };
const unsigned char RESET[] = { ESC, DEVICE, 'N', ZERO };
const unsigned char GIN[] = { ESC, DEVICE, 'M' };
const unsigned char MOVE_TEMPLATE[] = { GS, 0x20, 0x60, 0x60, 0x20, 0x40 };
const int	    DATA_RECORD_LENGTH = 7;

// Constants used to do bit manipulation
//const unsigned BITFOUR = 1 << 4;
//const unsigned BITFIVE = 1 << 5;
const unsigned LOWFIVEBITS = 0x001f;
const unsigned LOWTWOBITS = 0x0003;

//////////////////////////////////////////////////////////////////////////////////////
// Server Code

vrpn_Poser_Tek4662::vrpn_Poser_Tek4662 (const char *name, vrpn_Connection *c,
					const char *port, int baud, int bits,
					vrpn_SER_PARITY parity) :
    vrpn_Poser(name, c),
    vrpn_Tracker(name, c),
    d_serial_fd(-1),
    d_inbufcounter(0),
    d_new_location_requested(false),
    d_outstanding_requests(0)
{
    // Make sure that we have a valid connection
    if (d_connection == NULL) {
      fprintf(stderr,"vrpn_Poser_Tek4662: No connection\n");
      return;
    }

    // Check the port name;
    if (port == NULL) {
	fprintf(stderr,"vrpn_Poser_Tek4662: NULL port name\n");
	status = vrpn_Poser_Tek4662_FAIL;
	return;
    }

    // Open the serial port we're going to use
    if ( (d_serial_fd=vrpn_open_commport(port, baud, bits, parity)) == -1) {
	fprintf(stderr,"vrpn_Poser_Tek4662: Cannot Open serial port (%s)\n", port);
	status = vrpn_Poser_Tek4662_FAIL;
    }

    // Register a handler for the position change callback for this device
    if (register_autodeleted_handler(req_position_m_id,
		    handle_change_message, this, d_sender_id)) {
      fprintf(stderr,"vrpn_Poser_Server: can't register position handler\n");
      d_connection = NULL;
    }

    // Register a handler for the velocity change callback for this device
    if (register_autodeleted_handler(req_velocity_m_id,
		    handle_vel_change_message, this, d_sender_id)) {
      fprintf(stderr,"vrpn_Poser_Server: can't register velocity handler\n");
      d_connection = NULL;
    }

    // Set up the workspace max and min values
    p_pos_min[0] = 0.0;	  p_pos_max[0] = MAX_X;
    p_pos_min[1] = 0.0;	  p_pos_max[1] = MAX_Y;
    p_pos_min[2] = 0.0;	  p_pos_max[2] = 0.0;	      // There is no Z axis
    p_vel_min[0] = p_vel_max[0] = VELOCITY;	      // Meters per second
    p_vel_min[1] = p_vel_max[1] = VELOCITY;	      // Meters per second
    p_vel_min[2] = p_vel_max[2] = 0.0;		      // No motion in Z
    p_pos_rot_min[0] = 0.0;  p_pos_rot_max[0] = 0.0;  // There is no rotation
    p_pos_rot_min[1] = 0.0;  p_pos_rot_max[1] = 0.0;  // There is no rotation
    p_pos_rot_min[2] = 0.0;  p_pos_rot_max[2] = 0.0;  // There is no rotation
    p_vel_rot_min[0] = 0.0;  p_vel_rot_max[0] = 0.0;  // There is no rotation
    p_vel_rot_min[1] = 0.0;  p_vel_rot_max[1] = 0.0;  // There is no rotation
    p_vel_rot_min[2] = 0.0;  p_vel_rot_max[2] = 0.0;  // There is no rotation

    // Reset the device and find out what time it is
    status = vrpn_Poser_Tek4662_RESETTING;
    vrpn_gettimeofday(&timestamp, NULL);
}


vrpn_Poser_Tek4662::~vrpn_Poser_Tek4662()
{
    // Close com port when destroyed. 
    if (d_serial_fd != -1) {
        vrpn_close_commport(d_serial_fd);
    }
}

// This parses the pen position and location from a GIN report.
// It first verifies that the high-order tag bits match what is
// expected on all bytes (see page 2-27 of the manual).  If they
// do not, it returns false; otherwise, true.
static bool interpret_GIN_bytes(const unsigned char inbuf[], bool &pen_down, float &x, float &y)
{
    // Check the high-order tag bits to make sure they are valid.
    for (int i = 0; i < 6; i++) {
      if ( (inbuf[i] & 0x60) != 0x20) { return false; }
    }
    if ( (inbuf[6] & 0x60) != 0x40) { return false; }

    // Interpret the pen-down bit.
    pen_down = (inbuf[6] & (1<<2)) != 0;

    // Unpack the X and Y coordinates from the plotter report as described on
    // page 2-27 of the manual.  Note that in GIN mode, the plotter only uses
    // the higher-order 12 bits; the lower 4 are set to zero.  This means that
    // we shift three bit past the right and ignore the lowest bit.
    vrpn_uint16	x_int, y_int;
    x_int = static_cast<unsigned short>(((inbuf[0] & LOWFIVEBITS) << 7) | ((inbuf[2] & LOWFIVEBITS) << 2) | ((inbuf[4] & LOWFIVEBITS) >> 3));
    y_int = static_cast<unsigned short>(((inbuf[1] & LOWFIVEBITS) << 7) | ((inbuf[3] & LOWFIVEBITS) << 2) | ((inbuf[5] & LOWFIVEBITS) >> 3));

    // Convert the position from counts to meters.  This goes through inches, which is the native
    // plotter unit.
    x = (float)( x_int / COUNTS_PER_METER );
    y = (float)( y_int / COUNTS_PER_METER );

#if 0
    if (pen_down) { printf("XXX pen down\n"); } else { printf("XXX pen up\n"); }
    printf("XXX At %f, %f\n", x, y);
#endif
    return true;
}

void vrpn_Poser_Tek4662::reset()
{
  // Wait a little and then flush the input buffer so we don't get extra reports
  // from before the reset.
  vrpn_SleepMsecs(100);
  vrpn_flush_input_buffer( d_serial_fd );

  // Send a bunch of zeroes to clear out the input buffer,
  // then a "Plotter on", then a "Reset" to the plotter.
  vrpn_flush_output_buffer( d_serial_fd );
  vrpn_write_characters( d_serial_fd, ZEROES, sizeof(ZEROES) );
  vrpn_write_characters( d_serial_fd, PLOTTER_ON, strlen((const char*)PLOTTER_ON) );
  vrpn_write_characters( d_serial_fd, RESET, strlen((const char*)RESET) );
  vrpn_drain_output_buffer( d_serial_fd );

  // Request a position message from the plotter and then wait
  // until it responds.  Make sure we get a good response.  If
  // so, then send a Tracker message with the specified position
  // and go into SYNCING mode.  If not, then reset again.
  vrpn_write_characters( d_serial_fd, GIN, strlen((const char*)GIN) );
  vrpn_drain_output_buffer( d_serial_fd );
  unsigned  char  inbuf[DATA_RECORD_LENGTH];
  struct timeval wait_time = { 1, 0 };
  int bufcount = vrpn_read_available_characters( d_serial_fd, inbuf, sizeof(inbuf), &wait_time);
  if (bufcount != sizeof(inbuf)) {
    fprintf(stderr,"vrpn_Poser_Tek4662::reset(): Expected %d characters, got %d\n",
	static_cast<int>(sizeof(inbuf)), bufcount);
  } else {
    // Parse the input to find our position and store it in the tracker
    // position.
    float   x,y;
    bool    pen_down;

    vrpn_gettimeofday(&timestamp, NULL);
    if (!interpret_GIN_bytes(inbuf, pen_down, x, y)) {
      send_text_message("vrpn_Poser_Tek4662: Error resetting", timestamp, vrpn_TEXT_ERROR);
      return;
    } else {
      send_text_message("vrpn_Poser_Tek4662: Reset correctly", timestamp, vrpn_TEXT_ERROR);
    }

    // Set and send tracker position.
    pos[0] = x; pos[1] = y; pos[2] = 0;
    d_quat[0] = d_quat[1] = d_quat[2] = 0; d_quat[3] = 1;
    if (d_connection) {
      char	msgbuf[1000];
      int	len = vrpn_Tracker::encode_to(msgbuf);
      if (d_connection->pack_message(len, timestamp,
	      position_m_id, d_sender_id, msgbuf,
	      vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"vrpn_Poser_Tek4662: cannot write message: tossing\n");
      }
    } else {
	fprintf(stderr,"vrpn_Poser_Tek4662: No valid connection\n");
    }

    // We're waiting for the first character!
    d_outstanding_requests = 0;
    status = vrpn_Poser_Tek4662_SYNCING;
  }
}

void vrpn_Poser_Tek4662::run()
{
  struct timeval  now;

  // Send new positions as needed, in coordination with the callback handler.
  if ((d_outstanding_requests == 0) && d_new_location_requested) {

    // Figure out the integer location corresponding to the new poser
    // position requested.  We've already tested to make sure that the
    // position is in bounds.
    vrpn_uint16	x_int, y_int;
    x_int = (vrpn_uint16)floor( p_pos[0] * COUNTS_PER_METER );
    y_int = (vrpn_uint16)floor( p_pos[1] * COUNTS_PER_METER );

    // Send a command to the plotter to move to the new location.
    // Request the new location be reported.
    // The first two characters tell it to go into graph mode; the
    // last five include the correct upper-two bits for each byte,
    // with the rest filled in as described on page 2-26 in the manual.
    unsigned  char  MOVE[sizeof(MOVE_TEMPLATE)];
    memcpy(MOVE, MOVE_TEMPLATE, sizeof(MOVE));
    // Five MSB of y into 5 LSB
    MOVE[1] |= y_int >> 7;
    // Two LSB of y into bits 2-3, two LSB of x into bits 0-1
    MOVE[2] |= (x_int & LOWTWOBITS);
    MOVE[2] |= (y_int & LOWTWOBITS) << 2;
    // The rest of the Y bits (five in the middle) to LSB
    MOVE[3] |= (y_int >> 2) & LOWFIVEBITS;
    // High-order x bits into low-order bits
    MOVE[4] |= x_int >> 7;
    // Intermediate X bits into lower-order bits
    MOVE[5] |= (x_int >> 2) & LOWFIVEBITS;
    vrpn_write_characters(d_serial_fd, MOVE, sizeof(MOVE));
    vrpn_write_characters(d_serial_fd, GIN, strlen((const char*)GIN) );

    // Record the fact that we're moving so that we won't send a new
    // command until the move completes.
#if 0
    printf("XXX Going to %f,%f (%d, %d)\n", p_pos[0], p_pos[1], x_int, y_int);
#endif
    d_outstanding_requests++;
    d_new_location_requested = false;
  }

  // Listen for any new reports from the device.  Recall that we can get
  // partial results reported, especially with a 1200-baud or slower serial
  // connection like we have on this device.
  // This assumes that the plotter does not tell us where it is until
  // it finishes moving there.  This assumption looks valid from short
  // tests.
  if (status == vrpn_Poser_Tek4662_SYNCING) { // Try to get first byte
    // Zero timeout, poll for any available characters
    struct timeval timeout = {0, 0};
    if (1 == vrpn_read_available_characters(d_serial_fd, d_inbuf, 1, &timeout)) {
      d_inbufcounter = 1;   //< Ignore the status byte for the following record
      status = vrpn_Poser_Tek4662_RUNNING;
      vrpn_gettimeofday(&timestamp, NULL);
    } else {
      d_inbufcounter = 0;
    }
  }
  if (status == vrpn_Poser_Tek4662_RUNNING) {
    // Zero timeout, poll for any available characters
    struct timeval timeout = {0, 0};
    int result = vrpn_read_available_characters(d_serial_fd, 
		  &d_inbuf[d_inbufcounter], DATA_RECORD_LENGTH-d_inbufcounter, &timeout);    

    if (result < 0) {
      send_text_message("vrpn_Poser_Tek4662: Error reading", timestamp, vrpn_TEXT_ERROR);
      status = vrpn_Poser_Tek4662_RESETTING;
    } else {
      d_inbufcounter += result;
      if (d_inbufcounter == DATA_RECORD_LENGTH) {
	d_inbufcounter = 0;
	status = vrpn_Poser_Tek4662_SYNCING;

	// Parse the input to find our position and store it in the tracker
	// position.
	float   x,y;
	bool    pen_down;

	if (!interpret_GIN_bytes(d_inbuf, pen_down, x, y)) {
	  send_text_message("vrpn_Poser_Tek4662: Error parsing position", timestamp, vrpn_TEXT_ERROR);
	  return;
	}

	// Set and send tracker position.
	pos[0] = x; pos[1] = y; pos[2] = 0;
	d_quat[0] = d_quat[1] = d_quat[2] = 0; d_quat[3] = 1;
	if (d_connection) {
	  char	msgbuf[1000];
	  int	len = vrpn_Tracker::encode_to(msgbuf);
	  if (d_connection->pack_message(len, timestamp,
		  position_m_id, d_sender_id, msgbuf,
		  vrpn_CONNECTION_LOW_LATENCY)) {
	    fprintf(stderr,"vrpn_Poser_Tek4662: cannot write message: tossing\n");
	  }
	} else {
	    fprintf(stderr,"vrpn_Poser_Tek4662: No valid connection\n");
	}
	d_outstanding_requests--;
      }
    }
  }

  // Request the position four times per second when we're outside of
  // the position command.  Remember to increment the number of outstanding
  // requests so the position-request code keeps working.  This will let
  // the user move the plotter around with the joystick and have the
  // tracker follow around.  It will also keep sending reports so that a
  // client the connects after reset will know where the plotter is fairly
  // quickly.

  if ((d_outstanding_requests == 0) && !d_new_location_requested) {
    vrpn_gettimeofday(&now, NULL);
    if (vrpn_TimevalDuration(now, timestamp) > 250000L) {
      // Record the fact that we're asking so that we won't send a new
      // command until the response completes.
      vrpn_write_characters(d_serial_fd, GIN, strlen((const char*)GIN) );
      d_outstanding_requests++;
    }
  }

  // We need a watchdog timer to make sure that the plotter doesn't just
  // die on us in the middle of a move.  If we don't hear from it for 5 seconds,
  // reset it.
  vrpn_gettimeofday(&now, NULL);
  if (vrpn_TimevalDuration(now, timestamp) > 5000000L) {
    send_text_message("vrpn_Poser_Tek4662: Device timeout (resetting)", now, vrpn_TEXT_ERROR);
    status = vrpn_Poser_Tek4662_RESETTING;
    return;
  }
}
    
void vrpn_Poser_Tek4662::mainloop()
{
    // Call the generic server mainloop routine, since this is a server
    server_mainloop();

    // Depending on what mode we're in, do our thing.

    switch (status) {
      case vrpn_Poser_Tek4662_RESETTING:
	 reset();
	 break;

      case vrpn_Poser_Tek4662_SYNCING:
      case vrpn_Poser_Tek4662_RUNNING:
	 run();
	 break;

      case vrpn_Poser_Tek4662_FAIL:
	 break;

      default:
	fprintf(stderr,"vrpn_Poser_Tek4662: Unknown status (%d)\n", status);
	status = vrpn_Poser_Tek4662_FAIL;
	break;
    }
}

int vrpn_Poser_Tek4662::handle_change_message(void* userdata,
	    vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Tek4662* me = (vrpn_Poser_Tek4662 *)userdata;
    const char* params = (p.buffer);
    int	i;
    // Fill in the parameters to the poser from the message
    if (p.payload_len != (7 * sizeof(vrpn_float64)) ) {
	    fprintf(stderr,"vrpn_Poser_Server: change message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    p.payload_len, static_cast<int>(7 * sizeof(vrpn_float64)) );
	    return -1;
    }
    me->p_timestamp = p.msg_time;

    for (i = 0; i < 3; i++) {
	    vrpn_unbuffer(&params, &me->p_pos[i]);
    }
    for (i = 0; i < 4; i++) {
	    vrpn_unbuffer(&params, &me->p_quat[i]);
    }

    // Check the pose against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_pos[i] < me->p_pos_min[i]) {
            me->p_pos[i] = me->p_pos_min[i];
        }
        else if (me->p_pos[i] > me->p_pos_max[i]) {
            me->p_pos[i] = me->p_pos_max[i];
        }
    }

    // Set up so that run() will move the plotter to the requested location.
    me->d_new_location_requested = true;

    return 0;
}

int vrpn_Poser_Tek4662::handle_vel_change_message(void* userdata,
	    vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Tek4662* me = (vrpn_Poser_Tek4662*)userdata;
    const char* params = (p.buffer);
    int	i;

    // Fill in the parameters to the poser from the message
    if (p.payload_len != (8 * sizeof(vrpn_float64)) ) {
	    fprintf(stderr,"vrpn_Poser_Server: velocity message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    p.payload_len, static_cast<int>(8 * sizeof(vrpn_float64)) );
	    return -1;
    }
    me->p_timestamp = p.msg_time;

    for (i = 0; i < 3; i++) {
	    vrpn_unbuffer(&params, &me->p_vel[i]);
    }
    for (i = 0; i < 4; i++) {
	    vrpn_unbuffer(&params, &me->p_vel_quat[i]);
    }
    vrpn_unbuffer(&params, &me->p_vel_quat_dt);

    // Check the velocity against the max and min values of the workspace
    for (i = 0; i < 3; i++) {
        if (me->p_vel[i] < me->p_vel_min[i]) {
            me->p_vel[i] = me->p_vel_min[i];
        }
        else if (me->p_vel[i] > me->p_vel_max[i]) {
            me->p_vel[i] = me->p_vel_max[i];
        }
    }

    // No response to this message.
    return 0;
}
