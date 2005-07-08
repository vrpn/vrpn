//XXX Problem: Recovery time seems to have gotten itself unimplemented over time.
#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "vrpn_Connection.h"
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_ForceDevice.h"
#include "vrpn_Phantom.h"

#include "ghost.h"
#ifdef	VRPN_USE_HDAPI
  #include <HD/hd.h>
  #include <HDU/hduError.h>
#endif

#include "plane.h"
#include "texture_plane.h"
#include "trimesh.h"
#include "constraint.h"
#include "forcefield.h"
#include "quat.h"

// XXX Make sure to guard the setting of parameters to various effects when
// using HDAPI so that we don't get a partial update for them and then have
// the real-time loop run in the middle.

// This pragma tells the compiler not to tell us about truncated debugging info
// due to name expansion within the string, list, and vector classes.
#pragma warning( disable : 4786 )
#include  <vector>
using namespace std;

// ajout ONDIM
/*
# instantBuzzEffect : instantaneous buzz "custom" effect for Phantom server
# written by Sebastien MARAUX, ONDIM SA (France)
# maraux@ondim.fr
*/
// add each ghost effect header
#include "ghostEffects/InstantBuzzEffect.h"
// fin ajout ONDIM

#ifndef	_WIN32
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

#define CHECK(a) if (a == -1) return -1

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

#ifdef	VRPN_USE_HDAPI
//--------------------------------------------------------------------------------
// BEGIN HDAPI large chunk

// Structure to hold pointers to active force-generating objects,
// and the current force and surface contact point.

class ACTIVE_FORCE_OBJECT_LIST {
public:
#ifdef	DYNAMIC_PLANES
  vector<DynamicPlane*>	      Planes;
#else
  vector<Plane*>	      Planes;
#endif
  vector<ForceFieldEffect*>   ForceFieldEffects;
  vector<ConstraintEffect*>   ConstraintEffects;
  vector<InstantBuzzEffect*>  InstantBuzzEffects;

  vrpn_HapticVector	      CurrentForce;
  vrpn_HapticPosition	      SCP;		    //< Surface Contact Point

  ACTIVE_FORCE_OBJECT_LIST() { CurrentForce.set(0,0,0); SCP.set(0,0,0); };
};

static ACTIVE_FORCE_OBJECT_LIST	g_active_force_object_list;

// The work of reading the entire device state is done in a callback
// handler that is scheduled synchronously between the force-calculation
// thread and the main (communication) thread.  This makes sure we don't
// have a collision between the two.

HDCallbackCode HDCALLBACK readDeviceState(void *pUserData)
{
  HDAPI_state *state = (HDAPI_state *)pUserData;

  hdGetIntegerv(HD_CURRENT_BUTTONS, &state->buttons);
  hdGetDoublev(HD_CURRENT_TRANSFORM, &state->pose[0][0]);
  hdGetDoublev(HD_LAST_TRANSFORM, &state->last_pose[0][0]);
  hdGetIntegerv(HD_INSTANTANEOUS_UPDATE_RATE, &state->instant_rate);
  hdGetDoublev(HD_NOMINAL_MAX_STIFFNESS, &state->max_stiffness);

  return HD_CALLBACK_DONE;
}

// This callback handler is called once during each haptic schedule frame.
// It handles calculation of the force based on the currently-active planes,
// forcefields, constraints, effects, and other force-generating objects.
// It relies on the above static global data structure to keep track of
// which objects are active.

HDCallbackCode HDCALLBACK HDAPIForceCallback(void *pUserData)
{
  HDErrorInfo error;
  hdBeginFrame(hdGetCurrentDevice());

    HDAPI_state	state;
    hdScheduleSynchronous(readDeviceState, &state, HD_MAX_SCHEDULER_PRIORITY);

    int i;

    // Remember: g_active_force_object_list holds both the lists
    // of objects that generate forces and the current status of
    // both the force and the surface contact point, if any.
    // To start with, we have no force.
    g_active_force_object_list.CurrentForce.set(0,0,0);

    // XXX Some day, triangles meshes (and/or height fields?)

    // Update dynamics for all dynamic objects (right now just dynamic planes...)
#ifdef	DYNAMIC_PLANES
    for (i = 0; i < g_active_force_object_list.Planes.size(); i++) {
      g_active_force_object_list.Planes[i]->updateDynamics();
    }
#endif

    // Need to do collision detection with all planes and determine
    // the final location of the SCP.
    bool    found_collision = false;
    double  current_position[3] = { state.pose[3][0], state.pose[3][1], state.pose[3][2] };
    for (i = 0; i < g_active_force_object_list.Planes.size(); i++) {
      //XXXSolveForSCPAndCheckForCollisions;

      // XXX For now, just treat each plane separately and sum up
      // the forces; we base the force on the penetration depth of
      // the plane times its spring constant.  We say there are no
      // collisions (not true) to avoid calling the code below.
      vrpn_HapticPosition	  where(current_position);
      vrpn_HapticCollisionState	  collistion_state(where);

      //XXX Where did the collisionDetect() routine go for the dynamic plane?

      vrpn_HapticPlane	plane = g_active_force_object_list.Planes[i]->getPlane();
      bool  active = g_active_force_object_list.Planes[i]->getActive();
      double depth= -plane.error(where);
      //if (active) { printf("%lf, %lf, %lf,  %lf  (%lf)\n", plane.a(), plane.b(), plane.c(), plane.d(), g_active_force_object_list.Planes[i]->getSurfaceKspring()  ); }
      if (active && (depth > 0) ) {
	g_active_force_object_list.SCP = plane.projectPoint(where);

        // Scale the spring constant by the maximum stable stiffness for the device.
        // This is because the spring constant is in terms of this factor.
	g_active_force_object_list.CurrentForce +=
          plane.normal() * depth * g_active_force_object_list.Planes[i]->getSurfaceKspring() * state.max_stiffness;
      }
    }

    // If there were no collisions, then set the surface contact point
    // to be the current hand position.  If there were collisions,
    // then set the force based on the difference between the surface
    // contact point and the current hand position.
    if (!found_collision) {
	g_active_force_object_list.SCP.set(current_position);
    } else {
	// We did have at least one collision, so we need to compute
	// the reaction force due to surface penetration and friction.
	// XXX Friction for each plane differs
	// XXX Normal force for each plane differs
	// XXX Damping in each plane differs.
    }

    // Need to add in any constraint forces
    for (i = 0; i < g_active_force_object_list.ConstraintEffects.size(); i++) {
	g_active_force_object_list.CurrentForce += g_active_force_object_list.ConstraintEffects[i]->calcEffectForce(&state);
    }

    // Need to add in any effect forces
    for (i = 0; i < g_active_force_object_list.ForceFieldEffects.size(); i++) {
	g_active_force_object_list.CurrentForce += g_active_force_object_list.ForceFieldEffects[i]->calcEffectForce(&state);
    }

    // Need to add in any instant buzzing effect
    for (i = 0; i < g_active_force_object_list.InstantBuzzEffects.size(); i++) {
	g_active_force_object_list.CurrentForce += g_active_force_object_list.InstantBuzzEffects[i]->calcEffectForce();
    }

    // Send the force to the device
    double  force[3];
    force[0] = g_active_force_object_list.CurrentForce[0];
    force[1] = g_active_force_object_list.CurrentForce[1];
    force[2] = g_active_force_object_list.CurrentForce[2];
    hdSetDoublev(HD_CURRENT_FORCE, force);

  hdEndFrame(hdGetCurrentDevice());

  /* Check if an error occurred while attempting to render the force */
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      if (hduIsForceError(&error)) {
	  // XXX bRenderForce = FALSE;  // Example code from AnchoredSpringForce.c
      } else {
          return HD_CALLBACK_DONE;
      }
  }

  return HD_CALLBACK_CONTINUE;
}

