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
#include "texture_plane.h"
#include "trimesh.h"
#include "constraint.h"
#include "forcefield.h"
#include "quat.h"

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

void vrpn_Phantom::handle_plane(void *userdata,const vrpn_Plane_PHANTOMCB &p)
{
/*  printf("MY Plane is %lfX + %lfY + %lfZ + %lf = 0 \n",p.plane[0],
	p.plane[1],p.plane[2],p.plane[3]);
  printf("surface is %lf,%lf,%lf,%lf\n",p.SurfaceKspring,p.SurfaceKdamping,
	  p.SurfaceFdynamic, p.SurfaceFstatic);
     printf("which plane = %d\n", p.which_plane);
*/
    vrpn_Phantom *me = (vrpn_Phantom *) userdata;
 
    gstPHANToM *phan = me->phantom;
    int which_plane = p.which_plane;
    DynamicPlane *plane = (me->planes)[which_plane];

    // If the plane's normal is (0,0,0), it is a stop surface command
	// turn off the surface.
    if(p.plane[0]== 0 && p.plane[1] ==0  && p.plane[2] == 0 ) {
		plane->setActive(FALSE);
    }
    else {
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
	static int recentError = false;
	static timeval timeOfLastReport;
	timeval currTime;

	gettimeofday(&currTime, NULL);

	// if this is the first time check_parameters has been called then initialize
	// timeOfLastReport, otherwise this has no effect on behavior
	if (recentError == false)
		timeOfLastReport.tv_sec = currTime.tv_sec - 6;
	// if recentError is true then change it to false if enough time has elapsed
	else if (currTime.tv_sec - timeOfLastReport.tv_sec > 5)
		recentError = false;

    if (p->SurfaceKspring <= 0){
		if (!recentError)
			fprintf(stderr,"Error: Kspring = %f <= 0\n", p->SurfaceKspring);
		p->SurfaceKspring = 0.001f;
		recentError = true;
    }
    else if (p->SurfaceKspring > 1.0){
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
    }
    else if (p->SurfaceFstatic > 1.0){
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
    }
    else if (p->SurfaceKdamping > 0.005){
		if (!recentError)
			printf("Error: Kdamping = %f > 0.005\n", p->SurfaceKdamping);
		p->SurfaceKdamping = 0.005f;
		recentError = true;
    }
}

// This function reinitializes the PHANToM (resets the origin)
void vrpn_Phantom::resetPHANToM(void)
{
	scene->stopServoLoop();
	rootH->removeChild(phantom);
	delete(phantom);
	phantom = new gstPHANToM("Default PHANToM", TRUE);
	rootH->addChild(phantom);
	Sleep(500);
	scene->startServoLoop();
}

// return the position of the phantom stylus relative to base
// coordinate system
void vrpn_Phantom::getPosition(double *vec, double *orient)
{
    for (int i = 0; i < 3; i++){
		vec[i] = pos[i];
		orient[i] = quat[i];
	}
	orient[3] = quat[3];

}

vrpn_Phantom::vrpn_Phantom(char *name, vrpn_Connection *c, float hz)
		:vrpn_Tracker(name, c),vrpn_Button(name,c),
		 vrpn_ForceDevice(name,c), update_rate(hz),
		 plane_change_list(NULL){  
  num_buttons = 1;  // only has one button

  timestamp.tv_sec = 0;
  timestamp.tv_usec = 0;

  scene = new gstScene;

  /* make it so simulation loop doesn't exit if remote switch is released */
  scene->setQuitOnDevFault((gstBoolean)FALSE);

  
  /* disable popup error dialogs */
  printErrorMessages(FALSE);
  setErrorCallback(phantomErrorHandler, (void *)this);

  /* Create the root separator. */
  rootH = new gstSeparator;
  scene->setRoot(rootH);
  
  /* Create the phantom object.  When this line is processed, 
     the phantom position is zeroed. */
  phantom = new gstPHANToM("Default PHANToM");
  /* We add the phantom to the root instead of the hapticScene (whic
     is the child of the root) because when we turn haptics off
     via the 'H' key, we do not want to remove the phantom from the
     scene.  Pressing the 'H' key removes hapticScene and
     its children from the scene graph. */
  rootH->addChild(phantom);
  hapticScene = new gstSeparator;     
  rootH->addChild(hapticScene);

  pointConstraint = new ConstraintEffect();
  phantom->setEffect(pointConstraint);
  forceField = new ForceFieldEffect();

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

  for(int i=0; i<MAXPLANE; i++ ) {
    planes[i] = new DynamicPlane();
    planes[i]->setSurfaceKspring(SurfaceKspring);
    planes[i]->setSurfaceFdynamic(SurfaceFdynamic);
    planes[i]->setSurfaceFstatic(SurfaceFstatic);
    planes[i]->setSurfaceKdamping(SurfaceKdamping);
	planes[i]->setBuzzAmplitude(1000.0*SurfaceBuzzAmp);
	planes[i]->setBuzzFrequency(SurfaceBuzzFreq);
	planes[i]->setTextureAmplitude(1000.0*SurfaceTextureAmplitude);
	planes[i]->setTextureWavelength(1000.0*SurfaceTextureWavelength);
    hapticScene->addChild(planes[i]);
  }


  which_plane = 0;
 
  trimesh = new Trimesh();

  trimesh->addToScene(hapticScene);

  //  status= TRACKER_RESETTING;
  gettimeofday(&(vrpn_ForceDevice::timestamp),NULL);

  if (vrpn_ForceDevice::connection->register_handler(plane_message_id, 
	handle_plane_change_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(plane_effects_message_id,
	handle_effects_change_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setVertex_message_id, 
	handle_setVertex_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setNormal_message_id, 
	handle_setNormal_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setTriangle_message_id, 
	handle_setTriangle_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(removeTriangle_message_id, 
	handle_removeTriangle_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(updateTrimeshChanges_message_id, 
	handle_updateTrimeshChanges_message, this, vrpn_ForceDevice::  my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(transformTrimesh_message_id, 
	handle_transformTrimesh_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setTrimeshType_message_id, 
	handle_setTrimeshType_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(clearTrimesh_message_id, 
	handle_clearTrimesh_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(set_constraint_message_id,
	handle_constraint_change_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }

  if (vrpn_ForceDevice::connection->register_handler(forcefield_message_id,
	handle_forcefield_change_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_Tracker::connection->register_handler(reset_origin_m_id,
        handle_resetOrigin_change_message, this, vrpn_Tracker::my_id)) {
                fprintf(stderr,"vrpn_Phantom:can't register handler\n");
                vrpn_Tracker::connection = NULL;
  }


  this->register_plane_change_handler(this, handle_plane);

  if (vrpn_Tracker::register_server_handlers())
    fprintf(stderr, "vrpn_Phantom: couldn't register xform request handlers\n");

  // UNREGISTER the default handler, register our own.
  // Must be called after vrpn_Tracker::register_server_handlers()
  // because unregister_handler doesn't seem to deal with the case of
  // an unregistering a non-registered handler properly?

  // We need to unregister the address of this-as-a-vrpn_Tracker, not this.

  vrpn_Tracker * track_this = this;

  if (vrpn_Tracker::connection->unregister_handler
       (update_rate_id,
        vrpn_Tracker::handle_update_rate_request, track_this,
        vrpn_Tracker::my_id)) {
                fprintf(stderr,"vrpn_Phantom:  "
                        "Can't unregister default update-rate handler\n");
                vrpn_Tracker::connection = NULL;
  } else
      fprintf(stderr, "vrpn_Phantom unregistered old update-rate handler.\n");


  if (vrpn_Tracker::connection->register_handler
       (update_rate_id, handle_update_rate_request, this,
        vrpn_Tracker::my_id)) {
                fprintf(stderr, "vrpn_Phantom:  "
                                "Can't register update-rate handler\n");
                vrpn_Tracker::connection = NULL;
  } else
      fprintf(stderr, "vrpn_Phantom registered new update-rate handler.\n");

  scene->startServoLoop();

  }


void vrpn_Phantom::print_report(void)
{ 
   printf("----------------------------------------------------\n");
   vrpn_ForceDevice::print_report();

 //  printf("Timestamp:%ld:%ld\n",timestamp.tv_sec, timestamp.tv_usec);
   printf("Pos      :%lf, %lf, %lf\n", pos[0],pos[1],pos[2]);
   printf("Quat     :%lf, %lf, %lf, %lf\n", quat[0],quat[1],quat[2],quat[3]);
   //printf("Force	:%lf, %lf, %lf\n", force[0],force[1], force[2]);
}
void vrpn_Phantom::get_report(void)
{
	double x,y,z;
	double dt_vel = .10, dt_acc = .10;
	q_matrix_type rot_matrix;
	q_type p_quat, v_quat;
	//q_type a_quat; // angular information (pos, vel, acc)
	double angVelNorm;
	// double angAccNorm;
	int i,j;

	gstPoint scpt, pt;
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

        gstVector phantomVel = phantom->getVelocity();//mm/sec
        phantomVel.getValue(x, y, z);
        vel[0] = x/1000.0;      // convert from mm to m
        vel[1] = y/1000.0;
        vel[2] = z/1000.0;
        gstVector phantomAcc = phantom->getAccel(); //mm/sec^2
        phantomAcc.getValue(x,y,z);
        acc[0] = x/1000.0;      // convert from mm to m
        acc[1] = y/1000.0;
        acc[2] = z/1000.0;

	gstVector PhantomForce = phantom->getReactionForce();
        PhantomForce.getValue(x,y,z);
	force[0] = x;
	force[1] = y;
	force[2] = z;

		// transform rotation matrix to quaternion
	gstTransformMatrix PhantomRot=phantom->getRotationMatrix();	
	
	for(i=0; i<4; i++) {
	    for(j=0;j<4;j++) {
		rot_matrix[i][j] = PhantomRot.get(i,j);
	    }
	}
	q_from_row_matrix(p_quat,rot_matrix);

	// need to work out expression for acceleration quaternion

	//gstVector phantomAngAcc = phantom->getAngularAccel();//rad/sec^2
        gstVector phantomAngVel = phantom->getAngularVelocity();//rad/sec
        angVelNorm = phantomAngVel.norm();
        phantomAngVel.normalize();

	// compute angular velocity quaternion
        phantomAngVel.getValue(x,y,z);
        q_make(v_quat, x, y, z, angVelNorm*dt_vel);
        // set v_quat = v_quat*quat
        q_mult(v_quat, v_quat, quat);


	for(i=0;i<4;i++ ) {
		quat[i] = p_quat[i];
		vel_quat[i] = v_quat[i];
		scp_quat[i] = 0.0; // no torque with PHANToM
	}
	scp_quat[3] = 1.0;

	vel_quat_dt = dt_vel;
//printf("get report pos = %lf, %lf, %lf\n",pos[0],pos[1],pos[2]);
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
	char	msgbuf[1000];
	char    *buf;
	vrpn_int32	len;

    //check button status
    if(phantom->getStylusSwitch() ) {
	    buttons[0] = 1;   //button is pressed
	//	printf("button is pressed.\n");
    } else {
	    buttons[0]= 0;
	//	printf("button is released. \n");
    }

	// if button event happens, report changes
	report_changes();

    //set if its time to generate a new report 
    gettimeofday(&current_time, NULL);
    if(duration(current_time,timestamp) >= 1000000.0/update_rate) {
		
        //update the time
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        // Read the current PHANToM position, velocity, accel., and force
        // Put the information into pos/quat
        get_report();

        //Encode the position/orientation if there is a connection
        if(vrpn_Tracker::connection) {
            len = vrpn_Tracker::encode_to(msgbuf);
            if(vrpn_Tracker::connection->pack_message(len,
                timestamp,vrpn_Tracker::position_m_id,
                vrpn_Tracker::my_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }

        //Encode the velocity/angular velocity if there is a connection
        if(vrpn_Tracker::connection) {
            len = vrpn_Tracker::encode_vel_to(msgbuf);
            if(vrpn_Tracker::connection->pack_message(len,
                timestamp,vrpn_Tracker::velocity_m_id,
                vrpn_Tracker::my_id, msgbuf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }
        //Encode the acceleration/angular acceleration if there is a connection
/*        if (vrpn_Tracker::connection) {
            len = vrpn_Tracker::encode_acc_to(msgbuf);
            if(vrpn_Tracker::connection->pack_message(len,
                timestamp,vrpn_Tracker::acc_m_id,
                vrpn_Tracker::my_id, msgbuf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }
*/
        //Encode the force if there is a connection
        if(vrpn_ForceDevice::connection) {
            buf = vrpn_ForceDevice::encode_force(len, force);
            if(vrpn_ForceDevice::connection->pack_message(len,timestamp,
                force_message_id,vrpn_ForceDevice::my_id,
                buf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
			delete buf;
        }
	// Encode the SCP if there is a connection
		if (vrpn_ForceDevice::connection) {
			buf = vrpn_ForceDevice::encode_scp(len, scp_pos, scp_quat);
			if (vrpn_ForceDevice::connection->pack_message(len, timestamp,
				scp_message_id, vrpn_ForceDevice::my_id,
				buf, vrpn_CONNECTION_LOW_LATENCY)) {
					fprintf(stderr,"Phantom: cannot write message: tossing\n");
			}
			delete buf;
		}
	
  //      print_report();
    }
}



void vrpn_Phantom::reset(){
	if(trimesh->displayStatus())
		trimesh->clear();
	for (int i = 0; i < MAXPLANE; i++)
	   if (planes[i]) planes[i]->setActive(FALSE);
	phantom->stopEffect();
	if (errorCode != FD_OK) {
		errorCode = FD_OK;
		resetPHANToM();
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

int vrpn_Phantom::handle_effects_change_message(void *userdata,
												vrpn_HANDLERPARAM p){

    vrpn_Phantom *me = (vrpn_Phantom *) userdata;
 
    DynamicPlane *plane = (me->planes)[0];

	float k_adhesion_norm, k_adhesion_lat, tex_amp, tex_wl,
		buzz_amp, buzz_freq;

	if (decode_surface_effects(p.buffer, p.payload_len, &k_adhesion_norm, 
		&k_adhesion_lat, &tex_amp, &tex_wl, &buzz_amp, &buzz_freq)){
		fprintf(stderr, "Error: handle_effects_change_message(): can't decode\n");
	}
	
	plane->setTextureAmplitude(tex_amp*1000.0);
	plane->setTextureWavelength(tex_wl*1000.0);
	plane->setBuzzAmplitude(buzz_amp*1000.0);
	plane->setBuzzFrequency(buzz_freq);
	plane->enableTexture(TRUE);
	plane->enableBuzzing(TRUE);

	return 0;
}

int vrpn_Phantom::handle_setVertex_message(void *userdata, 
					   vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  long	temp;
  int vertNum;
  float x,y,z;

  decode_vertex(p.buffer, p.payload_len, &temp, &x, &y, &z);
  vertNum=temp;
    
  if(me->trimesh->setVertex(vertNum,x*1000.0,y*1000.0,z*1000.0))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setVertex\n");
    return -1;
  }
}

int vrpn_Phantom::handle_setNormal_message(void *userdata, 
					   vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  long	temp;
  int normNum;
  float x, y, z;

  decode_normal(p.buffer, p.payload_len, &temp, &x, &y, &z);
  normNum = temp;
    
  if(me->trimesh->setNormal(normNum,x,y,z))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setNormal\n");
    return -1;
  }
}

int vrpn_Phantom::handle_setTriangle_message(void *userdata, 
					     vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  int triNum, v0, v1, v2, n0, n1, n2;
  long ltriNum, lv0, lv1, lv2, ln0, ln1, ln2;
  decode_triangle(p.buffer, p.payload_len, 
	  &ltriNum, &lv0, &lv1, &lv2, &ln0, &ln1, &ln2);
  triNum = ltriNum; v0 = lv0; v1 = lv1; v2 = lv2;
  n0 = ln0; n1 = ln1; n2 = ln2;

  if(me->trimesh->setTriangle(triNum,v0,v1,v2,n0,n1,n2))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setTriangle\n");
    return -1;
  }
}


int vrpn_Phantom::handle_removeTriangle_message(void *userdata, 
						vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  long	temp;
 
  decode_removeTriangle(p.buffer, p.payload_len, &temp);

  int triNum=temp;

  if(me->trimesh->removeTriangle(triNum))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::removeTriangle\n");
    return -1;
  }
}


int vrpn_Phantom::handle_updateTrimeshChanges_message(void *userdata, 
						      vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  float SurfaceKspring,SurfaceKdamping,SurfaceFdynamic,SurfaceFstatic;

  decode_updateTrimeshChanges(p.buffer, p.payload_len,
	  &SurfaceKspring, &SurfaceKdamping, &SurfaceFdynamic,
	  &SurfaceFstatic);


  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->updateChanges();

  myTrimesh->setSurfaceKspring(SurfaceKspring);
  myTrimesh->setSurfaceFstatic(SurfaceFstatic);
  myTrimesh->setSurfaceFdynamic(SurfaceFdynamic); 
  myTrimesh->setSurfaceKdamping(SurfaceKdamping);

  return 0;
}

int vrpn_Phantom::handle_setTrimeshType_message(void *userdata, 
					       vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  
  long	temp;

  decode_setTrimeshType(p.buffer, p.payload_len, &temp);

  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->setType((TrimeshType)temp);

  return 0;
}

int vrpn_Phantom::handle_clearTrimesh_message(void *userdata, 
					      vrpn_HANDLERPARAM){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  me->trimesh->clear();
  return 0;
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

  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->setTransformMatrix(xformMatrix);

  return 0;
}

int vrpn_Phantom::handle_constraint_change_message(void *userdata,
						vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  double pnt[3]; 
  long enable;
  float x,y,z, kSpr;

  decode_constraint(p.buffer, p.payload_len,
	  &enable, &x, &y, &z, &kSpr);
  
  // convert from meters to mm and dyne/meter to dyne/mm
  pnt[0] = 1000*x; pnt[1] = 1000*y; pnt[2] = 1000*z;
  if (!enable){
	  me->phantom->stopEffect();
	  me->pointConstraint->stop();
  }
  else{
        me->pointConstraint->setPoint(pnt, kSpr/1000.0);
	me->phantom->startEffect();
	me->pointConstraint->start();
  }
  return 0;
}

int vrpn_Phantom::handle_forcefield_change_message(void *userdata,
						vrpn_HANDLERPARAM p){

  vrpn_Phantom *me = (vrpn_Phantom *)userdata;

  int i,j;

  decode_forcefield(p.buffer, p.payload_len,me->ff_origin,
	  me->ff_force, me->ff_jacobian, &(me->ff_radius));

  for (i=0;i<3;i++){
    me->ff_origin[i] *= (1000); // convert from m->mm
    for (j=0;j<3;j++){
      me->ff_jacobian[i][j] *= (0.001f); // convert from 
					// dyne/m to dyne/mm
    }
  }

  me->ff_radius *= (1000); // convert m to mm
  me->forceField->setForce(me->ff_origin, me->ff_force, 
	me->ff_jacobian, me->ff_radius);
  if (!me->forceField->isActive()){
	me->phantom->setEffect(me->forceField);
	me->phantom->startEffect();
	me->forceField->start();
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


void phantomErrorHandler( int errnum, char *description, void *userdata)
{
	vrpn_Phantom *me = (vrpn_Phantom *) userdata;
	fprintf(stderr, "PHANTOM ERROR: %s\n", description);
	int i;
	for (i = 0; i < MAXPLANE; i++){
		me->planes[i]->cleanUpAfterError();
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
			me->sendError(FD_FORCE_ERROR);
			me->errorCode = FD_FORCE_ERROR;
			break;
		default:
			me->sendError(FD_MISC_ERROR);
			me->errorCode = FD_MISC_ERROR;
			break;
	}
}


