#ifndef	TRACKER_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"
extern	int vrpn_open_commport(char *portname, long baud);

class vrpn_Tracker {
  public:
   vrpn_Tracker(char *name, vrpn_Connection *c = NULL);
   virtual void mainloop(void) = 0;	// Handle getting any reports

   void print_latest_report(void);

  protected:
   vrpn_Connection *connection;		// Used to send messages
   long my_id;				// ID of this tracker to connection
   long position_m_id;			// ID of tracker position message
   long velocity_m_id;			// ID of tracker velocity message
   long accel_m_id;			// ID of tracker acceleration message

   // Description of the next report to go out
   int sensor;			// Current sensor
   float pos[3], quat[4];	// Current position
   float vel[3], vel_quat[4];	// Current velocity
   float acc[3], acc_quat[4];	// Current acceleration
   struct timeval timestamp;	// Current timestamp

   int status;		// What are we doing?

   virtual int encode_to(char *buf);	 // Encodes the position report
   // Not all trackers will call the velocity and acceleration packers
   virtual int encode_vel_to(char *buf); // Encodes the velocity report
   virtual int encode_acc_to(char *buf); // Encodes the acceleration report
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

#endif  

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

// User routine to handle a tracker position update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	float		pos[3];		// Position of the sensor
	float		quat[4];	// Orientation of the sensor
} vrpn_TRACKERCB;
typedef void (*vrpn_TRACKERCHANGEHANDLER)(void *userdata,
					 const vrpn_TRACKERCB info);

// User routine to handle a tracker velocity update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	float		vel[3];		// Velocity of the sensor
	float		vel_quat[4];	// Delta Orientation of the sensor
} vrpn_TRACKERVELCB;
typedef void (*vrpn_TRACKERVELCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERVELCB info);

// User routine to handle a tracker acceleration update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	float		acc[3];		// Acceleration of the sensor
	float		acc_quat[4];	// Delta Delta Orientation of the sensor
} vrpn_TRACKERACCCB;
typedef void (*vrpn_TRACKERACCCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERACCCB info);

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

	// (un)Register a callback handler to handle a velocity change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler);

	// (un)Register a callback handler to handle an acceleration change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERACCCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERACCCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RTCS {
		void				*userdata;
		vrpn_TRACKERCHANGEHANDLER	handler;
		struct vrpn_RTCS		*next;
	} vrpn_TRACKERCHANGELIST;
	vrpn_TRACKERCHANGELIST	*change_list;

	typedef	struct vrpn_RTVCS {
		void				*userdata;
		vrpn_TRACKERVELCHANGEHANDLER	handler;
		struct vrpn_RTVCS		*next;
	} vrpn_TRACKERVELCHANGELIST;
	vrpn_TRACKERVELCHANGELIST	*velchange_list;

	typedef	struct vrpn_RTACS {
		void				*userdata;
		vrpn_TRACKERACCCHANGEHANDLER	handler;
		struct vrpn_RTACS		*next;
	} vrpn_TRACKERACCCHANGELIST;
	vrpn_TRACKERACCCHANGELIST	*accchange_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_vel_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_acc_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
};

#endif /* ifndef _WIN32 */

#define TRACKER_H
#endif
