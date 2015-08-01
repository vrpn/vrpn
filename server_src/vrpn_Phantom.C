//XXX Problem: Recovery time seems to have gotten itself unimplemented over time.

//XXX Buzzing and texture are not currently supported on HDAPI planes because
// only the basic Plane has been implemented within HDAPI, rather than the
// texturePlane.  This should be changeable in the future.

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "vrpn_Shared.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "vrpn_Connection.h"
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_ForceDeviceServer.h"
#include "vrpn_Phantom.h"

#include "ghost.h"
#ifdef	VRPN_USE_HDAPI
  #include <HD/hd.h>
  #include <HDU/hduError.h>
  #include <HL/hl.h>
  #include <GL/gl.h>  // Needed to deal with the fact that HL ties in to the OpenGL state.
  #pragma comment (lib, "opengl32.lib") // Needed for the OpenGL calls we use.
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

#ifdef	VRPN_USE_HDAPI
//--------------------------------------------------------------------------------
// BEGIN HDAPI large chunk

// Originally, all of the OpenHaptics code was implemented using HDAPI.  However,
// when the proxy-geometry code was needed that does friction for the planes, it
// was switched over to HL.  All of the low-level HDAPI force effects were turned
// into an HL custom force effect to avoid fighting between the HL and HDAPI
// servo callbacks.  The HDAPI servo callback was basically turned into an HL
// custom force callback.  This custom force callback handles everything but the
// planes.  The planes are handled within the Plane class, which knows how to
// render itself in HL.

// Structure to hold pointers to active force-generating objects,
// and the current force and surface contact point.

static void HLCALLBACK computeForceCB(HDdouble force[3], HLcache *cache, void *userdata);

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

// Initialize the HL library, so that we can do custom shapes (the
// plane is implemented this way) and later maybe add in the
// polygonal-object implementation.
void vrpn_Phantom::initHL(HHD phantom)
{
  // Create a haptic context for the device. The haptic context maintains 
  // the state that persists between frame intervals and is used for
  // haptic rendering.
  hHLRC = hlCreateContext(phantom);
  hlMakeCurrent(hHLRC);

  hlTouchableFace(HL_FRONT);
  hlTouchModel(HL_CONTACT);

  // Get IDs for all of the planes and other objects
  int i;
  for(i=0; i<MAXPLANE; i++ ) {
    planes[i]->getHLId();
  }

  effectId = hlGenEffects(1);
}

void vrpn_Phantom::tearDownHL(void)
{
  // Release the IDs for the planes and other objects
  // Get IDs for all of the planes and other objects
  int i;
  for(i=0; i<MAXPLANE; i++ ) {
    planes[i]->releaseHLId();
  }

  hlDeleteEffects(effectId, 1);

  // No longer using the context.
  hlMakeCurrent(NULL);
  hlDeleteContext(hHLRC);
}

// The work of reading the entire device state is done in a callback
// handler that is scheduled synchronously between the force-calculation
// thread and the main (communication) thread.  This makes sure we don't
// have a collision between the two.

static HDCallbackCode HDCALLBACK readDeviceState(void *pUserData)
{
  HDAPI_state *state = (HDAPI_state *)pUserData;

  hdGetIntegerv(HD_CURRENT_BUTTONS, &state->buttons);
  hdGetDoublev(HD_CURRENT_TRANSFORM, &state->pose[0][0]);
  hdGetDoublev(HD_LAST_TRANSFORM, &state->last_pose[0][0]);
  hdGetIntegerv(HD_INSTANTANEOUS_UPDATE_RATE, &state->instant_rate);
  hdGetDoublev(HD_NOMINAL_MAX_STIFFNESS, &state->max_stiffness);
  hdGetDoublev(HD_CURRENT_FORCE, &state->current_force[0]);

  return HD_CALLBACK_DONE;
}

// This callback handler is called once during each haptic schedule frame.
// It handles calculation of the force based on the currently-active
// forcefields, constraints, effects, and other force-generating objects.
// It relies on the above static global data structure to keep track of
// which objects are active.
// The planes are handled as objects with custom intersection routines
// so they can make use of the HL-supplied friction and so forth.  They
// are not handled in this function.

