
#ifndef FORCEDEVICE_H

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Connection.h"


#ifdef _WIN32
#include "ghost.h"
#include "plane.h"
#include <quat.h>
#endif

#define	FORCE_DEVICE_READY  0
#define FORCE_DEVICE_BROKEN 1

#define MAXPLANE 4   //maximum number of plane in the scene 

class vrpn_ForceDevice {
public:
	vrpn_ForceDevice(char * name, vrpn_Connection *c);
	virtual void mainloop(void) = 0;
	void print_report(void);
    void setSurfaceKspring(float k) { SurfaceKspring = k; }
    void setSurfaceKdamping(float d) {SurfaceKdamping =d;}
    void setSurfaceFstatic(float ks) {SurfaceFstatic = ks;}
    void setSurfaceFdynamic(float kd) {SurfaceFdynamic =kd;}


protected:
	vrpn_Connection *connection;		// Used to send messages
	int status;  //status of force device
//	virtual void get_report(void) = 0;
	virtual int encode_to(char *buf);
	
	struct timeval timestamp;
	long my_id;				// ID of this force device to connection
	long force_message_id;		// ID of force message to connection
	long plane_message_id;  //ID of plane equation message

	int   which_plane;
	float force[3];
	float plane[4];
	float SurfaceKspring;
	float SurfaceKdamping;
	float SurfaceFstatic;
	float SurfaceFdynamic;

};

#ifdef _WIN32

typedef	struct {
	struct		timeval	msg_time;	// Time of the report
	float		which_plane;
	float		plane[4];		// plane equation, ax+by+cz+d = 0
	float		SurfaceKspring; //surface spring coefficient
	float		SurfaceFdynamic;//surface dynamic friction conefficient
	float	    SurfaceFstatic; //surface static friction coefficient
	float		SurfaceKdamping;//surface damping coefficient
} vrpn_PHANTOMCB;

typedef void (*vrpn_PHANTOMCHANGEHANDLER)(void *userdata,
					 const vrpn_PHANTOMCB info);

class vrpn_Phantom: public vrpn_ForceDevice,public vrpn_Tracker,
					public vrpn_Button {
public:
	vrpn_Phantom(char *name, vrpn_Connection *c, float hz=1.0);
	virtual void mainloop(void);
	virtual void print_report(void);
	virtual int register_change_handler(void *userdata,
		vrpn_PHANTOMCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_PHANTOMCHANGEHANDLER handler);

	static void handle_plane(void *userdata,const vrpn_PHANTOMCB p);
    
protected:
	float update_rate;
	gstScene *scene;
	gstSeparator *rootH; /* This is the haptics root separator that is attached to the scene. */
	gstSeparator *hapticScene; /* This is the next separator that contains the entire scene graph. */
	gstPHANToM *phantom;
	struct timeval timestamp;
	Plane *planes[MAXPLANE];
	Plane *cur_plane;
  //  vrpn_PHANTOMCB	surface;

//	gstPoint cursor_pos;

	virtual void get_report(void);
	
	typedef	struct vrpn_RPCS {
		void				*userdata;
		vrpn_PHANTOMCHANGEHANDLER	handler;
		struct vrpn_RPCS		*next;
	} vrpn_PHANTOMCHANGELIST;
	vrpn_PHANTOMCHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);

};
#endif



typedef	struct {
	struct		timeval	msg_time;	// Time of the report
	float		force[3];		// force value
} vrpn_FORCECB;
typedef void (*vrpn_FORCECHANGEHANDLER)(void *userdata,
					 const vrpn_FORCECB info);

#ifndef _WIN32
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

	int encode_plane(char *buf);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a force change
	// and plane equation change
	virtual int register_change_handler(void *userdata,
		vrpn_FORCECHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_FORCECHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RFCS {
		void				*userdata;
		vrpn_FORCECHANGEHANDLER	handler;
		struct vrpn_RFCS		*next;
	} vrpn_FORCECHANGELIST;
	vrpn_FORCECHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);

};
#endif





#define  FORCEDEVICE_H

#endif

