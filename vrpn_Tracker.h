#ifndef	TRACKER_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"

class vrpn_Tracker {
  public:
   vrpn_Tracker(vrpn_Connection *c = NULL) : connection(c) {};
   virtual void mainloop(void) = 0;	// Handle getting any reports

   void print_latest_report(void);

  protected:
   vrpn_Connection *connection;		// Used to send messages
   long my_id;				// ID of this tracker to connection
   long message_id;			// ID of tracker message to connection

   // Description of the latest report
   int sensor;
   float pos[3], quat[4];
   struct timeval timestamp;

   int status;		// What are we doing?

   virtual int encode_to(char *buf);
};

#ifndef _WIN32
class vrpn_Tracker_Serial : public vrpn_Tracker {
  public:
   vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
		char *port = "/dev/ttyS1", long baud = 38400);
  protected:
   char portname[100];
   long baudrate;
   int serial_fd;

   unsigned char buffer[100];	// Characters read in from the tracker so far
   unsigned bufcount;		// How many characters in the buffer?

   int readAvailableCharacters(unsigned char *buffer, int count);
   virtual void get_report(void) = 0;
   virtual void reset(void) = 0;
};

class vrpn_Tracker_3Space: public vrpn_Tracker_Serial {
  public:
   vrpn_Tracker_3Space(char *name,
	vrpn_Connection *c, char *port = "/dev/ttyS1", long baud = 19200) :
		vrpn_Tracker_Serial(name,c,port,baud) {};
   virtual void mainloop(void);
  protected:
   virtual void get_report(void);
   virtual void reset();
};
#endif  // #ifndef _WIN32

class vrpn_Tracker_NULL: public vrpn_Tracker {
  public:
   vrpn_Tracker_NULL(char *name, vrpn_Connection *c, int sensors = 1,
	float Hz = 1.0);
   virtual void mainloop(void);
  protected:
   float	update_rate;
   int		num_sensors;
};


//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a tracker update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	float		pos[3];		// Position of the sensor
	float		quat[4];	// Orientation of the sensor
} vrpn_TRACKERCB;
typedef void (*vrpn_TRACKERCHANGEHANDLER)(void *userdata,
					 const vrpn_TRACKERCB info);

// Open a tracker that is on the other end of a connection
// and handle updates from it.  This is the type of tracker that user code will
// deal with.
#ifndef _WIN32

class vrpn_Tracker_Remote: public vrpn_Tracker {
  public:
	// The name of the tracker to connect to
	vrpn_Tracker_Remote(char *name);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a position change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RTCS {
		void				*userdata;
		vrpn_TRACKERCHANGEHANDLER	handler;
		struct vrpn_RTCS		*next;
	} vrpn_TRACKERCHANGELIST;
	vrpn_TRACKERCHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
};

#endif /* ifndef _WIN32 */

#define TRACKER_H
#endif
