#ifndef VRPN_PHANTOM_H
#define VRPN_PHANTOM_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

class gstScene;
class gstSeparator;
class gstPHANToM;
class DynamicPlane;
class Trimesh;
class ConstraintEffect;
class ForceFieldEffect;

#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_ForceDevice.h"

class vrpn_Plane_PHANTOMCB {
public:
  struct	timeval	msg_time;// Time of the report
  float		SurfaceKspring; //surface spring coefficient
  float		SurfaceFdynamic;//surface dynamic friction conefficient
  float	    SurfaceFstatic; //surface static friction coefficient
  float		SurfaceKdamping;//surface damping coefficient


  vrpn_int32		which_plane;
  float		plane[4];	// plane equation, ax+by+cz+d = 0
  vrpn_int32		numRecCycles;	// number of recovery cycles
};

typedef void (*vrpn_PHANTOMPLANECHANGEHANDLER)(void *userdata,
					 const vrpn_Plane_PHANTOMCB &info);

class vrpn_Phantom: public vrpn_ForceDevice,public vrpn_Tracker,
					public vrpn_Button_Filter {
	friend void phantomErrorHandler( int errnum, char *description,
                                         void *userdata);
protected:
	float update_rate;
	gstScene *scene;
	gstSeparator *rootH;	    //< The haptics root separator attached to the scene.
	gstSeparator *hapticScene;  //< The next separator containing the entire scene graph.
	gstPHANToM *phantom;
	struct timeval timestamp;
	DynamicPlane *planes[MAXPLANE];
	Trimesh *trimesh;
				       
	ConstraintEffect *pointConstraint; // this is a force appended to
					// other forces exerted by phantom
					// (Superceded by ForceFieldEffect)
	ForceFieldEffect *forceField; //< general purpose force field approximation

	virtual void get_report(void);
	
	typedef	struct vrpn_RPCS {
		void				*userdata;
		vrpn_PHANTOMPLANECHANGEHANDLER	handler;
		struct vrpn_RPCS		*next;
	} vrpn_PHANTOMCHANGELIST;
	vrpn_PHANTOMCHANGELIST	*plane_change_list;

	static int handle_plane_change_message(void *userdata, 
					       vrpn_HANDLERPARAM p);
	static int handle_effects_change_message(void *userdata,
							vrpn_HANDLERPARAM p);
	static int handle_setVertex_message(void *userdata, 
				     vrpn_HANDLERPARAM p);
	static int handle_setNormal_message(void *userdata, 
				     vrpn_HANDLERPARAM p);
	static int handle_setTriangle_message(void *userdata, 
				       vrpn_HANDLERPARAM p);
	static int handle_removeTriangle_message(void *userdata, 
				       vrpn_HANDLERPARAM p);
	static int handle_updateTrimeshChanges_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
	static int handle_transformTrimesh_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
	static int handle_setTrimeshType_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
	static int handle_clearTrimesh_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
	static int handle_forcefield_change_message(void *userdata,
						vrpn_HANDLERPARAM p);

	// from vrpn_Tracker

	static int handle_update_rate_request (void *, vrpn_HANDLERPARAM);
	static int handle_resetOrigin_change_message(void * userdata,
							vrpn_HANDLERPARAM p);

public:
	vrpn_Phantom(char *name, vrpn_Connection *c, float hz=1.0);
	virtual void mainloop(void);
	virtual void reset();
	void resetPHANToM(void);
	void getPosition(double *vec, double *quat);
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

#endif
#endif
