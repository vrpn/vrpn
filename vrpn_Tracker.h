#ifndef	TRACKER_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

// to use time synched tracking, just pass in a sync connection to the
// client and the server

#include "vrpn_Connection.h"

// flush discards characters in buffer
// drain blocks until they are written
extern int vrpn_open_commport(char *portname, long baud);
extern int vrpn_flush_input_buffer( int comm );
extern int vrpn_flush_output_buffer( int comm );
extern int vrpn_drain_output_buffer( int comm );

// tracker status flags
#define TRACKER_SYNCING		(2)
#define TRACKER_REPORT_READY 	(1)
#define TRACKER_PARTIAL 	(0)
#define TRACKER_RESETTING	(-1)
#define TRACKER_FAIL 	 	(-2)

#define TRACKER_MAX_SENSORS	(20)

class vrpn_Tracker {
  public:
   vrpn_Tracker(char *name, vrpn_Connection *c = NULL);
   virtual void mainloop(void) = 0;	// Handle getting any reports
   virtual ~vrpn_Tracker() {};

   int read_config_file(FILE *config_file, char *tracker_name);
   void print_latest_report(void);

  protected:
   vrpn_Connection *connection;         // Used to send messages
   long my_id;				// ID of this tracker to connection
   long position_m_id;			// ID of tracker position message
   long velocity_m_id;			// ID of tracker velocity message
   long accel_m_id;			// ID of tracker acceleration message
   long room2tracker_m_id;		// ID of tracker room2tracker message
   long sensor2unit_m_id;		// ID of tracker sensor2unit message
   long request_r2t_m_id;		// ID of room2tracker request message
   long request_s2u_m_id;		// ID of sensor2unit request message
					

   // Description of the next report to go out
   int sensor;			// Current sensor
   double pos[3], quat[4];	// Current position, (x,y,z), (qx,qy,qz,qw)
   double vel[3], vel_quat[4];	// Current velocity and dQuat/vel_quat_dt
   double vel_quat_dt;          // delta time (in secs) for vel_quat
   double acc[3], acc_quat[4];	// Current acceleration and d2Quat/acc_quat_dt2
   double acc_quat_dt;          // delta time (in secs) for acc_quat
   struct timeval timestamp;	// Current timestamp

   double room2tracker[3], room2tracker_quat[4]; // Current r2t xform
   long num_sensors;
   double sensor2unit[TRACKER_MAX_SENSORS][3];
   double sensor2unit_quat[TRACKER_MAX_SENSORS][4]; // Current s2u xforms

   int status;		// What are we doing?

   virtual int encode_to(char *buf);	 // Encodes the position report
   // Not all trackers will call the velocity and acceleration packers
   virtual int encode_vel_to(char *buf); // Encodes the velocity report
   virtual int encode_acc_to(char *buf); // Encodes the acceleration report
   virtual int encode_room2tracker_to(char *buf); // Encodes the room2tracker
   virtual int encode_sensor2unit_to(char *buf); // and sensor2unit xforms
};

#ifndef _WIN32
#define BUF_SIZE 100

class vrpn_Tracker_Serial : public vrpn_Tracker {
  public:
   vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
		char *port = "/dev/ttyS1", long baud = 38400);
  protected:
   char portname[BUF_SIZE];
   long baudrate;
   int serial_fd;

   unsigned char buffer[BUF_SIZE];// Characters read in from the tracker so far
   unsigned bufcount;		// How many characters in the buffer?

   int read_available_characters(unsigned char *buffer, int count);
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
// ************** Users deal with the following *************

// User routine to handle a tracker position update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		pos[3];		// Position of the sensor
	double		quat[4];	// Orientation of the sensor
} vrpn_TRACKERCB;
typedef void (*vrpn_TRACKERCHANGEHANDLER)(void *userdata,
					 const vrpn_TRACKERCB info);

// User routine to handle a tracker velocity update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		vel[3];		// Velocity of the sensor
	double		vel_quat[4];	// Future Orientation of the sensor
        double          vel_quat_dt;    // delta time (in secs) for vel_quat
} vrpn_TRACKERVELCB;
typedef void (*vrpn_TRACKERVELCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERVELCB info);

// User routine to handle a tracker acceleration update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		acc[3];		// Acceleration of the sensor
	double		acc_quat[4];	// Future Future Ori of the sensor
        double          acc_quat_dt;    // delta time (in secs) for acc_quat
        
} vrpn_TRACKERACCCB;
typedef void (*vrpn_TRACKERACCCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERACCCB info);

// User routine to handle a tracker room2tracker xform update. This is called
// when the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef struct {
	struct timeval	msg_time;	// Time of the report
	double	room2tracker[3];	// position offset
	double	room2tracker_quat[4];	// orientation offset
} vrpn_TRACKERROOM2TRACKERCB;
typedef void (*vrpn_TRACKERROOM2TRACKERCHANGEHANDLER)(void *userdata,
					const vrpn_TRACKERROOM2TRACKERCB info);

typedef struct {
        struct timeval  msg_time;       // Time of the report
	long	sensor;			// Which sensor this is the xform for
        double  sensor2unit[3];        // position offset
        double  sensor2unit_quat[4];   // orientation offset
} vrpn_TRACKERSENSOR2UNITCB;
typedef void (*vrpn_TRACKERSENSOR2UNITCHANGEHANDLER)(void *userdata,
                                        const vrpn_TRACKERSENSOR2UNITCB info);
// Open a tracker that is on the other end of a connection
// and handle updates from it.  This is the type of tracker that user code will
// deal with.

class vrpn_Tracker_Remote: public vrpn_Tracker {
  public:
	// The name of the tracker to connect to
	vrpn_Tracker_Remote(char *name);

	// request room to tracker xforms
	int request_r2t_xform(void);
	// request all available sensor to unit xforms
	int request_s2u_xform(void);

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

	// (un)Register a callback handler to handle a room2tracker change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERROOM2TRACKERCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERROOM2TRACKERCHANGEHANDLER handler);

	// (un)Register a callback handler to handle a sensor2unit change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERSENSOR2UNITCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERSENSOR2UNITCHANGEHANDLER handler);

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

	typedef struct vrpn_RTR2TCS {
		void				*userdata;
		vrpn_TRACKERROOM2TRACKERCHANGEHANDLER handler;
		struct vrpn_RTR2TCS		*next;
	} vrpn_TRACKERROOM2TRACKERCHANGELIST;
	vrpn_TRACKERROOM2TRACKERCHANGELIST *room2trackerchange_list;

	typedef struct vrpn_RTS2UCS {
		void                            *userdata;
		vrpn_TRACKERSENSOR2UNITCHANGEHANDLER handler;
		struct vrpn_RTS2UCS		*next;
        } vrpn_TRACKERSENSOR2UNITCHANGELIST;
	vrpn_TRACKERSENSOR2UNITCHANGELIST *sensor2unitchange_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_vel_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_acc_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_room2tracker_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_sensor2unit_change_message(void *userdata,
                        vrpn_HANDLERPARAM p);
};


#define TRACKER_H
#endif
