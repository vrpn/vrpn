#ifndef VRPN_ANALOG
#define VRPN_ANALOG
#define vrpn_CHANNEL_MAX 128
// analog status flags
#define ANALOG_SYNCING		(2)
#define ANALOG_REPORT_READY 	(1)
#define ANALOG_PARTIAL 	(0)
#define ANALOG_RESETTING	(-1)
#define ANALOG_FAIL 	 	(-2)

#include "vrpn_Connection.h"

// Contention:  vrpn_Analog shouldn't have a mainloop() function.
//   (Neither should other similar base classes).  There isn't any
//   reason for some derived classes to implement one, and there may
//   not even be any meaningful semantics.  (qv vrpn_Analog_Server below)
// Instead, declare a virtual destructor = 0 to force it to be an abstract
//   class.
// TCH March 1999

class vrpn_Analog {
public:
	vrpn_Analog (const char * name, vrpn_Connection * c = NULL);

	// Print the status of the analog device
	void print(void);

	// Called once through each main loop iteration to handle
	// updates.
	virtual void mainloop (const struct timeval * timeout = NULL) = 0;

	// Report changes to conneciton
        vrpn_Connection *connectionPtr();

  protected:
	vrpn_Connection *connection;
	vrpn_float64	channel[vrpn_CHANNEL_MAX];
	vrpn_float64	last[vrpn_CHANNEL_MAX];
	vrpn_int32	num_channel;
	struct timeval	timestamp;
	vrpn_int32 my_id;		// ID of this device to connection
	vrpn_int32 channel_m_id;	// channel message id
	int status; 

	virtual vrpn_int32 encode_to(char *buf);
        virtual void report_changes (vrpn_uint32 class_of_service
                                     = vrpn_CONNECTION_LOW_LATENCY);
            // send report iff changed
	virtual void report (vrpn_uint32 class_of_service
                             = vrpn_CONNECTION_LOW_LATENCY);  // send report
};

class vrpn_Serial_Analog: public vrpn_Analog {
public:
  vrpn_Serial_Analog(const char * name, vrpn_Connection * connection,
		     const char * port, int baud);
protected:
  int serial_fd;
  char portname[1024];
  int baudrate;
  unsigned char buffer[1024];
  int bufcounter;

  int read_available_characters(char *buffer,
	int bytes);
};

// vrpn_Analog_Server
// Tom Hudson, March 1999
//
// A simple core class to build other servers around;  used for the
// nMmon network monitor and some test programs.
//
// Write whatever values you want into channels(), then call report()
// or report_changes().  (Original spec only called for report_changes(),
// but vrpn_Analog's assumption that "no new data = same data" doesn't
// match the BLT stripchart assumption  of "no intervening data = ramp".
//
// For a sample application, see server_src/sample_analog.C


class vrpn_Analog_Server : public vrpn_Analog {

  public:

    vrpn_Analog_Server (const char * name, vrpn_Connection * c);
    virtual ~vrpn_Analog_Server (void);

    // The following function shouldn't exist;  there is no reasonable
    // semantics for it (neither report_changes() nor report() is
    // necessarily a good default behavior;  I want to make the choice
    // between the two explicit.)

    virtual void mainloop (const struct timeval *) { }

    // If anything has changed, report it to the client.
    
    virtual void report_changes (vrpn_uint32 class_of_service
                                 = vrpn_CONNECTION_LOW_LATENCY);

    // Reports current status to the client regardless of whether
    // or not there have been any changes.

    virtual void report (vrpn_uint32 class_of_service
                                 = vrpn_CONNECTION_LOW_LATENCY);

    // Exposes an array of values for the user to write into.

    vrpn_float64 * channels (void);

    // Size of the array.

    vrpn_int32 numChannels (void) const;

    // Sets the size of the array;  returns the size actually set.
    // (May be clamped to vrpn_CHANNEL_MAX)

    vrpn_int32 setNumChannels (vrpn_int32 sizeRequested);

};


//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a change in analog values.  This is called when
// the analog callback is called (when a message from its counterpart
// across the connetion arrives).


typedef	struct {
	struct timeval	msg_time;	// Timestamp of analog data
	vrpn_int32	num_channel;    // how many channels
	vrpn_float64	channel[vrpn_CHANNEL_MAX];  // analog values
} vrpn_ANALOGCB;

typedef void (*vrpn_ANALOGCHANGEHANDLER) (void * userdata,
					  const vrpn_ANALOGCB info);

// Open an analog device that is on the other end of a connection
// and handle updates from it.  This is the type of analog device
// that user code will deal with.

class vrpn_Analog_Remote: public vrpn_Analog {
  public:
	// The name of the analog device to connect to
        // Optional argument to be used when the Remote MUST listen on
        // a connection that is already open.
	vrpn_Analog_Remote (const char * name, vrpn_Connection * c = NULL);
	virtual ~vrpn_Analog_Remote (void);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(const struct timeval * timeout = NULL);

	// (un)Register a callback handler to handle analog value change
	virtual int register_change_handler(void *userdata,
		vrpn_ANALOGCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_ANALOGCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RBCS {
		void				*userdata;
		vrpn_ANALOGCHANGEHANDLER	handler;
		struct vrpn_RBCS		*next;
	} vrpn_ANALOGCHANGELIST;
	vrpn_ANALOGCHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
};

#endif

