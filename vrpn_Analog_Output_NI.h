// vrpn_Analog_Output_NI.h
// Russell Taylor, May 2004
//

#ifndef VRPN_ANALOG_OUTPUT_NI_H
#define VRPN_ANALOG_OUTPUT_NI_H

#include "vrpn_Analog_Output.h"

// An Analog output server that uses National Instruments cards to do its
// output.

class VRPN_API vrpn_Analog_Output_Server_NI : public vrpn_Analog_Output {
public:
    vrpn_Analog_Output_Server_NI(const char* name, vrpn_Connection* c, 
			     const char *boardName = "PCI-6713",
			     vrpn_int32 numChannels = vrpn_CHANNEL_MAX,
			     bool bipolar = false,
			     double minVoltage = 0.0,
			     double maxVoltage = 10.0);
    virtual ~vrpn_Analog_Output_Server_NI(void);

    virtual void mainloop();

protected:
    int NI_device_number;	//< National Instruments device to use
    int NI_num_channels;	//< Number of channels on the board
    double	min_voltage;	//< Minimum voltage allowed on a channel
    double	max_voltage;	//< Maximum voltate allowed on a channel
    int		polarity;	//< Polarity (1 = unipolar, 0 = bipolar)

    /// Sets the size of the array;  returns the size actually set.
    /// (May be clamped to vrpn_CHANNEL_MAX)
    /// This should be used before mainloop is ever called.
    vrpn_int32 setNumChannels (vrpn_int32 sizeRequested);

    /// Responds to a request to change one of the values by
    /// setting the channel to that value.  Derived class must
    /// either install handlers for this routine or else make
    /// its own routines to handle the request message.
    static int VRPN_CALLBACK handle_request_message( void *userdata,
				      vrpn_HANDLERPARAM p );

    /// Responds to a request to change a number of channels
    /// Derived class must either install handlers for this
    /// routine or else make its own routines to handle the
    /// multi-channel request message.
    static int VRPN_CALLBACK handle_request_channels_message( void* userdata,
					       vrpn_HANDLERPARAM p);

    /// Used to notify us when a new connection is requested, so that
    /// we can let the client know how many channels are active
    static int VRPN_CALLBACK handle_got_connection( void* userdata, vrpn_HANDLERPARAM p );

    virtual bool report_num_channels( vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE );
    virtual vrpn_int32 encode_num_channels_to( char* buf, vrpn_int32 num );
};

#endif