static void HLCALLBACK computeForceCB(HDdouble force[3], HLcache *cache, void *userdata)
{
  //---------------------------------------------------------------
  // Handle the custom force calculation.

  // Is there a better, HL way to read this state?  There is a cache version,
  // but then we end up with two different ways to read the state: one from the
  // main routine to get data to send back to the client and another to read it
  // for our use here.  Unless this is a problem, lets stick with the HDAPI method
  // in both places to avoid code drift.  Actually, we can't get the last state
  // using HL, so we can't compute things like velocity, so we're better off
  // sticking with HDAPI if we can.
  HDAPI_state	state;
  hdScheduleSynchronous(readDeviceState, &state, HD_MAX_SCHEDULER_PRIORITY);

  // Remember: g_active_force_object_list holds both the lists
  // of objects that generate forces and the current status of
  // both the force and the surface contact point, if any.
  // To start with, we have no force.
  g_active_force_object_list.CurrentForce.set(0,0,0);

  // The plane forces are computed separately: they are included in the HL
  // display as their own objects and they will add in their contributions
  // through that mechanism.

  // Need to add in any constraint forces
 unsigned i;
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

  // Add the results of the above force calculations to the currently-displayed on
  // within the standard HL rendering.
  force[0] += g_active_force_object_list.CurrentForce[0];
  force[1] += g_active_force_object_list.CurrentForce[1];
  force[2] += g_active_force_object_list.CurrentForce[2];
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
	//plane->setNumRecCycles((int)p2.numRecCycles);
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

  // Turn off everything in HL.
  tearDownHL();

  hdDisableDevice(phantom);

  HDErrorInfo error;
  phantom = hdInitDevice(HD_DEFAULT_DEVICE);
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to initialize haptic device");
      phantom = -1;
  } else {
     initHL(phantom);
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

vrpn_Phantom::vrpn_Phantom(char *name, vrpn_Connection *c, float hz, const char * newsconf)
		:vrpn_Tracker(name, c),vrpn_Button_Filter(name,c),
		 vrpn_ForceDeviceServer(name,c), update_rate(hz),
#ifndef	VRPN_USE_HDAPI
                 scene(NULL), 
                 rootH(NULL),
                 hapticScene(NULL),
                 phantom(NULL),
                 //HHLRC(NULL),
                 //trimesh(NULL),
#endif
                 pointConstraint(NULL),
                 forceField(NULL),
		 plane_change_list(NULL),
		 // ajout ONDIM
		 // add each effect
		 instantBuzzEffect(NULL)
		 // fin ajout ONDIM
{  
#ifdef	VRPN_USE_HDAPI
	// Jean SIMARD <jean.simard@limsi.fr>
	// Initialize the 'sconf' field
	strcpy( sconf, newsconf );
  vrpn_Button_Filter::num_buttons = 2;  // Omni has 2 buttons, others have 1. XXX This overestimates it
  button_0_bounce_count = 0;
  button_1_bounce_count = 0;
#else
  vrpn_Button_Filter::num_buttons = 1;
#endif

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
	// Jean SIMARD <jean.simard@limsi.fr>
	// I don't made some modifications here because I never used Ghost but I 
	// suppose that 'sconf' should be use instead of "Default PHANToM".
  phantom = new gstPHANToM("Default PHANToM");
  phantomAxis = new gstSeparator;
  phantomAxis->addChild(phantom);
  // Ghost 3.0 spec says we should make sure construction succeeded. 
  if(!phantom->getValidConstruction()) {
      fprintf(stderr, "ERROR: Invalid Phantom object created\n");
  }
  /* We add the phantom to the root instead of the hapticScene (which
     is the child of the root) because when we turn haptics off
     via the 'H' key, we do not want to remove the phantom from the
     scene.  Pressing the 'H' key removes hapticScene and
     its children from the scene graph. */
  rootH->addChild(phantomAxis);
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

  SurfaceKspring= 0.8f;
  SurfaceFdynamic = 0.3f; 
  SurfaceFstatic = 0.7f;
  SurfaceKdamping = 0.001f;

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
  addObject(0,-1);
  //trimesh = new Trimesh();
  //trimesh->addToScene(hapticScene);
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

  // Open the Phantom and get all of the contexts we need.
  // This is also done in resetPHANToM.
#ifdef	VRPN_USE_HDAPI
  HDErrorInfo error;
	// Jean SIMARD <jean.simard@limsi.fr>
	// Modify the configuration name called by 'hdInitDevice'
  phantom = hdInitDevice(sconf);
  if (HD_DEVICE_ERROR(error = hdGetError())) {
      hduPrintError(stderr, &error, "Failed to initialize haptic device");
      phantom = -1;
  } else {

     // XXX When this is removed, we lose the fighting
     initHL(phantom);
  }
#else
  scene->startServoLoop();
#endif
}

vrpn_Phantom::~vrpn_Phantom()
{
#ifdef	VRPN_USE_HDAPI
   if (phantom != -1) {
      tearDownHL();
      hdDisableDevice(phantom);
   }
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
//      send button message (done in vrpn_Button_Filter::report_changes)
//          NOT vrpn_Button::report_changes.
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

#ifdef	VRPN_USE_HDAPI
    // Okay, this is REALLY HORRIBLE.  It turns out that HL ties itself
    // intimately into OpenGL.  It gets its original workspace to haptic
    // mapping by looking at the OpenGL Modelview matrix when the haptic
    // frame is begun.  To avoid having the graphics code interfere with
    // the haptic code in an application that runs this server within
    // the same process, we set the ModelView matrix to the identity here
    // (after storing it).  We then restore it after the haptic rendering
    // loop.  We also store the current matrix mode so that we can restore
    // it afterwards.  We want to leave the OpenGL state exactly like it
    // was before we did the haptic rendering.
    glPushAttrib(GL_TRANSFORM_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    //---------------------------------------------------------------
    // Handle the HL portion of the force calculation.  This currently
    // includes only the planes.  This rendering does not work if we
    // put it inside the HD force callback routine, so we do it here
    // in the mainloop() function.  In fact, we should only have to
    // re-render when something changes, but it doesn't seem to hurt
    // to re-render all the time.

    hlBeginFrame();

      // Render each of the active force objects.
      unsigned i;
      for (i = 0; i < g_active_force_object_list.Planes.size(); i++) {
        g_active_force_object_list.Planes[i]->renderHL();
      }

      // Render the effect forces
      hlCallback(HL_EFFECT_COMPUTE_FORCE, (HLcallbackProc) computeForceCB, NULL);
      hlStartEffect(HL_EFFECT_CALLBACK, effectId);

    hlEndFrame();

    // Put the OpenGL state back where it was.
    glPopMatrix();
    glPopAttrib();
#endif

    //set if it is time to generate a new report 
    vrpn_gettimeofday(&current_time, NULL);
    if(vrpn_TimevalDuration(current_time,timestamp) >= 1000000.0/update_rate) {
		
        //update the time
        timestamp = current_time;

	//------------------------------------------------------------------------
	// Read the device state and convert all values into the appropriate
	// units for VRPN and store them into local state.
#ifdef	VRPN_USE_HDAPI
	HDAPI_state state;
   // if phantom is null, next call crashes. 
   if (phantom == -1) return;
	hdScheduleSynchronous(readDeviceState, &state, HD_MIN_SCHEDULER_PRIORITY);

	// Convert buttons to VRPN.  Debounce them, making sure they have
        // stabilized at their current value for at least 5 cycles before
        // switching (this was needed for a particular Phantom Omni whose
        // button glitched a lot).  This is done by setting bounce count to
        // zero whenever the button matches what is stored and incrementing
        // it whenever it is different until 5 is reached.
        int button_0_state = ((state.buttons & HD_DEVICE_BUTTON_1) == 0) ? 0 : 1;
        int button_1_state = ((state.buttons & HD_DEVICE_BUTTON_2) == 0) ? 0 : 1;

        if (button_0_state == buttons[0]) {
          button_0_bounce_count = 0;
        } else if (++button_0_bounce_count >= 5) {
          buttons[0] = button_0_state;
          button_0_bounce_count = 0;
	}
        if (button_1_state == buttons[1]) {
          button_1_bounce_count = 0;
        } else if (++button_1_bounce_count >= 5) {
          buttons[1] = button_1_state;
          button_1_bounce_count = 0;
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

	// Store the current force being generated.
	// XXX This does not include forces being generated by the
	// plane objects within HL.
	d_force[0] = state.current_force[0];
	d_force[1] = state.current_force[1];
	d_force[2] = state.current_force[2];

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
	//  we have to use vrpn_Button_Filter, not vrpn_Button, or
	//  we lose the toggle functionality.
	vrpn_Button_Filter::report_changes();

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
/*  Trimesh *mesh=GetObjectMesh(0);
  if(mesh)
  {
	if(mesh->displayStatus()) 
		mesh->clear();
  }*/
  scene->stopServoLoop();
  bool found = m_hObjectList.MoveFirst();
  while(found)
  {
		removeObject(m_hObjectList.GetCurrentKey());
		found=m_hObjectList.MoveFirst();
  }
  m_hObjectList.Clear();
  scene->startServoLoop();
  phantom->stopEffect();
  pointConstraint->stop();
#endif
  for (int i = 0; i < MAXPLANE; i++) {
    if (planes[i]) {
      planes[i]->setActive(FALSE);
    }
  }
  forceField->stop();
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

#ifndef VRPN_USE_HDAPI
gstSeparator *vrpn_Phantom::GetObject(vrpn_int32 objNum)
{
	vrpn_DISPLAYABLEOBJECT *obj=m_hObjectList.Find(objNum);
	if(obj)
	{
		return (gstSeparator*)(obj->m_pObject);
	}
	return 0;
}
#endif

#ifndef VRPN_USE_HDAPI
Trimesh *vrpn_Phantom::GetObjectMesh(vrpn_int32 objNum)
{
	vrpn_DISPLAYABLEOBJECT *obj=m_hObjectList.Find(objNum);
	if(obj)
	{
		return (Trimesh*)(obj->m_pObjectMesh);
	}
	return 0;
}
#endif

bool vrpn_Phantom::addObject(vrpn_int32 objNum, vrpn_int32 ParentNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::addObject: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
        Trimesh *newObjectMesh= GetObjectMesh(objNum);
	if( newObjectMesh)
		removeObject(objNum);
	newObjectMesh = new Trimesh();
	gstSeparator *newObject = new gstSeparator();
	vrpn_DISPLAYABLEOBJECT *elem= new vrpn_DISPLAYABLEOBJECT;
	elem->m_ObjectType=1;
	elem->m_pObject=newObject;
	elem->m_pObjectMesh=newObjectMesh;
	newObjectMesh->addToScene(newObject);
	m_hObjectList.Add(objNum,elem);
	gstSeparator *parentObj=GetObject(ParentNum);
	if(parentObj)
		parentObj->addChild(newObject);
	else
		hapticScene->addChild(newObject);
	
	return true;
#endif
}


bool vrpn_Phantom::addObjectExScene(vrpn_int32 objNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::addObjectExScene: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *newObjectMesh= GetObjectMesh(objNum);
	if( newObjectMesh)
		removeObject(objNum);
	newObjectMesh = new Trimesh();
	gstSeparator *newObject = new gstSeparator();
	vrpn_DISPLAYABLEOBJECT *elem= new vrpn_DISPLAYABLEOBJECT;
	elem->m_ObjectType=1;
	elem->m_pObject=newObject;
	elem->m_pObjectMesh=newObjectMesh;
	newObjectMesh->addToScene(newObject);
	m_hObjectList.Add(objNum,elem);
	rootH->addChild(newObject);
	
	return true;
#endif
}

bool vrpn_Phantom::setVertex(vrpn_int32 objNum, vrpn_int32 vertNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setVertex: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
		return obj->setVertex(vertNum,x*1000.0f,y*1000.0f,z*1000.0f);
	else
		return false;
#endif
}

bool vrpn_Phantom::setNormal(vrpn_int32 objNum, vrpn_int32 normNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setNormal: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
		return obj->setNormal(normNum,x*1000.0f,y*1000.0f,z*1000.0f);
	else
		return false;
#endif
}

bool vrpn_Phantom::setTriangle(vrpn_int32 objNum, vrpn_int32 triNum,vrpn_int32 vert0,vrpn_int32 vert1,vrpn_int32 vert2,
	  vrpn_int32 norm0,vrpn_int32 norm1,vrpn_int32 norm2)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setTriangle: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
		return obj->setTriangle(triNum,vert0,vert1,vert2,norm0,norm1,norm2);
	else
		return false;
#endif
}

bool vrpn_Phantom::removeTriangle(vrpn_int32 objNum, vrpn_int32 triNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::removeTriangle: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
		return obj->removeTriangle(triNum);
	else 
		return false;
#endif
}

// should be called to incorporate the above changes into the 
// displayed trimesh 
bool vrpn_Phantom::updateTrimeshChanges(vrpn_int32 objNum,vrpn_float32 kspring, vrpn_float32 kdamp, vrpn_float32 fdyn, vrpn_float32 fstat)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::updateTrimeshChanges: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
	{
		if(obj->updateChanges())
		{
			obj->setSurfaceKspring(kspring);
			obj->setSurfaceFstatic(kdamp);
			obj->setSurfaceFdynamic(fdyn); 
			obj->setSurfaceKdamping(fstat);
			return true;
		}
		return true;
	}
	else 
		return false;

#endif
}

// set the type of trimesh
bool vrpn_Phantom::setTrimeshType(vrpn_int32 objNum,vrpn_int32 type)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setTrimeshType: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	if(obj)
	{
		obj->setType((TrimeshType)type);
		return true;
	}
	else
		return false;
#endif
}

// set the trimesh's homogen transform matrix (in row major order)
bool vrpn_Phantom::setTrimeshTransform(vrpn_int32 objNum, vrpn_float32 homMatrix[16])
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setTrimeshTransform: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *obj=GetObjectMesh(objNum);
	/* now we need to scale the transformation vector of our matrix from meters
	to millimeters */
	homMatrix[3]*=1000.0;
	homMatrix[7]*=1000.0;
	homMatrix[11]*=1000.0;
	if(obj)
	{
		obj->setTransformMatrix(homMatrix);
		return true;
	}
	else
		return false;
#endif
}

// set position of an object
bool vrpn_Phantom::setObjectPosition(vrpn_int32 objNum, vrpn_float32 Pos[3])
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setObjectPosition: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	gstSeparator *obj=GetObject(objNum);
	if(obj)
	{
		obj->setTranslate(Pos[0]*1000.0f,Pos[1]*1000.0f,Pos[2]*1000.0f);
		return true;
	}
	else
		return false;
#endif
}

