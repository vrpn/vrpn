#ifndef	TRACKER_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

// NOTE: a vrpn tracker must call user callbacks with tracker data (pos and
//       ori info) which represent the transformation xfSourceFromSensor.
//       This means that the pos info is the position of the origin of
//       the sensor coord sys in the source coord sys space, and the
//       quat represents the orientation of the sensor relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the sensor's)

// to use time synched tracking, just pass in a sync connection to the 
// client and the server

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"

class vrpn_RedundantTransmission;

// tracker status flags
#define TRACKER_SYNCING		    (3)
#define	TRACKER_AWAITING_STATION    (2)
#define TRACKER_REPORT_READY 	    (1)
#define TRACKER_PARTIAL 	    (0)
#define TRACKER_RESETTING	    (-1)
#define TRACKER_FAIL 	 	    (-2)

#define TRACKER_MAX_SENSORS	(20)
// index for the change_list that should be called for all sensors
#define ALL_SENSORS		TRACKER_MAX_SENSORS
// this is the maximum for indices into sensor-specific change lists
// it is 1 more than the number of sensors because the last list is
// used to specify all sensors
#define TRACKER_MAX_SENSOR_LIST (TRACKER_MAX_SENSORS + 1)

class vrpn_Tracker : public vrpn_BaseClass {
  public:
   vrpn_Tracker (const char * name, vrpn_Connection * c = NULL);
   virtual ~vrpn_Tracker (void);

   int read_config_file (FILE * config_file, const char * tracker_name);
   void print_latest_report(void);
   // a tracker server should call the following to register the
   // default xform and workspace request handlers
   int register_server_handlers(void);
   void get_local_t2r(vrpn_float64 *vec, vrpn_float64 *quat);
   void get_local_u2s(vrpn_int32 sensor, vrpn_float64 *vec, vrpn_float64 *quat);
   static int handle_t2r_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_u2s_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_workspace_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_update_rate_request (void *, vrpn_HANDLERPARAM);

  protected:
   vrpn_int32 position_m_id;		// ID of tracker position message
   vrpn_int32 velocity_m_id;		// ID of tracker velocity message
   vrpn_int32 accel_m_id;		// ID of tracker acceleration message
   vrpn_int32 tracker2room_m_id;	// ID of tracker tracker2room message
   vrpn_int32 unit2sensor_m_id;		// ID of tracker unit2sensor message
   vrpn_int32 request_t2r_m_id;		// ID of tracker2room request message
   vrpn_int32 request_u2s_m_id;		// ID of unit2sensor request message
   vrpn_int32 request_workspace_m_id;	// ID of workspace request message
   vrpn_int32 workspace_m_id;		// ID of workspace message
   vrpn_int32 update_rate_id;		// ID of update rate message
   vrpn_int32 connection_dropped_m_id;	// ID of connection dropped message
   vrpn_int32 reset_origin_m_id;	// ID of reset origin message					

   // Description of the next report to go out
   vrpn_int32 d_sensor;			// Current sensor
   vrpn_float64 pos[3], d_quat[4];	// Current pose, (x,y,z), (qx,qy,qz,qw)
   vrpn_float64 vel[3], vel_quat[4];	// Cur velocity and dQuat/vel_quat_dt
   vrpn_float64 vel_quat_dt;		// delta time (in secs) for vel_quat
   vrpn_float64 acc[3], acc_quat[4];	// Cur accel and d2Quat/acc_quat_dt2
   vrpn_float64 acc_quat_dt;		// delta time (in secs) for acc_quat
   struct timeval timestamp;		// Current timestamp

   vrpn_float64 tracker2room[3], tracker2room_quat[4]; // Current t2r xform
   vrpn_int32 num_sensors;
   vrpn_float64 unit2sensor[TRACKER_MAX_SENSORS][3];
   vrpn_float64 unit2sensor_quat[TRACKER_MAX_SENSORS][4]; // Current u2s xforms

   // bounding box for the tracker workspace (in tracker space)
   // these are the points with (x,y,z) minimum and maximum
   // note: we assume the bounding box edges are aligned with the tracker
   // coordinate system
   vrpn_float64 workspace_min[3], workspace_max[3];

   int status;		// What are we doing?

   virtual int register_types(void);	//< Called by BaseClass init()
   virtual int encode_to(char *buf);	 // Encodes the position report
   // Not all trackers will call the velocity and acceleration packers
   virtual int encode_vel_to(char *buf); // Encodes the velocity report
   virtual int encode_acc_to(char *buf); // Encodes the acceleration report
   virtual int encode_tracker2room_to(char *buf); // Encodes the tracker2room
   virtual int encode_unit2sensor_to(char *buf); // and unit2sensor xforms
   virtual int encode_workspace_to(char *buf); // Encodes workspace info
};