// END HDAPI large chunk
//--------------------------------------------------------------------------------
#endif

void vrpn_Phantom::handle_plane(void *userdata,const vrpn_Plane_PHANTOMCB &p)
{
/*  printf("MY Plane is %lfX + %lfY + %lfZ + %lf = 0 \n",p.plane[0],
	p.plane[1],p.plane[2],p.plane[3]);
  printf("surface is %lf,%lf,%lf,%lf\n",p.SurfaceKspring,p.SurfaceKdamping,
	  p.SurfaceFdynamic, p.SurfaceFstatic);
     printf("which plane = %d\n", p.which_plane);
*/
    vrpn_Phantom *me = (vrpn_Phantom *) userdata;
 
#ifndef	VRPN_USE_HDAPI
    gstPHANToM *phan = me->phantom;
#endif
    int which_plane = p.which_plane;
#ifdef	DYNAMIC_PLANES
    DynamicPlane *plane = (me->planes)[which_plane];
#else
    Plane *plane = (me->planes)[which_plane];
#endif

    // If the plane's normal is (0,0,0), it is a stop surface command
    // turn off the surface.
    if( (p.plane[0] == 0) && (p.plane[1] == 0) && (p.plane[2] == 0) ) {
	plane->setActive(FALSE);
    } else {
	vrpn_Plane_PHANTOMCB p2 = p;
	check_parameters(&p2);
	plane->update(p2.plane[0],p2.plane[1],
		p2.plane[2],p2.plane[3]*1000.0);//convert m to mm
	plane->setSurfaceKspring(p2.SurfaceKspring);
	plane->setSurfaceFstatic(p2.SurfaceFstatic);
	plane->setSurfaceFdynamic(p2.SurfaceFdynamic); 
	plane->setSurfaceKdamping(p2.SurfaceKdamping);
	plane->setNumRecCycles((int)p2.numRecCycles);
	plane->setActive(TRUE);
    }
}

// This function reports errors if surface friction, compliance parameters
// are not in valid ranges for the GHOST library and sets them as 
// close as it can to the requested values.

void vrpn_Phantom::check_parameters(vrpn_Plane_PHANTOMCB *p)
{
    // only prints out errors every 5 seconds so that
    // we don't take too much time from servo loop
    static bool recentError = false;
    static timeval timeOfLastReport;
    timeval currTime;

    vrpn_gettimeofday(&currTime, NULL);

    // if this is the first time check_parameters has been called then initialize
    // timeOfLastReport, otherwise this has no effect on behavior
    if (!recentError) {
	    timeOfLastReport.tv_sec = currTime.tv_sec;

    // if recentError is true then change it to false if enough time has elapsed
    } else if (currTime.tv_sec - timeOfLastReport.tv_sec > 5) {
	    recentError = false;
	    timeOfLastReport.tv_sec = currTime.tv_sec;
    }

    if (p->SurfaceKspring <= 0){
	if (!recentError)
		fprintf(stderr,"Error: Kspring = %f <= 0\n", p->SurfaceKspring);
	p->SurfaceKspring = 0.001f;
	recentError = true;
    } else if (p->SurfaceKspring > 1.0){
	if (!recentError)
		printf("Error: Kspring = %f > 1.0\n", p->SurfaceKspring);
	p->SurfaceKspring = 1.0;
	recentError = true;
    }
    if (p->SurfaceFstatic < 0){
	if (!recentError)
		printf("Error: Fstatic = %f < 0\n", p->SurfaceFstatic);
        p->SurfaceFstatic = 0.0;
		recentError = true;
    } else if (p->SurfaceFstatic > 1.0){
	if (!recentError)
		printf("Error: Fstatic = %f > 1.0\n", p->SurfaceFstatic);
	p->SurfaceFstatic = 1.0;
	recentError = true;
    }
    if (p->SurfaceFdynamic > p->SurfaceFstatic){
	if (!recentError)
		printf("Error: Fdynamic > Fstatic\n");
	p->SurfaceFdynamic = 0.0;
	recentError = true;
    }
    if (p->SurfaceKdamping < 0){
	if (!recentError)
		printf("Error: Kdamping = %f < 0\n", p->SurfaceKdamping);
	p->SurfaceKdamping = 0;
	recentError = true;
    } else if (p->SurfaceKdamping > 0.005){
	if (!recentError)
		printf("Error: Kdamping = %f > 0.005\n", p->SurfaceKdamping);
	p->SurfaceKdamping = 0.005f;
	recentError = true;
    }
    if (p->numRecCycles < 1) {
	if (!recentError)
		printf("Error: numRecCycles < 1\n");
	p->numRecCycles = 1;
	recentError = true;
    }
}

