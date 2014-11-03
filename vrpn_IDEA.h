#ifndef VRPN_IDEA_H
#define VRPN_IDEA_H

//------------------------------------------------------------------------------
// Driver for the Haydon-Kerk IDEA PCM4806X motor controller.
// This assumes that the operating system has provided a virtual COM port
// for the device, so that it can be opened as a serial device.  Both
// Windows 7 and Ubuntu Linux provided this by default as of 8/6/2012.
// This driver does not support the daisy-chained configuration of the
// devices (which is available for RS-485 devices).  If you do not find
// the serial device, you may need to install the device driver for the FTDI
// chipset it uses under Windows.

// See http://www.haydonkerk.com/LinkClick.aspx?fileticket=LEcwYeRmKVg%3d&tabid=331
// for the software manual for this device.

#include "vrpn_Analog.h"                // for vrpn_Serial_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Analog_Output.h"         // for vrpn_Analog_Output
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, VRPN_API
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_RELIABLE, etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_uint32

// XXX Add two buttons to the device, to report limit-switch state.

class VRPN_API vrpn_IDEA: public vrpn_Serial_Analog, public vrpn_Analog_Output,
		public vrpn_Button_Filter
{
public:
	vrpn_IDEA (const char * name, vrpn_Connection * c, const char * port
          , int run_speed_tics_sec = 3200
          , int start_speed_tics_sec = 1200
          , int end_speed_tics_sec = 2000
          , int accel_rate_tics_sec_sec = 40000
          , int decel_rate_tics_sec_sec = 100000
          , int run_current = 290
          , int hold_current = 0
          , int accel_current = 290
          , int decel_current = 290
          , int delay = 50
          , int step = 8      // Microstepping to do; 1/step steps
          , int high_limit_index = -1  // Input index for high limits switch (-1 for none)
          , int low_limit_index = -1   // Input index fro low limit switch (-1 for none)
          , int output_1_setting = -1
          , int output_2_setting = -1
          , int output_3_setting = -1
          , int output_4_setting = -1
          , double initial_move = 0     // Move to one end of travel when reset
          , double fractional_c_a = 1.0 // Use lower accel and current during this move
          , double reset_location = 0.0 // Where to set the value to after reset
        );
	~vrpn_IDEA () {};

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();

  protected:
    unsigned char d_buffer[512];  //< Buffer of characters in report
    unsigned d_bufcount;	      //< How many characters we have so far

    struct timeval  d_timestamp;   //< Time of the last report from the device

    int d_run_speed_tics_sec;
    int d_start_speed_tics_sec;
    int d_end_speed_tics_sec;
    int d_accel_rate_tics_sec_sec;
    int d_decel_rate_tics_sec_sec;
    int d_run_current;
    int d_hold_current;
    int d_accel_current;
    int d_decel_current;
    int d_delay;
    int d_step;
    int d_high_limit_index;
    int d_low_limit_index;
    int d_output_1_setting;
    int d_output_2_setting;
    int d_output_3_setting;
    int d_output_4_setting;
    double d_initial_move;
    double d_fractional_c_a;
    double d_reset_location;
    struct timeval d_last_poll;

    virtual int reset(void);      //< Set device back to starting config
    virtual int get_report(void); //< Try to read a report from the device

    /// Appends carriage-return and then sends the command.
    // Returns true if all characters could be sent.  Returns false
    // on failure.
    bool send_command(const char *cmd);

    /// Request a move from the motor to the specified location.
    // Scale the acceleration and current values for the move by the
    // specified fraction between 0 and 1.  This lets us execute
    // "gentler" moves for doing things like jamming against the rails,
    // so we don't get stuck.
    bool send_move_request(vrpn_float64 location_in_steps, double scale = 1.0);

    /// Send a move request and then wait for the move to complete.  Repeat
    // the command asking the motor to move until it says that we are no
    // longer moving.
    bool move_until_done_or_error(vrpn_float64 location_in_steps, double scale = 1.0);

    /// Parses a position report.  Returns -1 on failure, 0 on no value
    // found, and 1 on value found.  Store the result into our analog channel 0.
    int convert_report_to_position(unsigned char *buf);

    /// Parses an input/output  report.  Returns -1 on failure, 0 on no value
    // found, and 1 on value found.  Store the results of our input reads into
    // buttons 0-3.
    int convert_report_to_buttons(unsigned char *buf);

    /// send report iff changed
    virtual void report_changes
               (vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);
    /// send report whether or not changed
    virtual void report
               (vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);

    /// Responds to a request to change one of the values by
    /// setting the channel to that value.
    static int VRPN_CALLBACK handle_request_message(void *userdata, vrpn_HANDLERPARAM p);

    /// Responds to a request to change multiple channels at once.
    static int VRPN_CALLBACK handle_request_channels_message(void *userdata, vrpn_HANDLERPARAM p);

    /// Responds to a connection request with a report of the values
    static int VRPN_CALLBACK handle_connect_message(void *userdata, vrpn_HANDLERPARAM p);
};

#endif
