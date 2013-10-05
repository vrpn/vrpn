#ifndef VRPN_PHANTOM_H
#define VRPN_PHANTOM_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

// Hide the include files which use the old version of iostreams
// by only defining the types we need from them here and doing
// the includes in the .C file.  Yucky and frought with danger.
// I've asked SensAble to hide the includes.
#ifdef	VRPN_USE_HDAPI
typedef unsigned int HHD;
typedef unsigned long HDSchedulerHandle;
typedef void *HHLRC;
typedef unsigned int HLuint;
#else
class gstScene;
class gstSeparator;
class gstPHANToM;
#endif

class Plane;
class DynamicPlane;
class Trimesh;
class ConstraintEffect;
class ForceFieldEffect;

// XXX HDAPI uses Planes rather than DynamicPlanes because I couldn't debug the
// more complicated plane port.  Symptoms of the broken-ness for the Dynamic
// plane: The sphere_client program puts a plane facing
// in the positive Y direction (at Y=0) which never moves.  Check the
// updateDynamics() call, perhaps.
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
#include "vrpn_ForceDeviceServer.h"

class vrpn_Plane_PHANTOMCB {
public:
  struct	timeval	msg_time;   // Time of the report
  float		SurfaceKspring;	    //surface spring coefficient
  float		SurfaceFdynamic;    //surface dynamic friction coefficient
  float		SurfaceFstatic;	    //surface static friction coefficient
  float		SurfaceKdamping;    //surface damping coefficient


  vrpn_int32	which_plane;
  float		plane[4];	// plane equation, ax+by+cz+d = 0
  vrpn_int32	numRecCycles;	// number of recovery cycles
};

typedef void (VRPN_CALLBACK *vrpn_PHANTOMPLANECHANGEHANDLER)(void *userdata,
					 const vrpn_Plane_PHANTOMCB &info);

class vrpn_Phantom: public vrpn_ForceDeviceServer,public vrpn_Tracker,
					public vrpn_Button_Filter {
	friend void phantomErrorHandler( int errnum, char *description,
                                         void *userdata);
protected:
	double update_rate;
#ifdef	VRPN_USE_HDAPI
	HHD		  phantom;	    //< The Phantom hardware device we are using
        HHLRC             hHLRC;            //< handle to haptic rendering context
        HLuint            effectId;         //< Effect ID of HL custom force effect
        int   button_0_bounce_count, button_1_bounce_count; //< Used to remove button "bounce"
	// Jean SIMARD <jean.simard@limsi.fr>
	// Add a field which contain the name of the PHANToM configuration
	char sconf[512];

#else
	gstScene *scene;
	gstSeparator *rootH;	    //< The haptics root separator attached to the scene.
	gstSeparator *hapticScene;  //< The next separator containing the entire scene graph.
	gstSeparator *phantomAxis;	//< The axis of the phantom is moved
	gstPHANToM *phantom;
#endif
	struct timeval timestamp;
#ifdef	DYNAMIC_PLANES
	DynamicPlane *planes[MAXPLANE];
#else
	Plane *planes[MAXPLANE];
#endif
//	Trimesh *trimesh;
				       
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

	static int VRPN_CALLBACK handle_plane_change_message(void *userdata, 
					       vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK handle_effects_change_message(void *userdata,
							vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK handle_forcefield_change_message(void *userdata,
						vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK handle_custom_effect_change_message(void *userdata,
						vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK handle_dropConnection (void *, vrpn_HANDLERPARAM);

	// from vrpn_Tracker

	static int VRPN_CALLBACK handle_update_rate_request (void *, vrpn_HANDLERPARAM);
	static int VRPN_CALLBACK handle_resetOrigin_change_message(void * userdata,
							vrpn_HANDLERPARAM p);

	// Add an object to the haptic scene as root (parent -1 = default) or as child (ParentNum =the number of the parent)
	virtual bool addObject(vrpn_int32 objNum, vrpn_int32 ParentNum); 
	// Add an object next to the haptic scene as root 
	virtual bool addObjectExScene(vrpn_int32 objNum); 
	// vertNum normNum and triNum start at 0
	virtual bool setVertex(vrpn_int32 objNum, vrpn_int32 vertNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z);
    // NOTE: ghost doesn't take normals, 
    //       and normals still aren't implemented for Hcollide
    virtual bool setNormal(vrpn_int32 objNum, vrpn_int32 normNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z);
    virtual bool setTriangle(vrpn_int32 objNum, vrpn_int32 triNum,vrpn_int32 vert0,vrpn_int32 vert1,vrpn_int32 vert2,
		  vrpn_int32 norm0,vrpn_int32 norm1,vrpn_int32 norm2);
    virtual bool removeTriangle(vrpn_int32 objNum, vrpn_int32 triNum); 
    // should be called to incorporate the above changes into the 
    // displayed trimesh 
    virtual bool updateTrimeshChanges(vrpn_int32 objNum,vrpn_float32 kspring, vrpn_float32 kdamp, vrpn_float32 fdyn, vrpn_float32 fstat);
	// set the type of trimesh
	virtual bool setTrimeshType(vrpn_int32 objNum,vrpn_int32 type);
    // set the trimesh's homogen transform matrix (in row major order)
    virtual bool setTrimeshTransform(vrpn_int32 objNum, vrpn_float32 homMatrix[16]);
	// set position of an object
	virtual bool setObjectPosition(vrpn_int32 objNum, vrpn_float32 Pos[3]);
	// set orientation of an object
	virtual bool setObjectOrientation(vrpn_int32 objNum, vrpn_float32 axis[3], vrpn_float32 angle);
	// set Scale of an object
	virtual bool setObjectScale(vrpn_int32 objNum, vrpn_float32 Scale[3]);
	// remove an object from the scene
	virtual bool removeObject(vrpn_int32 objNum);
    virtual bool clearTrimesh(vrpn_int32 objNum);
  
	/** Functions to organize the scene	**********************************************************/
	// Change The parent of an object
	virtual bool moveToParent(vrpn_int32 objNum, vrpn_int32 ParentNum);
	// Set the Origin of the haptic scene
	virtual bool setHapticOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle);    
	// Set the Scale factor of the haptic scene
	virtual bool setHapticScale(vrpn_float32 Scale);
	// Set the Origin of the haptic scene
	virtual bool setSceneOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle);    
	// get new ID, use only if wish to use vrpn ids and do not want to manage them yourself: ids need to be unique
	//virtual vrpn_int32 getNewObjectID();
	// make an object touchable or not
	virtual bool setObjectIsTouchable(vrpn_int32 objNum, vrpn_bool IsTouchable);

#ifdef VRPN_USE_HDAPI
        void initHL(HHD phantom);
        void tearDownHL(void);
#else
        gstSeparator *GetObject(vrpn_int32 objNum);
#endif
	Trimesh *GetObjectMesh(vrpn_int32 objNum);
public:
	// Jean SIMARD <jean.simard@limsi.fr>
	// Modification of the constructor
	// The default value is the default value use by the tool of SensAble
	vrpn_Phantom(char *name, vrpn_Connection *c, float hz=1.0, const char * newsconf="Default PHANToM");
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

	static void VRPN_CALLBACK handle_plane(void *userdata,const vrpn_Plane_PHANTOMCB &p);
	static void VRPN_CALLBACK check_parameters(vrpn_Plane_PHANTOMCB *p);
};

#endif
#endif