// set orientation of an object
bool vrpn_Phantom::setObjectOrientation(vrpn_int32 objNum, vrpn_float32 axis[3], vrpn_float32 angle)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setObjectOrientation: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	gstSeparator *obj=GetObject(objNum);
	if(obj)
	{
		gstVector Axis(axis[0],axis[1],axis[2]);
		obj->setRotation(Axis,angle);
		return true;
	}
	else
		return false;
#endif
}

// set Scale of an object
bool vrpn_Phantom::setObjectScale(vrpn_int32 objNum, vrpn_float32 Scale[3])
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setObjectScale: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	gstSeparator *obj=GetObject(objNum);
	if(obj)
	{
		obj->setScale(Scale[0]/*,Scale[1],Scale[2]*/);
		return true;
	}
	else
		return false;
#endif
}

// remove an object from the scene
bool vrpn_Phantom::removeObject(vrpn_int32 objNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::removeObject: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	gstSeparator *obj=GetObject(objNum);
	if(obj)
	{
                // Lock the scene when doing this change and then unlock it
                // when we're done to avoid errors caused by our changes during
                // a traversal of the scene.  Do not stop the servoloop here, which will
                // remove the other objects from the scene.
		scene->lock();
		gstSeparator *parent = ( gstSeparator* )obj->getParent();
		parent->removeChild( obj );

		for ( int i = obj->getNumChildren() ; i > 0 ; i-- )
		{
			gstSeparator *child = (gstSeparator*)obj->getChild( i - 1 );
			if(!child->isOfType(gstShape::getClassTypeId() ))
			{
				obj->removeChild( i - 1 );
				parent->addChild(child);
				//delete child;
			}
			/*if ( child->isOfType( gwpVRmentPolyMesh::getClassTypeId() ) )
				delete child;
			else
				parent->addChild( child->AsTransform() );*/
		}

		Trimesh *m=GetObjectMesh(objNum);
		delete m;
		delete obj;		
		m_hObjectList.Remove( objNum );
		scene->unlock();
		return true;
	}
	else
		return false;
#endif
}