// This function reinitializes the PHANToM (resets the origin)
void vrpn_Phantom::resetPHANToM(void)
{
#ifdef	VRPN_USE_HDAPI
  /* Cleanup by stopping the haptics loop, unscheduling the asynchronous
     callback and disabling the device */
  hdStopScheduler();
  hdUnschedule(hServoCallback);
  hdDisableDevice(phantom);

  HDErrorInfo error;
  phantom = hdInitDevice(HD_DEFAULT_DEVICE);
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to initialize haptic device");
  }

  /* Schedule the haptic callback function for continuously monitoring the
     button state and rendering the anchored spring force */
  hServoCallback = hdScheduleAsynchronous(
      HDAPIForceCallback, this, HD_MAX_SCHEDULER_PRIORITY);

  hdEnable(HD_FORCE_OUTPUT);
  Sleep(500);

  /* Start the haptic rendering loop */
  hdStartScheduler();
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to start scheduler");
  }

#else
  // Ghost 3.0 manual says you don't have to do this
    // with the Desktop Phantom...
    scene->stopServoLoop();
    rootH->removeChild(phantom);
    delete(phantom);
    phantom = new gstPHANToM("Default PHANToM", TRUE);
    rootH->addChild(phantom);
    Sleep(500);
    scene->startServoLoop();
#endif
}

int vrpn_Phantom::handle_dropConnection(void * userdata, vrpn_HANDLERPARAM)
{     
  printf("vrpn_Phantom: Dropped last connection, resetting force effects\n");
  ((vrpn_Phantom *)(userdata))->reset();

  // Always return 0 here, because nonzero return means that the input data
  // was garbage, not that there was an error. If we return nonzero from a
  // vrpn_Connection handler, it shuts down the connection.
  return 0;
}

// return the position of the phantom stylus relative to base
// coordinate system
void vrpn_Phantom::getPosition(double *vec, double *orient)
{
    for (int i = 0; i < 3; i++){
      vec[i] = pos[i];
      orient[i] = d_quat[i];
    }
    orient[3] = d_quat[3];
}

vrpn_Phantom::vrpn_Phantom(char *name, vrpn_Connection *c, float hz)
		:vrpn_Tracker(name, c),vrpn_Button_Filter(name,c),
		 vrpn_ForceDevice(name,c), update_rate(hz),
#ifndef	VRPN_USE_HDAPI
                 scene(NULL), 
                 rootH(NULL),
                 hapticScene(NULL),
                 phantom(NULL),
                 trimesh(NULL),
