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
#include "vrpn_BaseClass.h"

// Contention:  vrpn_Analog shouldn't have a mainloop() function.
//   (Neither should other similar base classes).  There isn't any
//   reason for some derived classes to implement one, and there may
//   not even be any meaningful semantics.  (qv vrpn_Analog_Server below)
// Instead, declare a virtual destructor = 0 to force it to be an abstract
//   class.
// TCH March 1999

class vrpn_Analog : public vrpn_BaseClass {
public:
	vrpn_Analog (const char * name, vrpn_Connection * c = NULL);

	// Print the status of the analog device
	void print(void);

  protected:
	vrpn_float64	channel[vrpn_CHANNEL_MAX];
	vrpn_float64	last[vrpn_CHANNEL_MAX];
	vrpn_int32	num_channel;
	struct timeval	timestamp;
	vrpn_int32 channel_m_id;	// channel message id
	int status; 

	virtual	int register_types(void);
	virtual vrpn_int32 encode_to(char *buf);
	/// Send a report only if something has changed (for servers)
        virtual void report_changes (vrpn_uint32 class_of_service
                                     = vrpn_CONNECTION_LOW_LATENCY);
	/// Send a report whether something has changed or not (for servers)
	virtual void report (vrpn_uint32 class_of_service
                             = vrpn_CONNECTION_LOW_LATENCY);
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
/// A simple core class to build other servers around;
// used for the nMmon network monitor and some test programs.
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

    /// Makes public the protected base class function
    virtual void report_changes (vrpn_uint32 class_of_service
                                 = vrpn_CONNECTION_LOW_LATENCY);

    /// Makes public the protected base class function
    virtual void report (vrpn_uint32 class_of_service
                                 = vrpn_CONNECTION_LOW_LATENCY);

    /// For this server, the user will normally call report() or
    /// report_changes() directly.  Here, mainloop() defaults to
    /// calling report(), since we are not asking for reliable
    /// communication.  Note that this will cause very rapid sending
    /// of reports if this is called each time through a loop whose
    /// rate is unchecked.
    virtual void mainloop () { server_mainloop(); report(); };

    /// Exposes an array of values for the user to write into.
    vrpn_float64 * channels (void);

    /// Size of the array.
    vrpn_int32 numChannels (void) const;

    /// Sets the size of the array;  returns the size actually set.
    /// (May be clamped to vrpn_CHANNEL_MAX)
    vrpn_int32 setNumChannels (vrpn_int32 sizeRequested);
};

/// Analog server that can scale and clip its range to -1..1.
// This is useful for joysticks, to allow them to be centered and
// scaled to cover the whole range.  Rather than writing directly
// into the channels array, call the setChannel() method.

class	vrpn_Clipping_Analog_Server : public vrpn_Analog_Server {
  public:
    vrpn_Clipping_Analog_Server(const char *name, vrpn_Connection *c);

    /// Set the clipping values for the specified channel.
    /// min maps to -1, values between lowzero and highzero map to 0,
    /// max maps to 1.  Values less than min map to -1, values larger
    /// than max map to 1. Default for each channel is -1,0,0,1
    /// It is possible to compress the range to [0..1] by setting the
    /// minimum equal to the lowzero.
    /// Returns 0 on success, -1 on failure.
    int	setClipValues(int channel, double min, double lowzero, double
	highzero, double max);

    /// This method should be used to set the value of a channel.
    /// It will be scaled and clipped as described in setClipValues.
    /// It returns 0 on success and -1 on failure.
    int	setChannelValue(int channel, double value);

  protected:
      typedef	struct {
	  double    minimum_val;    // Value mapped to -1
	  double    lower_zero;	    // Minimum value mapped to 0
	  double    upper_zero;	    // Maximum value mapped to 0
	  double    maximum_val;    // Value mapped to 1
      } clipvals_struct;

      clipvals_struct	clipvals[vrpn_CHANNEL_MAX];
};

//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a change in analog values.  This is called when
// the analog callback is called (when a message from its counterpart
// across the connection arrives).


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
	virtual void mainloop();

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