#ifndef VRPN_CLIENT_ONLY
#define VRPN_TRACKER_BUF_SIZE 100

class vrpn_Tracker_Serial : public vrpn_Tracker {
  public:
   vrpn_Tracker_Serial
               (const char * name, vrpn_Connection * c,
		const char * port = "/dev/ttyS1", long baud = 38400);
  protected:
   char portname[VRPN_TRACKER_BUF_SIZE];
   long baudrate;
   int serial_fd;

   unsigned char buffer[VRPN_TRACKER_BUF_SIZE];// Characters read in from the tracker so far
   vrpn_uint32 bufcount;		// How many characters in the buffer?

   virtual void get_report(void) = 0;
   virtual void reset(void) = 0;
};
#endif  // VRPN_CLIENT_ONLY


class vrpn_Tracker_NULL: public vrpn_Tracker {
  public:
   vrpn_Tracker_NULL (const char * name, vrpn_Connection * c,
	vrpn_int32 sensors = 1, vrpn_float64 Hz = 1.0);
   virtual void mainloop();

   void setRedundantTransmission (vrpn_RedundantTransmission *);

  protected:
   vrpn_float64	update_rate;
   vrpn_int32	num_sensors;

   vrpn_RedundantTransmission * d_redundancy;
};




//----------------------------------------------------------
// ************** Users deal with the following *************

// User routine to handle a tracker position update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	vrpn_int32	sensor;		// Which sensor is reporting
	vrpn_float64	pos[3];		// Position of the sensor
	vrpn_float64	quat[4];	// Orientation of the sensor
} vrpn_TRACKERCB;
typedef void (*vrpn_TRACKERCHANGEHANDLER)(void *userdata,
					 const vrpn_TRACKERCB info);

// User routine to handle a tracker velocity update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	vrpn_int32	sensor;		// Which sensor is reporting
	vrpn_float64	vel[3];		// Velocity of the sensor
	vrpn_float64	vel_quat[4];	// Future Orientation of the sensor
        vrpn_float64	vel_quat_dt;    // delta time (in secs) for vel_quat
} vrpn_TRACKERVELCB;
typedef void (*vrpn_TRACKERVELCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERVELCB info);

// User routine to handle a tracker acceleration update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	vrpn_int32	sensor;		// Which sensor is reporting
	vrpn_float64	acc[3];		// Acceleration of the sensor
	vrpn_float64	acc_quat[4];	// ?????
        vrpn_float64	acc_quat_dt;    // delta time (in secs) for acc_quat
        
} vrpn_TRACKERACCCB;
typedef void (*vrpn_TRACKERACCCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERACCCB info);

// User routine to handle a tracker room2tracker xform update. This is called
// when the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef struct {
	struct timeval	msg_time;		// Time of the report
	vrpn_float64	tracker2room[3];	// position offset
	vrpn_float64	tracker2room_quat[4];	// orientation offset
} vrpn_TRACKERTRACKER2ROOMCB;
typedef void (*vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER)(void *userdata,
					const vrpn_TRACKERTRACKER2ROOMCB info);

typedef struct {
        struct timeval  msg_time;       	// Time of the report
	vrpn_int32	sensor;			// Which sensor this is for
        vrpn_float64  unit2sensor[3];		// position offset
        vrpn_float64  unit2sensor_quat[4];	// orientation offset
} vrpn_TRACKERUNIT2SENSORCB;
typedef void (*vrpn_TRACKERUNIT2SENSORCHANGEHANDLER)(void *userdata,
                                        const vrpn_TRACKERUNIT2SENSORCB info);

typedef struct {
	struct timeval  msg_time;       // Time of the report
	vrpn_float64 workspace_min[3];	// minimum corner of box (tracker CS)
	vrpn_float64 workspace_max[3];	// maximum corner of box (tracker CS)
} vrpn_TRACKERWORKSPACECB;
typedef void (*vrpn_TRACKERWORKSPACECHANGEHANDLER)(void *userdata,
					const vrpn_TRACKERWORKSPACECB info);

#ifndef VRPN_CLIENT_ONLY


// This is a virtual device that plays back pre-recorded sensor data.

class vrpn_Tracker_Canned: public vrpn_Tracker {
  
  public:
  
