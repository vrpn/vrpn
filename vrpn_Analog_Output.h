// vrpn_Analog_Output.h
// David Borland, September 2002
//
// These classes are for setting values for an analog output device.  The vrpn_Analog was
// getting overloaded by trying to have functionality for both reading and writing in it.
// If wanting to read analog values from a device, a vrpn_Analog should be used, if 
// wanting ro write analog values to a device, a vrpn_Analog_Output should be used.  
// This is similar to the Tracker/Poser dichotomy.

#ifndef VRPN_ANALOG_OUTPUT_H
#define VRPN_ANALOG_OUTPUT_H

#include "vrpn_Analog.h"    // just for some #define's and constants

// Similar to vrpn_Analog, but messages are different
// Members beginning with o_ are also found in vrpn_Analog, the o_ is 
// so that you can derive a class from both without getting ambiguities
class vrpn_Analog_Output : public vrpn_BaseClass {
    public:
        vrpn_Analog_Output(const char* name, vrpn_Connection* c = NULL);

        // Print the status of the analog output device
        void o_print(void);

    protected:
	    vrpn_float64    o_channel[vrpn_CHANNEL_MAX];
	    vrpn_int32	    o_num_channel;
	    struct timeval  o_timestamp;
	    vrpn_int32	    request_m_id;	        //< Request to change message from client
            vrpn_int32      request_channels_m_id;	//< Request to change channels message from client
	    int             o_status; 

	    virtual	int register_types(void);

	    //------------------------------------------------------------------
	    // Routines used to send data from the client
	    virtual vrpn_int32 encode_to(char *buf);
};

// A *Sample* Analog output server.  Use this, or derive your own server
// from vrpn_Analog_Output with this as a guide

class vrpn_Analog_Output_Server : public vrpn_Analog_Output {
    public:
        vrpn_Analog_Output_Server(const char* name, vrpn_Connection* c);
        virtual ~vrpn_Analog_Output_Server(void);

        virtual void mainloop() { server_mainloop(); }

        /// Sets the size of the array;  returns the size actually set.
        /// (May be clamped to vrpn_CHANNEL_MAX)
        vrpn_int32 setNumChannels (vrpn_int32 sizeRequested);

        /// Responds to a request to change one of the values by
        /// setting the channel to that value.  Derived class must
	/// either install handlers for this routine or else make
	/// its own routines to handle the request message.
        static int handle_request_message(void *userdata,
	        vrpn_HANDLERPARAM p);

        /// Responds to a request to change a number of channels
	/// Derived class must either install handlers for this
	/// routine or else make its own routines to handle the
	/// multi-channel request message.
        static int handle_request_channels_message(void* userdata,
            vrpn_HANDLERPARAM p);

	/// Exposes an array of values for the user to read from.
	const vrpn_float64* o_channels (void) const { return o_channel; };
};

// Open an analog output device that is on the other end of a connection
// and send updates to it.  This is the type of analog output device
// that user code will deal with.
class vrpn_Analog_Output_Remote : public vrpn_Analog_Output {
    public:
        // The name of the analog device to connect to
        // Optional argument to be used when the Remote should listen on
        // a connection that is already open.
        vrpn_Analog_Output_Remote(const char* name, vrpn_Connection* c = NULL);
        virtual ~vrpn_Analog_Output_Remote (void);

        // This routine calls the mainloop of the connection it's on
        virtual void mainloop();

        // Request the analog to change its value to the one specified.
        // Returns false on failure.
        virtual	bool request_change_channel_value(unsigned int chan, vrpn_float64 val,
            vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);

        // Request the analog to change values all at once.  If more values are given than we have channels, the 
        // Extra values are discarded.  If less values are given than we have channels, the extra channels are set to 0
        // Returns false on failure
        virtual bool request_change_channels(int num, vrpn_float64* vals,
            vrpn_uint32 class_of_service = vrpn_CONNECTION_RELIABLE);

    protected:
        // Routines used to send requests from the client
        virtual vrpn_int32 encode_change_to(char *buf, vrpn_int32 chan, vrpn_float64 val);
        virtual vrpn_int32 encode_change_channels_to(char* buf, vrpn_int32 num, vrpn_float64* vals);
};

#endif
