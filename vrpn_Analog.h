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
class vrpn_Analog {
public:
	vrpn_Analog (const char * name, vrpn_Connection * c = NULL);

	// Print the status of the analog device
	void print(void);

	// Called once through each main loop iteration to handle
	// updates.
	virtual void mainloop(void) = 0;	// Report changes to conneciton

        
        vrpn_Connection *connectionPtr();

  protected:
	vrpn_Connection *connection;
	double	channel[vrpn_CHANNEL_MAX];
	double	last[vrpn_CHANNEL_MAX];
	int	num_channel;
	struct timeval	timestamp;
	long my_id;	                   // ID of this device to connection
	long channel_m_id;                 // channel message id
	int status; 

	virtual int encode_to(char *buf);
        virtual void report_changes(); 
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


//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a change in button state.  This is called when
// the analog callback is called (when a message from its counterpart
// across the connetion arrives).


typedef	struct {
	struct timeval	msg_time;	// Timestamp of analog data
	int		num_channel;    // how many channels
	double		channel[vrpn_CHANNEL_MAX];  // analog values
} vrpn_ANALOGCB;

typedef void (*vrpn_ANALOGCHANGEHANDLER) (void * userdata,
					  const vrpn_ANALOGCB info);

// Open a analog device that is on the other end of a connection
// and handle updates from it.  This is the type of analog device
// that user code will deal with.

class vrpn_Analog_Remote: public vrpn_Analog {
  public:
	// The name of the button device to connect to
        // Optional argument to be used when the Remote MUST listen on
        // a connection that is already open.
	vrpn_Analog_Remote (const char * name, vrpn_Connection * c = NULL);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a button state change
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