   vrpn_Tracker_Canned (const char * name, vrpn_Connection * c,
                        const char * datafile);

   virtual ~vrpn_Tracker_Canned (void);
   virtual void mainloop();

  protected:

   virtual void get_report (void);
   virtual void reset (void);

  private:

   void copy (void);

   FILE * fp;
   vrpn_int32 totalRecord;
   vrpn_TRACKERCB t;
   vrpn_int32 current;

};
#endif  // VRPN_CLIENT_ONLY




// Open a tracker that is on the other end of a connection
// and handle updates from it.  This is the type of tracker that user code will
// deal with.

class vrpn_Tracker_Remote: public vrpn_Tracker {
  public:
	// The name of the tracker to connect to, including connection name,
	// for example "Ceiling_tracker@ceiling.cs.unc.edu". If you already
	// have the connection open, you can specify it as the second parameter.
	// This allows both servers and clients in the same thread, for example.
	// If it is not specified, then the connection will be looked up based
	// on the name passed in.
	vrpn_Tracker_Remote (const char * name, vrpn_Connection *c = NULL);

        // unregister all of the handlers registered with the connection
        virtual ~vrpn_Tracker_Remote (void);

	// request room from tracker xforms
	int request_t2r_xform(void);
	// request all available sensor from unit xforms
	int request_u2s_xform(void);
	// request workspace bounding box
	int request_workspace(void);

	// set rate of p/v/a updates from the tracker
	int set_update_rate (vrpn_float64 samplesPerSecond);

	// reset origin to current tracker location (e.g. - to reinitialize
	// a PHANToM in its reset position)
	int reset_origin(void);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop();

	// **** to register handlers for sensor-specific messages: ****
	// Default is to register them for all sensors.

        // (un)Register a callback handler to handle a position change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);

        // (un)Register a callback handler to handle a velocity change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERVELCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERVELCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);

        // (un)Register a callback handler to handle an acceleration change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERACCCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERACCCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);

        // (un)Register a callback handler to handle a unit2sensor change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, vrpn_int32 sensor = ALL_SENSORS);

	// **** to get workspace information ****
	// (un)Register a callback handler to handle a workspace change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler);

	// (un)Register a callback handler to handle a tracker2room change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RTCS {
		void				*userdata;
		vrpn_TRACKERCHANGEHANDLER	handler;
		struct vrpn_RTCS		*next;
	} vrpn_TRACKERCHANGELIST;
	vrpn_TRACKERCHANGELIST	*change_list[TRACKER_MAX_SENSORS + 1];

	typedef	struct vrpn_RTVCS {
		void				*userdata;
		vrpn_TRACKERVELCHANGEHANDLER	handler;
		struct vrpn_RTVCS		*next;
	} vrpn_TRACKERVELCHANGELIST;
	vrpn_TRACKERVELCHANGELIST *velchange_list[TRACKER_MAX_SENSORS + 1];

	typedef	struct vrpn_RTACS {
		void				*userdata;
		vrpn_TRACKERACCCHANGEHANDLER	handler;
		struct vrpn_RTACS		*next;
	} vrpn_TRACKERACCCHANGELIST;
	vrpn_TRACKERACCCHANGELIST *accchange_list[TRACKER_MAX_SENSORS + 1];

	typedef struct vrpn_RTT2RCS {
		void				*userdata;
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler;
		struct vrpn_RTT2RCS		*next;
	} vrpn_TRACKERTRACKER2ROOMCHANGELIST;
	vrpn_TRACKERTRACKER2ROOMCHANGELIST *tracker2roomchange_list;

	typedef struct vrpn_RTU2SCS {
		void                            *userdata;
		vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler;
		struct vrpn_RTU2SCS		*next;
    } vrpn_TRACKERUNIT2SENSORCHANGELIST;
	vrpn_TRACKERUNIT2SENSORCHANGELIST *unit2sensorchange_list[TRACKER_MAX_SENSORS + 1];
	
	typedef struct vrpn_RTWSCS {
		void				*userdata;
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler;
		struct vrpn_RTWSCS	*next;
	} vrpn_TRACKERWORKSPACECHANGELIST;
	vrpn_TRACKERWORKSPACECHANGELIST *workspacechange_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_vel_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_acc_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_tracker2room_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_unit2sensor_change_message(void *userdata,
                        vrpn_HANDLERPARAM p);
	static int handle_workspace_change_message(void *userdata,
			vrpn_HANDLERPARAM p);

};


#define TRACKER_H
#endif
