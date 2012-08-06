#ifndef VRPN_IDEA_H
#define VRPN_IDEA_H

//------------------------------------------------------------------------------
// Driver for the Haydon-Kerk IDEA PCM4806X motor controller.
// This assumes that the operating system has provided a virtual COM port
// for the device, so that it can be opened as a serial device.  Both
// Windows 7 and Ubuntu Linux provided this by default as of 8/6/2012.
// This driver does not support the daisy-chained configuration of the
// devices (which is available for RS-485 devices).  If you do not find
// it immediately, you may need to install the device driver for the FTDI
// chipset it uses under Windows.

#include "vrpn_Analog.h"
#include "vrpn_Analog_Output.h"

// XXX Add two buttons to the device, for the limit switches.

class VRPN_API vrpn_IDEA: public vrpn_Serial_Analog, public vrpn_Analog_Output
{
public:
	vrpn_IDEA (const char * name, vrpn_Connection * c,
			const char * port);
	~vrpn_IDEA () {};

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();

  protected:
	unsigned char d_buffer[512];  //< Buffer of characters in report
	unsigned d_bufcount;	      //< How many characters we have so far

	struct timeval d_timestamp;   //< Time of the last report from the device

	virtual int reset(void);      //< Set device back to starting config
	virtual	int get_report(void); //< Try to read a report from the device

        /// Appends carriage-return and then sends the command.
        // Returns true if all characters could be sent.  Returns false
        // on failure.
        bool send_command(const char *cmd);

        /// Request a move from the motor to the specified location.
        bool send_move_request(vrpn_float64 location_in_steps);

        /// Parses a position report.  Returns false on failure.
	bool convert_report_to_value(unsigned char *buf, vrpn_float64 *value);

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
