
#ifndef FORCEDEVICE_H
#define  FORCEDEVICE_H

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

#ifdef _WIN32
#ifndef	VRPN_CLIENT_ONLY
#include "ghost.h"
#include "plane.h"
#include "trimesh.h"
#endif
#include <quat.h>
#endif

#define	FORCE_DEVICE_READY  0
#define FORCE_DEVICE_BROKEN 1

#define MAXPLANE 4   //maximum number of plane in the scene 

// for recovery:
#define DEFAULT_NUM_REC_CYCLES	(10)

// number of floating point values encoded in a message
#define NUM_MESSAGE_PARAMETERS (10)

class vrpn_ForceDevice {
public:
	vrpn_ForceDevice(char * name, vrpn_Connection *c);
	virtual void mainloop(void) = 0;
	void print_report(void);

	void setSurfaceKspring(float k) { 
					SurfaceKspring = k; }
	void setSurfaceKdamping(float d) {SurfaceKdamping =d;}
	void setSurfaceFstatic(float ks) {SurfaceFstatic = ks;}
	void setSurfaceFdynamic(float kd) {SurfaceFdynamic =kd;}
	void setRecoveryTime(int rt) {numRecCycles = rt;}
	int getRecoveryTime(void) {return numRecCycles;}
	int connectionAvailable(void) {return (connection != NULL);}

protected:
	vrpn_Connection *connection;		// Used to send messages
	int status;  //status of force device
//	virtual void get_report(void) = 0;
	virtual int encode_to(char *buf);
	virtual int encode_scp_to(char *buf);
	
	struct timeval timestamp;
	long my_id;		// ID of this force device to connection
	long force_message_id;	// ID of force message to connection
	long plane_message_id;  //ID of plane equation message
	long scp_message_id;	// ID of surface contact point message
        // IDs for trimesh messages
        long startTrimesh_message_id;   
        long setVertex_message_id;   
        long setTriangle_message_id;   
        long finishTrimesh_message_id;   


	int   which_plane;
	double force[3];
	double scp_pos[3];
	double scp_quat[4];  // I think this would only be used on 6DOF device
	float plane[4];

	float SurfaceKspring;
	float SurfaceKdamping;
	float SurfaceFstatic;
	float SurfaceFdynamic;
	int numRecCycles;

};

#ifdef _WIN32
#ifndef	VRPN_CLIENT_ONLY

class vrpn_Plane_PHANTOMCB {
public:
  struct	timeval	msg_time;// Time of the report
  float		SurfaceKspring; //surface spring coefficient
  float		SurfaceFdynamic;//surface dynamic friction conefficient
  float	        SurfaceFstatic; //surface static friction coefficient
  float		SurfaceKdamping;//surface damping coefficient

  float		which_plane;
  float		plane[4];	// plane equation, ax+by+cz+d = 0
  float		numRecCycles;	// number of recovery cycles
};


typedef void (*vrpn_PHANTOMPLANECHANGEHANDLER)(void *userdata,
					 const vrpn_Plane_PHANTOMCB &info);

class vrpn_Phantom: public vrpn_ForceDevice,public vrpn_Tracker,
					public vrpn_Button {
protected:
	float update_rate;
	gstScene *scene;
	gstSeparator *rootH; /* This is the haptics root separator that is 
					attached to the scene. */
	gstSeparator *hapticScene; /* This is the next separator that contains 
					the entire scene graph. */
	gstPHANToM *phantom;
	struct timeval timestamp;
	Plane *planes[MAXPLANE];
	Plane *cur_plane;
	Trimesh *trimesh;
				       
	gstEffect *effect; 	// this is a force appended to
				// other forces exerted by phantom
  //  vrpn_PHANTOMCB	surface;

//	gstPoint cursor_pos;

	virtual void get_report(void);
	
	typedef	struct vrpn_RPCS {
		void				*userdata;
		vrpn_PHANTOMPLANECHANGEHANDLER	handler;
		struct vrpn_RPCS		*next;
	} vrpn_PHANTOMCHANGELIST;
	vrpn_PHANTOMCHANGELIST	*plane_change_list;

	static int handle_plane_change_message(void *userdata, 
					       vrpn_HANDLERPARAM p);
	static int handle_startTrimesh_message(void *userdata, 
					vrpn_HANDLERPARAM p);
	static int handle_setVertex_message(void *userdata, 
				     vrpn_HANDLERPARAM p);
	static int handle_setTriangle_message(void *userdata, 
				       vrpn_HANDLERPARAM p);
	static int handle_finishTrimesh_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
public:
	vrpn_Phantom(char *name, vrpn_Connection *c, float hz=1.0);
	virtual void mainloop(void);
	virtual void reset();
	virtual void print_report(void);
	virtual int register_change_handler(void *userdata,
		vrpn_PHANTOMPLANECHANGEHANDLER handler,
		vrpn_PHANTOMCHANGELIST	*&change_list);
	virtual int unregister_change_handler(void *userdata,
		vrpn_PHANTOMPLANECHANGEHANDLER handler,
		vrpn_PHANTOMCHANGELIST	*&change_list);

	virtual int register_plane_change_handler(void *userdata,
		vrpn_PHANTOMPLANECHANGEHANDLER handler);
	virtual int unregister_plane_change_handler(void *userdata,
		vrpn_PHANTOMPLANECHANGEHANDLER handler);

	static void handle_plane(void *userdata,const vrpn_Plane_PHANTOMCB &p);
    static void check_parameters(vrpn_Plane_PHANTOMCB *p);	

};

