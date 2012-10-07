#ifndef VRPN_BIOSCIENCES_H
#define VRPN_BIOSCIENCES_H

// XXX Need to have a VRPN boolean output device.  Then we can set the
// temperature control to be on or off based on its value.  For now, we're
// doing a horrible thing and packing it into an analog output channel.

#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#include "vrpn_Analog_Output.h"

class VRPN_API vrpn_BiosciencesTools: public vrpn_Serial_Analog,
  public vrpn_Analog_Output, public vrpn_Button_Filter
{
public:
        // Tell it the temperature to use to set channels 1 and 2 to
        // in Celcius and also whether to turn the temperature control on.
	vrpn_BiosciencesTools (const char * name, vrpn_Connection * c,
			const char * port, float temp1, float temp2,
                        bool control_on);
	~vrpn_BiosciencesTools () {};

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();

  protected:
	unsigned d_expected_chars;    //< How many characters to expect in the report
	char d_buffer[128];           //< Buffer of characters in report
	unsigned d_bufcount;	      //< How many characters we have so far

	struct timeval timestamp;   //< Time of the last report from the device

	virtual int reset(void);		//< Set device back to starting config
	virtual	int get_report(void);		//< Try to read a report from the device

        // Channels are zero-referenced.  Use 0 for channel 1.
        bool  set_reference_temperature(unsigned channel, float value);
        bool  set_control_status(bool on);
        bool  request_temperature(unsigned channel);

        // Sets a specified channel based on a new value from the Analog_Output.
        // Channels 0 and 1 are temperature settings, and channel 2 is our 
        // hack to turn on and off temperatur control.
        bool  set_specified_channel(unsigned channel, vrpn_float64 value);

        // This lets us know which channel we're waiting for a reading on.
        // It cycles; each time we hear from one, we ask for the next.
        unsigned  d_next_channel_to_read;

	float  convert_bytes_to_reading(const char *buf);

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