bool vrpn_Phantom::clearTrimesh(vrpn_int32 objNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::clearTrimesh: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	Trimesh *objmesh=GetObjectMesh(objNum);
	if(objmesh)
	{
		objmesh->clear();
		return true;
	}
	else
		return false;
#endif
}


/** Functions to organize the scene	**********************************************************/
// Change The parent of an object
bool vrpn_Phantom::moveToParent(vrpn_int32 objNum, vrpn_int32 ParentNum)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::moveToParent: Trimesh not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	gstSeparator *obj=GetObject(objNum);
	if(obj)
	{
		gstSeparator *parent = (gstSeparator*)obj->getParent();
		parent->removeChild((gstTransform*)obj);
		parent = GetObject(ParentNum);
		if(parent)
			parent->addChild(obj);
		else
			hapticScene->addChild(obj);
		return true;
	}
	else
		return false;
#endif
}

// Set the Origin of the haptic device
bool vrpn_Phantom::setHapticOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setHapticOrigin: Not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	phantomAxis->setPosition(Pos[0]*1000.0f,Pos[1]*1000.0f,Pos[2]*1000.0f);
	phantomAxis->setRotation(gstVector(axis[0],axis[1],axis[2]),angle);
	return true;
#endif
}

// Set the scale factor of the haptic device
bool vrpn_Phantom::setHapticScale(vrpn_float32 Scale)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setHapticScale: Not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
	phantomAxis->setScale(Scale);
	return true;