#endif  // VRPN_CLIENT_ONLY
#endif  // _WIN32

// User routine to handle position reports for surface contact point (SCP)
// This is in vrpn_ForceDevice rather than vrpn_Tracker because only
// a force feedback device should know anything about SCPs as this is a
// part of the force feedback model. It may be preferable to use the SCP
// rather than the tracker position for graphics so the hand position
// doesn't appear to go below the surface making the surface look very
// compliant. 
typedef struct {
	struct		timeval msg_time;	// Time of the report
	double		pos[3];			// position of SCP
	double		quat[4];		// orientation of SCP
} vrpn_FORCESCPCB;
typedef void (*vrpn_FORCESCPHANDLER) (void *userdata,
					const vrpn_FORCESCPCB info);

typedef	struct {
	struct		timeval	msg_time;	// Time of the report
	double		force[3];		// force value
} vrpn_FORCECB;
typedef void (*vrpn_FORCECHANGEHANDLER)(void *userdata,
					 const vrpn_FORCECB info);
class vrpn_ForceDevice_Remote: public vrpn_ForceDevice {
public:

	// The name of the force device to connect to
	vrpn_ForceDevice_Remote(char *name);
 	void set_plane(float *p);
	void set_plane(float *p, float d);
	void set_plane(float a, float b, float c,float d);

	void sendSurface(void);
	void startSurface(void);
	void stopSurface(void);

	void startSendingTrimesh(int numVerts,int numTris);
        // vertNum and triNum start at 0
        void sendVertex(int vertNum,float x,float y,float z);
        void sendTriangle(int triNum,int vert0,int vert1,int vert2);
        void finishSendingTrimesh();
  	void stopTrimesh(void);

	char *encode_plane(int &len);
	char *encode_startTrimesh(int &len,int numVerts,int numTris);
        char *encode_vertex(int &len,int vertNum,float x,float y,float z); 
        char *encode_triangle(int &len,int triNum,
			      int vert0,int vert1,int vert2);	       
        char *encode_finishTrimesh(int &len);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a force change
	// and plane equation change and trimesh change
	virtual int register_change_handler(void *userdata,
		vrpn_FORCECHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_FORCECHANGEHANDLER handler);

	virtual int register_scp_change_handler(void *userdata,
                vrpn_FORCESCPHANDLER handler);
        virtual int unregister_scp_change_handler(void *userdata,
                vrpn_FORCESCPHANDLER handler);

protected:

	typedef	struct vrpn_RFCS {
		void				*userdata;
		vrpn_FORCECHANGEHANDLER	handler;
		struct vrpn_RFCS		*next;
	} vrpn_FORCECHANGELIST;
	vrpn_FORCECHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);

        typedef struct vrpn_RFSCPCS {
                void                            *userdata;
                vrpn_FORCESCPHANDLER handler;
		struct vrpn_RFSCPCS		*next;
	} vrpn_FORCESCPCHANGELIST;
	vrpn_FORCESCPCHANGELIST	*scp_change_list;
        static int handle_scp_change_message(void *userdata,
                                                        vrpn_HANDLERPARAM p);

        void set_trimesh(int nV,float v[][3],
			 int nT,int t[][3]);
};

#endif