#endif
                 pointConstraint(NULL),
                 forceField(NULL),
		 plane_change_list(NULL),
		 // ajout ONDIM
		 // add each effect
		 instantBuzzEffect(NULL)
		 // fin ajout ONDIM
{  
  num_buttons = 1;  // only has one button

  timestamp.tv_sec = 0;
  timestamp.tv_usec = 0;
  // Initialization to NULL necessary if construction of Phantom fails below. 
  // Error handler gets called, and it references planes[] array. 
  int i;
  for (i = 0; i < MAXPLANE; i++){
      planes[i]= NULL;
  }

#ifndef	VRPN_USE_HDAPI
  scene = new gstScene;

  /* make it so simulation loop doesn't exit if remote switch is released */
#ifdef VRPN_USE_GHOST_31
  scene->setQuitOnDevFault((vrpn_HapticBoolean)FALSE);
#endif
  /*  this function is removed in Ghost 4.0.  From Ryan Toohil, SensAble support:
		"SetQuitOnDevFault() was removed and replaced by a far better method of
		managing device faults.  In GHOST v4, device faults are handled
		automatically, with the servo loop restarting itself.  It's a far less
		invasive method of handling errors, but you can override that behavior if
		desired (by creating your own error handler)."
  */

  /* disable popup error dialogs */
  printErrorMessages(FALSE);
  setErrorCallback(phantomErrorHandler, (void *)this);

  /* Create the root separator. */
  rootH = new gstSeparator;
  scene->setRoot(rootH);
  
  /* Create the phantom object.  When this line is processed, 
     the phantom position is zeroed. */
  phantom = new gstPHANToM("Default PHANToM");
  // Ghost 3.0 spec says we should make sure construction succeeded. 
  if(!phantom->getValidConstruction()) {
      fprintf(stderr, "ERROR: Invalid Phantom object created\n");
  }
  /* We add the phantom to the root instead of the hapticScene (whic
     is the child of the root) because when we turn haptics off
     via the 'H' key, we do not want to remove the phantom from the
     scene.  Pressing the 'H' key removes hapticScene and
     its children from the scene graph. */
  rootH->addChild(phantom);
  hapticScene = new gstSeparator;     
  rootH->addChild(hapticScene);
#endif

  pointConstraint = new ConstraintEffect();
#ifdef	VRPN_USE_HDAPI
  g_active_force_object_list.ConstraintEffects.push_back(pointConstraint);
  pointConstraint->stop();
#else
  phantom->setEffect(pointConstraint);
#endif
  forceField = new ForceFieldEffect();
#ifdef	VRPN_USE_HDAPI
  g_active_force_object_list.ForceFieldEffects.push_back(forceField);
  forceField->stop();
#endif
  // ajout ONDIM
  // add each custom effect instance here
  instantBuzzEffect = new InstantBuzzEffect();
#ifdef	VRPN_USE_HDAPI
  g_active_force_object_list.InstantBuzzEffects.push_back(instantBuzzEffect);
  instantBuzzEffect->stop();
#endif
  // fin ajout ONDIM

  SurfaceKspring= 0.29f;
  SurfaceFdynamic = 0.02f;
  SurfaceFstatic = 0.03f;
  SurfaceKdamping = 0.0f;

  SurfaceKadhesionNormal = 0.0f;
  SurfaceKadhesionLateral = 0.0f;
  SurfaceBuzzFreq = 60.0f;
  SurfaceBuzzAmp = 0.0f;
  SurfaceTextureWavelength = 0.01f;
  SurfaceTextureAmplitude = 0.0f;

  for(i=0; i<MAXPLANE; i++ ) {
#ifdef	DYNAMIC_PLANES
    planes[i] = new DynamicPlane();
    planes[i]->setBuzzAmplitude(1000.0*SurfaceBuzzAmp);
    planes[i]->setBuzzFrequency(SurfaceBuzzFreq);
    planes[i]->setTextureAmplitude(1000.0*SurfaceTextureAmplitude);
    planes[i]->setTextureWavelength(1000.0*SurfaceTextureWavelength);
#else
    planes[i] = new Plane(0,1,0,0);
#endif
    planes[i]->setActive(FALSE);
    planes[i]->setSurfaceKspring(SurfaceKspring);
    planes[i]->setSurfaceFdynamic(SurfaceFdynamic);
    planes[i]->setSurfaceFstatic(SurfaceFstatic);
    planes[i]->setSurfaceKdamping(SurfaceKdamping);
#ifdef	VRPN_USE_HDAPI
    g_active_force_object_list.Planes.push_back(planes[i]);
#else
    hapticScene->addChild(planes[i]);
#endif
  }

  which_plane = 0;
 
#ifndef	VRPN_USE_HDAPI
  trimesh = new Trimesh();
  trimesh->addToScene(hapticScene);
#endif

  //  status= TRACKER_RESETTING;
  vrpn_gettimeofday(&(vrpn_ForceDevice::timestamp),NULL);

  if (register_autodeleted_handler(plane_message_id, 
	handle_plane_change_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(plane_effects_message_id,
	handle_effects_change_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(setVertex_message_id, 
	handle_setVertex_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(setNormal_message_id, 
	handle_setNormal_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(setTriangle_message_id, 
	handle_setTriangle_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(removeTriangle_message_id, 
	handle_removeTriangle_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(updateTrimeshChanges_message_id, 
	handle_updateTrimeshChanges_message, this, vrpn_ForceDevice::  d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(transformTrimesh_message_id, 
	handle_transformTrimesh_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(setTrimeshType_message_id, 
	handle_setTrimeshType_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(clearTrimesh_message_id, 
	handle_clearTrimesh_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(forcefield_message_id,
	handle_forcefield_change_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }
  if (register_autodeleted_handler(reset_origin_m_id,
        handle_resetOrigin_change_message, this, vrpn_Tracker::d_sender_id)) {
                fprintf(stderr,"vrpn_Phantom:can't register handler\n");
                vrpn_Tracker::d_connection = NULL;
  }
  if (register_autodeleted_handler(custom_effect_message_id,
	handle_custom_effect_change_message, this, vrpn_ForceDevice::d_sender_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::d_connection = NULL;
  }

  this->register_plane_change_handler(this, handle_plane);

  if (vrpn_Tracker::register_server_handlers())
    fprintf(stderr, "vrpn_Phantom: couldn't register xform request handlers\n");

  if (register_autodeleted_handler
       (update_rate_id, handle_update_rate_request, this,
        vrpn_Tracker::d_sender_id)) {
                fprintf(stderr, "vrpn_Phantom:  "
                                "Can't register update-rate handler\n");
                vrpn_Tracker::d_connection = NULL;
  }

  //--------------------------------------------------------------------
  // Whenever we drop the last connection to this server, we also
  // want to reset all of the planes and other active effects so that
  // forces are zeroed.
  register_autodeleted_handler(d_connection->register_message_type
                                      (vrpn_dropped_last_connection),
		handle_dropConnection, this);

#ifdef	VRPN_USE_HDAPI
  HDErrorInfo error;
  phantom = hdInitDevice(HD_DEFAULT_DEVICE);
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to initialize haptic device");
  }

  /* Schedule the haptic callback function for continuously monitoring the
     button state and rendering the anchored spring force */
  hServoCallback = hdScheduleAsynchronous(
      HDAPIForceCallback, this, HD_MAX_SCHEDULER_PRIORITY);

  hdEnable(HD_FORCE_OUTPUT);

  /* Start the haptic rendering loop */
  hdStartScheduler();
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to start scheduler");
  }
#else
  scene->startServoLoop();
#endif
}

vrpn_Phantom::~vrpn_Phantom()
{
#ifdef	VRPN_USE_HDAPI
  /* Cleanup by stopping the haptics loop, unscheduling the asynchronous
     callback and disabling the device */
  hdStopScheduler();
  hdUnschedule(hServoCallback);
  hdDisableDevice(phantom);
#endif
}

void vrpn_Phantom::print_report(void)
{ 
   printf("----------------------------------------------------\n");
   vrpn_ForceDevice::print_report();

 //  printf("Timestamp:%ld:%ld\n",timestamp.tv_sec, timestamp.tv_usec);
   printf("Pos      :%lf, %lf, %lf\n", pos[0],pos[1],pos[2]);
   printf("Quat     :%lf, %lf, %lf, %lf\n", d_quat[0],d_quat[1],d_quat[2],d_quat[3]);
   //printf("Force	:%lf, %lf, %lf\n", d_force[0],d_force[1], d_force[2]);
}

// mainloop:
//      get button status from GHOST
//      send button message (done in vrpn_Button::report_changes)
//      get position from GHOST (done in vrpn_Phantom::get_report())
//      get force from GHOST (done in vrpn_Phantom::get_report())
//      send position message
//      send force message

void vrpn_Phantom::mainloop(void) {
    struct timeval current_time;
    char    msgbuf[1000];
    char    *buf;
    vrpn_int32	len;

    // Allow the base server class to do its thing
    server_mainloop();

    //set if it is time to generate a new report 
    vrpn_gettimeofday(&current_time, NULL);
    if(duration(current_time,timestamp) >= 1000000.0/update_rate) {
		
        //update the time
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

	//------------------------------------------------------------------------
	// Read the device state and convert all values into the appropriate
	// units for VRPN and store them into local state.
#ifdef	VRPN_USE_HDAPI
	HDAPI_state state;
	hdScheduleSynchronous(readDeviceState, &state, HD_MIN_SCHEDULER_PRIORITY);

	// Convert buttons to VRPN
	if (state.buttons & HD_DEVICE_BUTTON_1) {
	  buttons[0] = 1;
	} else {
	  buttons[0] = 0;
	}

	// Convert position and orientation to VRPN (meters and quaternion)
	q_type	rot;
	pos[0] = state.pose[3][0] / 1000.0;  // mm to meters
	pos[1] = state.pose[3][1] / 1000.0;  // mm to meters
	pos[2] = state.pose[3][2] / 1000.0;  // mm to meters
	q_from_row_matrix(rot, state.pose);
	q_copy(d_quat, rot);

	// Convert velocity to VRPN
	double	deltaT = 1.0 / state.instant_rate;
	vel[0] = (pos[0] - state.last_pose[3][0] / 1000.0) / deltaT;
	vel[1] = (pos[1] - state.last_pose[3][1] / 1000.0) / deltaT;
	vel[2] = (pos[2] - state.last_pose[3][2] / 1000.0) / deltaT;

	// Convert orientation rate to VRPN.
	// First, find the change in rotation from last to now,
	// then scale this by the time (scaling is done in an axis/angle
	// representation; we convert to and from this to get the quaternion
	// corresponding that the resulting rotation).
	q_type	last_rot;
	q_from_row_matrix(last_rot, state.last_pose);
	q_type	delta_rot;
	q_invert(last_rot, last_rot);
	q_mult(delta_rot, rot, last_rot); // Find the change in orientation
	double	x,y,z, angle;	//< The axis-angle representation
	q_to_axis_angle(&x, &y, &z, &angle, delta_rot);
	angle /= deltaT;	//< Divide by change in time to get change per unit time
	q_from_axis_angle(delta_rot, x,y,z, angle); //< Convert back to Quaternion.

	// Store the surface contact point
	scp_pos[0] = g_active_force_object_list.SCP[0];
	scp_pos[1] = g_active_force_object_list.SCP[1];
	scp_pos[2] = g_active_force_object_list.SCP[2];
	scp_quat[0] = scp_quat[1] = scp_quat[2] = 0; scp_quat[3] = 1;

	// Store the current force being generated
	d_force[0] = g_active_force_object_list.CurrentForce[0];
	d_force[1] = g_active_force_object_list.CurrentForce[1];
	d_force[2] = g_active_force_object_list.CurrentForce[2];
#else
	//check button status
	if(phantom->getStylusSwitch() ) {
		buttons[0] = 1;   //button is pressed
	    //	printf("button is pressed.\n");
	} else {
		buttons[0]= 0;
	    //	printf("button is released. \n");
	}

        // Read the current PHANToM position, velocity, and force
        // Put the information into pos/quat
	double x,y,z;
	double dt_vel = .10, dt_acc = .10;
	q_matrix_type rot_matrix;
	q_type p_quat, v_quat;
	//q_type a_quat; // angular information (pos, vel, acc)
	double angVelNorm;
	// double angAccNorm;
	int i,j;

        // SCP means "surface contact point"
	vrpn_HapticPosition scpt, pt;
        vrpn_HapticVector phantomVel;
        vrpn_HapticVector phantomAcc;
	vrpn_HapticVector phantomForce;
	vrpn_HapticMatrix phantomRot;

	phantom->getPosition_WC(pt);
	phantom->getSCP_WC(scpt);

	scpt.getValue(x,y,z);
	scp_pos[0] = x/1000.0;	// convert from mm to m
	scp_pos[1] = y/1000.0;
	scp_pos[2] = z/1000.0;
	
	pt.getValue(x,y,z);
	pos[0] = x/1000.0;  //converts from mm to meter
	pos[1] = y/1000.0;
	pos[2] = z/1000.0;

	phantomVel = phantom->getVelocity();//mm/sec
        phantomVel.getValue(x, y, z);
        vel[0] = x/1000.0;      // convert from mm to m
        vel[1] = y/1000.0;
        vel[2] = z/1000.0;

	phantomForce = phantom->getReactionForce();
        phantomForce.getValue(x,y,z);
	d_force[0] = x;
	d_force[1] = y;
	d_force[2] = z;

	// transform rotation matrix to quaternion
	phantomRot = phantom->getRotationMatrix();	
	for(i=0; i<4; i++) {
	    for(j=0;j<4;j++) {
		rot_matrix[i][j] = phantomRot.get(i,j);
	    }
	}
	q_from_row_matrix(p_quat,rot_matrix);

        vrpn_HapticVector phantomAngVel = phantom->getAngularVelocity();//rad/sec
        angVelNorm = phantomAngVel.norm();
        phantomAngVel.normalize();

	// compute angular velocity quaternion
        phantomAngVel.getValue(x,y,z);
        q_make(v_quat, x, y, z, angVelNorm*dt_vel);
        // set v_quat = v_quat*d_quat
        q_mult(v_quat, v_quat, d_quat);


	for(i=0;i<4;i++ ) {
		d_quat[i] = p_quat[i];
		vel_quat[i] = v_quat[i];
		scp_quat[i] = 0.0; // no torque with PHANToM
	}
	scp_quat[3] = 1.0;

	vel_quat_dt = dt_vel;
#endif
//printf("get report pos = %lf, %lf, %lf\n",pos[0],pos[1],pos[2]);

	// If button a event has happened, report changes
	vrpn_Button::report_changes();

        //Encode the position/orientation if there is a connection
        if(vrpn_Tracker::d_connection) {
            len = vrpn_Tracker::encode_to(msgbuf);
            if(vrpn_Tracker::d_connection->pack_message(len,
                timestamp,vrpn_Tracker::position_m_id,
                vrpn_Tracker::d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }

        //Encode the velocity/angular velocity if there is a connection
        if(vrpn_Tracker::d_connection) {
            len = vrpn_Tracker::encode_vel_to(msgbuf);
            if(vrpn_Tracker::d_connection->pack_message(len,
                timestamp,vrpn_Tracker::velocity_m_id,
                vrpn_Tracker::d_sender_id, msgbuf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }

        //Encode the force if there is a connection
	// Yes, the strange deletion of the returned buffer does make sense
	// given the semantics of the encode_to in ForceDevice, which for some
	// reason are different than those for other devices.
        if(vrpn_ForceDevice::d_connection) {
            buf = vrpn_ForceDevice::encode_force(len, d_force);
            if(vrpn_ForceDevice::d_connection->pack_message(len,timestamp,
                force_message_id,vrpn_ForceDevice::d_sender_id,
                buf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
	    delete buf;
	}
	// Encode the SCP (surface contact point) if there is a connection
	if (vrpn_ForceDevice::d_connection) {
	    buf = vrpn_ForceDevice::encode_scp(len, scp_pos, scp_quat);
	    if (vrpn_ForceDevice::d_connection->pack_message(len, timestamp,
		    scp_message_id, vrpn_ForceDevice::d_sender_id,
		    buf, vrpn_CONNECTION_LOW_LATENCY)) {
			    fprintf(stderr,"Phantom: cannot write message: tossing\n");
	    }
	    delete buf;
	}
	
  //      print_report();
    }
}



void vrpn_Phantom::reset(){
#ifdef	VRPN_USE_HDAPI
  instantBuzzEffect->stop();
  pointConstraint->stop();
#else
  if(trimesh->displayStatus()) {
    trimesh->clear();
  }
  phantom->stopEffect();
  pointConstraint->stop();
#endif
  for (int i = 0; i < MAXPLANE; i++) {
    if (planes[i]) {
      planes[i]->setActive(FALSE);
    }
  }
}

int vrpn_Phantom::register_change_handler(void *userdata,
			vrpn_PHANTOMPLANECHANGEHANDLER handler,
			vrpn_PHANTOMCHANGELIST	*&change_list)
{
	vrpn_PHANTOMCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		printf("vrpn_Phantom::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_PHANTOMCHANGELIST) == NULL) {
		printf(	"vrpn_Phantom::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = change_list;
	change_list = new_entry;

	return 0;
}

int vrpn_Phantom::unregister_change_handler(void *userdata,
			vrpn_PHANTOMPLANECHANGEHANDLER handler,
			vrpn_PHANTOMCHANGELIST	*&change_list){
	// The pointer at *snitch points to victim
	vrpn_PHANTOMCHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &change_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		printf("vrpn_Phantom::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Phantom::register_plane_change_handler(void *userdata,
				  vrpn_PHANTOMPLANECHANGEHANDLER handler){
  return register_change_handler(userdata, handler,
		plane_change_list);
}

int vrpn_Phantom::unregister_plane_change_handler(void *userdata,
				    vrpn_PHANTOMPLANECHANGEHANDLER handler){
  return unregister_change_handler(userdata, handler,
		plane_change_list);
}

int vrpn_Phantom::handle_plane_change_message(void *userdata, 
					      vrpn_HANDLERPARAM p){
	vrpn_Phantom *me = (vrpn_Phantom *)userdata;
	vrpn_Plane_PHANTOMCB	tp;
	vrpn_PHANTOMCHANGELIST *handler = me->plane_change_list;

	tp.msg_time = p.msg_time;

	decode_plane(p.buffer, p.payload_len, tp.plane, 
		&(tp.SurfaceKspring), &(tp.SurfaceKdamping),
		&(tp.SurfaceFdynamic), &(tp.SurfaceFstatic),
		&(tp.which_plane), &(tp.numRecCycles));

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

	return 0;
}

int vrpn_Phantom::handle_effects_change_message(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Phantom *me = (vrpn_Phantom *) userdata;
 
#ifdef	DYNAMIC_PLANES
    DynamicPlane *plane = (me->planes)[0];
#else
    Plane *plane = (me->planes)[0];
#endif

    float k_adhesion_norm, k_adhesion_lat, tex_amp, tex_wl,
	    buzz_amp, buzz_freq;

    if (decode_surface_effects(p.buffer, p.payload_len, &k_adhesion_norm, 
	    &k_adhesion_lat, &tex_amp, &tex_wl, &buzz_amp, &buzz_freq)){
	    fprintf(stderr, "Error: handle_effects_change_message(): can't decode\n");
    }
    
#ifdef	DYNAMIC_PLANES
    plane->setTextureAmplitude(tex_amp*1000.0);
    plane->setTextureWavelength(tex_wl*1000.0);
    plane->setBuzzAmplitude(buzz_amp*1000.0);
    plane->setBuzzFrequency(buzz_freq);
    plane->enableTexture(TRUE);
    plane->enableBuzzing(TRUE);
#endif

    return 0;
}

int vrpn_Phantom::handle_setVertex_message(void *userdata, 
					   vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  vrpn_int32 temp;
  int vertNum;
  float x,y,z;

  decode_vertex(p.buffer, p.payload_len, &temp, &x, &y, &z);
  vertNum=temp;

#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  if(me->trimesh->setVertex(vertNum,x*1000.0,y*1000.0,z*1000.0)) {
    return 0;
  } else {
    fprintf(stderr,"vrpn_Phantom: error in trimesh::setVertex\n");
    return -1;
  }
#endif
}

int vrpn_Phantom::handle_setNormal_message(void *userdata, 
					   vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  vrpn_int32 temp;
  int normNum;
  float x, y, z;

  decode_normal(p.buffer, p.payload_len, &temp, &x, &y, &z);
  normNum = temp;
    
#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  if(me->trimesh->setNormal(normNum,x,y,z))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setNormal\n");
    return -1;
  }
#endif
}

int vrpn_Phantom::handle_setTriangle_message(void *userdata, 
					     vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  int triNum, v0, v1, v2, n0, n1, n2;
  vrpn_int32 ltriNum, lv0, lv1, lv2, ln0, ln1, ln2;
  decode_triangle(p.buffer, p.payload_len, 
	  &ltriNum, &lv0, &lv1, &lv2, &ln0, &ln1, &ln2);
  triNum = ltriNum; v0 = lv0; v1 = lv1; v2 = lv2;
  n0 = ln0; n1 = ln1; n2 = ln2;

#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  if(me->trimesh->setTriangle(triNum,v0,v1,v2,n0,n1,n2))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setTriangle\n");
    return -1;
  }
#endif
}


int vrpn_Phantom::handle_removeTriangle_message(void *userdata, 
						vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  vrpn_int32 temp;
 
  decode_removeTriangle(p.buffer, p.payload_len, &temp);

  int triNum=temp;

#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  if(me->trimesh->removeTriangle(triNum))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::removeTriangle\n");
    return -1;
  }
#endif
}


int vrpn_Phantom::handle_updateTrimeshChanges_message(void *userdata, 
						      vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  float SurfaceKspring,SurfaceKdamping,SurfaceFdynamic,SurfaceFstatic;

  decode_updateTrimeshChanges(p.buffer, p.payload_len,
	  &SurfaceKspring, &SurfaceKdamping, &SurfaceFdynamic,
	  &SurfaceFstatic);


#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->updateChanges();

  myTrimesh->setSurfaceKspring(SurfaceKspring);
  myTrimesh->setSurfaceFstatic(SurfaceFstatic);
  myTrimesh->setSurfaceFdynamic(SurfaceFdynamic); 
  myTrimesh->setSurfaceKdamping(SurfaceKdamping);
  return 0;
#endif
}

int vrpn_Phantom::handle_setTrimeshType_message(void *userdata, 
					       vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  vrpn_int32 temp;

  decode_setTrimeshType(p.buffer, p.payload_len, &temp);

#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->setType((TrimeshType)temp);

  return 0;
#endif
}

int vrpn_Phantom::handle_clearTrimesh_message(void *userdata, 
					      vrpn_HANDLERPARAM){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  me->trimesh->clear();
  return 0;
#endif
}

int vrpn_Phantom::handle_transformTrimesh_message(void *userdata, 
					       vrpn_HANDLERPARAM p){

  vrpn_Phantom *me = (vrpn_Phantom *)userdata;

  float xformMatrix[16];

  decode_trimeshTransform(p.buffer, p.payload_len, xformMatrix);

  /* now we need to scale the transformation vector of our matrix from meters
	to millimeters */
  xformMatrix[3]*=1000.0;
  xformMatrix[7]*=1000.0;
  xformMatrix[11]*=1000.0;

#ifdef	VRPN_USE_HDAPI
  struct timeval now;
  gettimeofday(&now, NULL);
  me->send_text_message("Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return 0;
#else
  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->setTransformMatrix(xformMatrix);

  return 0;
#endif
}

int vrpn_Phantom::handle_forcefield_change_message(void *userdata,
						vrpn_HANDLERPARAM p){

  vrpn_Phantom *me = (vrpn_Phantom *)userdata;

  int i,j;

  decode_forcefield(p.buffer, p.payload_len,me->ff_origin,
	  me->ff_force, me->ff_jacobian, &(me->ff_radius));

  for (i=0;i<3;i++){
    me->ff_origin[i] *= (1000); // convert from meters->mm
    for (j=0;j<3;j++){
      me->ff_jacobian[i][j] *= (0.001f); // convert from 
					// dyne/m to dyne/mm
    }
  }

  me->ff_radius *= (1000); // convert meters to mm
  me->forceField->setForce(me->ff_origin, me->ff_force, 
	me->ff_jacobian, me->ff_radius);
  if (!me->forceField->isActive()){
#ifdef	VRPN_USE_HDAPI
#else
	me->phantom->setEffect(me->forceField);
	me->phantom->startEffect();
#endif
	me->forceField->start();
	//XXX Where is the associated stop, other than when the Phantom is destroyed?
  }
  return 0;
}

// static
int vrpn_Phantom::handle_resetOrigin_change_message(void * userdata,
                                                vrpn_HANDLERPARAM p) {
  vrpn_Phantom * me = (vrpn_Phantom *) userdata;
  me->resetPHANToM();
  return 0;

}

// ajout ONDIM
// check processing of each effect
int vrpn_Phantom::handle_custom_effect_change_message(void *userdata,
						vrpn_HANDLERPARAM p){

  vrpn_Phantom *me = (vrpn_Phantom *)userdata;

  vrpn_uint32 effectId;
  vrpn_float32* params = NULL;
  vrpn_uint32 nbParams;

  decode_custom_effect(p.buffer, p.payload_len,&effectId,&params,&nbParams);

  switch(effectId) {
	case BUZZ_EFFECT_ID:
		if (nbParams >= 1) {
			me->instantBuzzEffect->setAmplitude(params[0]);
		}
		if (nbParams >= 2) {
			me->instantBuzzEffect->setFrequency(params[1]);
		}
		if (nbParams >= 3) {
			me->instantBuzzEffect->setDuration(params[2]);
		}
		if (nbParams >= 6) {
			me->instantBuzzEffect->setDirection(params[3], params[4], params[5]);
		}

#ifdef	VRPN_USE_HDAPI
		me->instantBuzzEffect->start();
#else
		me->phantom->setEffect(me->instantBuzzEffect);
		me->phantom->startEffect();	
#endif

		break;
	default:
#ifdef	VRPN_USE_HDAPI
		me->instantBuzzEffect->stop();
#else
		me->phantom->stopEffect();
#endif
		break;
  }

  return 0;
}

// fin ajout ONDIM

#if 0

//
// constraint
//
//
// Would like to implement with gstConstraintEffect, but that has a
// THREE SECOND relaxation on startup and instant cutoff.  We need
// relaxation closer to tenth-second on startup and cutoff.
//

//static
int vrpn_Phantom::handle_enableConstraint_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  int enable;

  CHECK(decode_enableConstraint(p.buffer, p.payload_len, &enable));

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintMode_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  ConstraintGeometry mode;

  CHECK(decode_setConstraintMode(p.buffer, p.payload_len, &mode));

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintPoint_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float x, y, z;

  CHECK(decode_setConstraintPoint(p.buffer, p.payload_len, &x, &y, &z));

  // convert m to mm
  x *= 1000.0f;  y *= 1000.0f;  z *= 1000.0f;

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintLinePoint_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float x, y, z;

  CHECK(decode_setConstraintLinePoint(p.buffer, p.payload_len, &x, &y, &z));

  // convert m to mm
  x *= 1000.0f;  y *= 1000.0f;  z *= 1000.0f;

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintLineDirection_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float x, y, z;

  CHECK(decode_setConstraintLineDirection(p.buffer, p.payload_len, &x, &y, &z));

  // convert m to mm
  x *= 1000.0f;  y *= 1000.0f;  z *= 1000.0f;

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintPlanePoint_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float x, y, z;

  CHECK(decode_setConstraintPlanePoint(p.buffer, p.payload_len, &x, &y, &z));

  // convert m to mm
  x *= 1000.0f;  y *= 1000.0f;  z *= 1000.0f;

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintPlaneNormal_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float x, y, z;

  CHECK(decode_setConstraintPlaneNormal(p.buffer, p.payload_len, &x, &y, &z));

  // convert m to mm
  x *= 1000.0f;  y *= 1000.0f;  z *= 1000.0f;

  // TODO

  return 0;
}

//static
int vrpn_Phantom::handle_setConstraintKSpring_message
                      (void * userdata, vrpn_HANDLERPARAM p) {
  float k;

  CHECK(decode_setConstraintKSpring(p.buffer, p.payload_len, &k));

  // TODO

  return 0;
}

#endif  // 0


// static
int vrpn_Phantom::handle_update_rate_request (void * userdata,
                                                  vrpn_HANDLERPARAM p) {
  vrpn_Phantom * me = (vrpn_Phantom *) userdata;

  if (p.payload_len != sizeof(double)) {
    fprintf(stderr, "vrpn_Phantom::handle_update_rate_request:  "
                    "Got %d-byte message, expected %d bytes.\n",
            p.payload_len, sizeof(double));
    return -1;  // XXXshould this be a fatal error?
  }

  me->update_rate = ntohd(*((double *) p.buffer));
  fprintf(stderr, "vrpn_Phantom:  set update rate to %.2f hertz.\n",
          me->update_rate);
  return 0;
}


#ifndef	HDAPI
void phantomErrorHandler( int errnum, char *description, void *userdata)
{
	vrpn_Phantom *me = (vrpn_Phantom *) userdata;
	fprintf(stderr, "PHANTOM ERROR: %s\n", description);
	int i;
	for (i = 0; i < MAXPLANE; i++){
            if (me->planes[i] != NULL) {
		me->planes[i]->cleanUpAfterError();
            }
	}

	switch(errnum) {
		case GST_OUT_OF_RANGE_ERROR:
			me->sendError(FD_VALUE_OUT_OF_RANGE);
			me->errorCode = FD_VALUE_OUT_OF_RANGE;
			break;
		case GST_DUTY_CYCLE_ERROR:
			me->sendError(FD_DUTY_CYCLE_ERROR);
			me->errorCode = FD_DUTY_CYCLE_ERROR;
			break;
		case GST_PHANTOM_FORCE_ERROR:
                case -2:   // This is the number I get when I push through the surface.
			me->sendError(FD_FORCE_ERROR);
			me->errorCode = FD_FORCE_ERROR;
			break;
		default:
			me->sendError(FD_MISC_ERROR);
			me->errorCode = FD_MISC_ERROR;
			break;
	}
}
#endif

#endif
