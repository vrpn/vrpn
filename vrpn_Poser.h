#ifndef	vrpn_POSER_H
#define vrpn_POSER_H

#ifndef _WIN32_WCE
#include <time.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#ifndef _WIN32_WCE
#include <sys/time.h>
#endif
#endif

// NOTE: the poser class borrows heavily from the vrpn_Tracker code.
//       The poser is basically the inverse of a tracker.  Start off 
//       really simple--just handles getting pose requests.  Add 
//       counterparts of Tracker code as needed...

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"
#include "vrpn_Analog.h"



// These structures are used in callbacks on both the client and server side 

// User routine to handle a poser position request.
typedef	struct {
	struct timeval	msg_time;	// Time of the report
	vrpn_float64	pos[3];		// Position of the poser
	vrpn_float64	quat[4];	// Orientation of the poser
} vrpn_POSERCB;
typedef void (*vrpn_POSERCHANGEHANDLER)(void *userdata,
                    const vrpn_POSERCB info);

// User routine to handle a velocity request
typedef	struct {
	struct timeval	msg_time;	    // Time of the report
	vrpn_float64	vel[3];		    // Requested velocity of the position
	vrpn_float64	vel_quat[4];	// Requested velocity of the orientation
    vrpn_float64	vel_quat_dt;    // delta time (in secs) for vel_quat
} vrpn_POSERVELCB;
typedef void (*vrpn_POSERVELCHANGEHANDLER)(void *userdata,
                    const vrpn_POSERVELCB info);



class vrpn_Poser : public vrpn_BaseClass {
    public:
        vrpn_Poser (const char * name, vrpn_Connection * c = NULL );

        virtual ~vrpn_Poser (void);

        // a poser server should call the following to register the
        // default xform and workspace request handlers
//        int register_server_handlers(void);

        // (un)Register a callback handler to handle a position change
        virtual int register_change_handler(void *userdata,
                vrpn_POSERCHANGEHANDLER handler);
        virtual int unregister_change_handler(void *userdata,
                vrpn_POSERCHANGEHANDLER handler);

        // (un)Register a callback handler to handle a velocity change
        virtual int register_change_handler(void *userdata,
                vrpn_POSERVELCHANGEHANDLER handler);
        virtual int unregister_change_handler(void *userdata,
                vrpn_POSERVELCHANGEHANDLER handler);

    protected:
        // client-->server
        vrpn_int32 req_position_m_id;		// ID of poser position message
        vrpn_int32 req_velocity_m_id;       // ID of poser velocity message

        // server-->client
        vrpn_int32 ack_position_m_id;       // ID of poser position acknowledgement
        vrpn_int32 ack_velocity_m_id;       // ID of poser velocity acknowledgement
        
        // Description of current state
        vrpn_float64 pos[3], d_quat[4];	    // Current pose, (x,y,z), (qx,qy,qz,qw)
        vrpn_float64 vel[3], vel_quat[4];   // Current velocity and dQuat/vel_quat_dt
        vrpn_float64 vel_quat_dt;           // delta time (in secs) for vel_quat
        struct timeval timestamp;		    // Current timestamp

        // bounding box for the poser workspace (in poser space)
        // these are the points with (x,y,z) minimum and maximum
        // note: we assume the bounding box edges are aligned with the poser
        // coordinate system
        vrpn_float64 workspace_min[3], workspace_max[3];

        virtual int register_types(void);	    // Called by BaseClass init()

        virtual int encode_to(char *buf);       // Encodes the position
        virtual int encode_vel_to(char *buf);   // Encodes the velocity

        virtual void set_pose(struct timeval t,                 // Sets the pose
                            vrpn_float64 position[3], 
                            vrpn_float64 quaternion[4]);
        virtual void set_pose_velocity(struct timeval t,        // Sets the velocity
                                    vrpn_float64 position[3], 
                                    vrpn_float64 quaternion[4], 
                                    vrpn_float64 interval);

        // Client or Server should supply this code...Basically just depends on the message id.  Req or Ack
        // Ideally, we want these to be pure virtual...
        virtual int send_pose() { return 0; }               // Sends the current pose (server-->client or client-->server)
        virtual int send_pose_velocity() { return 0;}       // Sends the current pose velocity (server-->client or client-->server)   

        typedef	struct vrpn_RPCS {
		    void				        *userdata;
		    vrpn_POSERCHANGEHANDLER	    handler;
		    struct vrpn_RPCS		    *next;
	    } vrpn_POSERCHANGELIST;
	    vrpn_POSERCHANGELIST *change_list;

        typedef struct vrpn_RPVCS {
            void                        *userdata;
            vrpn_POSERVELCHANGEHANDLER  handler;
            struct vrpn_RPVCS           *next;
        } vrpn_POSERVELCHANGELIST;
        vrpn_POSERVELCHANGELIST *velchange_list;

        static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
        static int handle_vel_change_message(void *userdata, vrpn_HANDLERPARAM p);
};


//------------------------------------------------------------------------------------
// Server Code
// Users supply the routines to handle requests from the client

// This is a basic poser server used to drive an Analog device

class vrpn_Poser_Server: public vrpn_Poser {
    public:
        vrpn_Poser_Server (const char * name, vrpn_Connection * c, const char * ana_name = NULL);

        /// This function should be called each time through app mainloop.
        virtual void mainloop();

    protected:
        vrpn_Analog_Remote* ana;            // Analog client

        virtual int send_pose();            // Sends the current pose
        virtual int send_pose_velocity();   // Sends the current velocity
            
        static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
        static int handle_vel_change_message(void *userdata, vrpn_HANDLERPARAM p);
};

//------------------------------------------------------------------------------------
// Client Code
// Open a poser that is on the other end of a connection for sending updates to 
// it.  

class vrpn_Poser_Remote: public vrpn_Poser {
    public:
	    // The name of the poser to connect to, including connection name,
	    // for example "poser@magnesium.cs.unc.edu". If you already
	    // have the connection open, you can specify it as the second parameter.
	    // This allows both servers and clients in the same thread, for example.
	    // If it is not specified, then the connection will be looked up based
	    // on the name passed in.
	    vrpn_Poser_Remote (const char * name, vrpn_Connection *c = NULL);

        // unregister all of the handlers registered with the connection
        virtual ~vrpn_Poser_Remote (void);

	    // This routine calls the mainloop of the connection it's on
	    virtual void mainloop();
   
        // Routines to set the state of the poser
        int request_pose(struct timeval t, vrpn_float64 position[3], vrpn_float64 quaternion[4]);
        int request_pose_velocity(struct timeval t, vrpn_float64 position[3], vrpn_float64 quaternion[4], vrpn_float64 interval);

    protected:
        virtual int send_pose();            // Sends the current pose
        virtual int send_pose_velocity();   // Sends the current velocity
};

#endif
