#ifndef VRPN_PHANTOM_H
#define VRPN_PHANTOM_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include "ghost.h"
class Plane;
class DynamicPlane;
class Trimesh;
class ConstraintEffect;
class ForceFieldEffect;

// XXX HDAPI uses Planes rather than DynamicPlanes because I couldn't debug the
// more complicated plane port.
#ifdef	VRPN_USE_HDAPI
#undef	DYNAMIC_PLANES
#else
#define	DYNAMIC_PLANES
#endif

// ajout ONDIM
/*
# instantBuzzEffect : instantaneous buzz "custom" effect for Phantom server
# written by Sebastien MARAUX, ONDIM SA (France)
# maraux@ondim.fr
*/
const unsigned int NB_CUSTOM_EFFECTS = 1;
class InstantBuzzEffect;
const unsigned BUZZ_EFFECT_ID = 0;
// fin ajout ONDIM

#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_ForceDevice.h"

class vrpn_Plane_PHANTOMCB {
public:
  struct	timeval	msg_time;   // Time of the report
  float		SurfaceKspring;	    //surface spring coefficient
  float		SurfaceFdynamic;    //surface dynamic friction conefficient
  float		SurfaceFstatic;	    //surface static friction coefficient
  float		SurfaceKdamping;    //surface damping coefficient


  vrpn_int32	which_plane;
  float		plane[4];	// plane equation, ax+by+cz+d = 0
  vrpn_int32	numRecCycles;	// number of recovery cycles
};

typedef void (*vrpn_PHANTOMPLANECHANGEHANDLER)(void *userdata,
					 const vrpn_Plane_PHANTOMCB &info);

class vrpn_Phantom: public vrpn_ForceDevice,public vrpn_Tracker,
					public vrpn_Button_Filter {
	friend void phantomErrorHandler( int errnum, char *description,
                                         void *userdata);
protected:
	double update_rate;
#ifdef	VRPN_USE_HDAPI
	HHD		  phantom;	    //< The Phantom hardware device we are using
	HDSchedulerHandle hServoCallback;   //< The Haptic Servo loop callback identifier
#else
	gstScene *scene;
	gstSeparator *rootH;	    //< The haptics root separator attached to the scene.
	gstSeparator *hapticScene;  //< The next separator containing the entire scene graph.
	gstPHANToM *phantom;
#endif
	struct timeval timestamp;
#ifdef	DYNAMIC_PLANES
	DynamicPlane *planes[MAXPLANE];
#else
	Plane *planes[MAXPLANE];
#endif
	Trimesh *trimesh;
				       
	ConstraintEffect *pointConstraint;  // this is a force appended to
					    // other forces exerted by phantom
					    // (Superceded by ForceFieldEffect)
	ForceFieldEffect *forceField; //< general purpose force field approximation

	// ajout ONDIM
	// add each effect pointer
	InstantBuzzEffect *instantBuzzEffect;
	// fin ajout ONDIM
	
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
	static int handle_custom_effect_change_message(void *userdata,
						vrpn_HANDLERPARAM p);
	static int handle_dropConnection (void *, vrpn_HANDLERPARAM);

	// from vrpn_Tracker

	static int handle_update_rate_request (void *, vrpn_HANDLERPARAM);
	static int handle_resetOrigin_change_message(void * userdata,
							vrpn_HANDLERPARAM p);

public:
	vrpn_Phantom(char *name, vrpn_Connection *c, float hz=1.0);
	~vrpn_Phantom();

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