#endif
}

// Set the Origin of the haptic scene
bool vrpn_Phantom::setSceneOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setSceneOrigin: Not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
//	static FILE *f=fopen( "sceneorigin.txt", "w" );

	gstTransformMatrix matrix;
	matrix.setTranslation( Pos[0]*1000.0f, Pos[1]*1000.0f, Pos[2]*1000.0f );

	matrix.setRotation(gstVector( axis[0], axis[1], axis[2] ), angle );
	
	hapticScene->setTransformMatrix( matrix.getInverse() );
	return true;
#endif
}

// make an object touchable or not
bool vrpn_Phantom::setObjectIsTouchable(vrpn_int32 objNum, vrpn_bool IsTouchable)
{
#ifdef  VRPN_USE_HDAPI
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  send_text_message("vrpn_Phantom::setObjectIsTouchable: Not supported under HDAPI",now, vrpn_TEXT_ERROR);
  return false;
#else
  #ifdef sgi
    // For some reason, this function seems to be missing from GHOST 3.1 on the
    // SGI architecture, even though it is described in the manual.
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    send_text_message("vrpn_Phantom::setObjectIsTouchable: Not supported under SGI",now, vrpn_TEXT_ERROR);
  #else
	gstSeparator *obj=GetObject(objNum);
	obj->setTouchableByPHANToM(IsTouchable) ;
	return true;
  #endif
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

  me->update_rate = vrpn_ntohd(*((double *) p.buffer));
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
