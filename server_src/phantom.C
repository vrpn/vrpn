#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "vrpn_Connection.h"
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_ForceDevice.h"

#include "ghost.h"
#include "plane.h"
#include "trimesh.h"
#include "constraint.h"
#include "quat.h"

#ifndef	_WIN32
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

const int 	MAX_TRACKERS = 100;
const int	MAX_BUTTONS = 100;

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

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

void phantomErrorHandler (int errorNumber, char *description, void *userdata);

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
				       
	ConstraintEffect *pointConstraint; 	// this is a force appended to
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
	static int handle_transformTrimesh_message(void *userdata, 
					 vrpn_HANDLERPARAM p);
	static int handle_constraint_change_message(void *userdata,
					vrpn_HANDLERPARAM p);

	// from vrpn_Tracker
	static int handle_resetOrigin_change_message(void *, vrpn_HANDLERPARAM);
	static int handle_update_rate_request (void *, vrpn_HANDLERPARAM);

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
	void handlePHANToM_Error(int errorNumber, char *description);
};


void vrpn_Phantom::handle_plane(void *userdata,const vrpn_Plane_PHANTOMCB &p)
{
 // printf("MY Plane is %lfX + %lfY + %lfZ + %lf = 0 \n",p.plane[0],
//	p.plane[1],p.plane[2],p.plane[3]);
  //printf("surface is %lf,%lf,%lf,%lf\n",p.SurfaceKspring,p.SurfaceKdamping,
//	  p.SurfaceFdynamic, p.SurfaceFstatic);
//     printf("which plane = %d\n", p.which_plane);

    vrpn_Phantom *me = (vrpn_Phantom *) userdata;
 
    Plane **plane_node = me->planes;
    gstPHANToM *phan = me->phantom;
    int which_plane = me->which_plane;
	
    which_plane = p.which_plane;
 
    // If the plane's normal is (0,0,0), it is a stop surface command
	// turn off the surface.
    if(p.plane[0]== 0 && p.plane[1] ==0  && p.plane[2] == 0 ) {
	plane_node[which_plane]->setInEffect(FALSE);
    }
    else {
	vrpn_Plane_PHANTOMCB p2 = p;
	check_parameters(&p2);
	plane_node[which_plane]->update(p2.plane[0],p2.plane[1],
					p2.plane[2],p2.plane[3]*1000.0);
	plane_node[which_plane]->setSurfaceKspring(p2.SurfaceKspring);
	plane_node[which_plane]->setSurfaceFstatic(p2.SurfaceFstatic);
	plane_node[which_plane]->setSurfaceFdynamic(p2.SurfaceFdynamic); 
	plane_node[which_plane]->setSurfaceKdamping(p2.SurfaceKdamping);
	plane_node[which_plane]->setNumRecCycles((int)p2.numRecCycles);
	plane_node[which_plane]->setInEffect(TRUE); 

    }
}

// This function reports errors if surface friction, compliance parameters
// are not in valid ranges for the GHOST library and sets them as 
// close as it can to the requested values.

void vrpn_Phantom::check_parameters(vrpn_Plane_PHANTOMCB *p)
{
        if (p->SurfaceKspring <= 0){
                printf("Error: Kspring = %f <= 0\n", p->SurfaceKspring);
                p->SurfaceKspring = 0.001f;
        }
        else if (p->SurfaceKspring > 1.0){
                printf("Error: Kspring = %f > 1.0\n", p->SurfaceKspring);
                p->SurfaceKspring = 1.0;
        }
        if (p->SurfaceFstatic < 0){
                printf("Error: Fstatic = %f < 0\n", p->SurfaceFstatic);
                p->SurfaceFstatic = 0.0;
        }
        else if (p->SurfaceFstatic > 1.0){
                printf("Error: Fstatic = %f > 1.0\n", p->SurfaceFstatic);
                p->SurfaceFstatic = 1.0;
        }
        if (p->SurfaceFdynamic > p->SurfaceFstatic){
                printf("Error: Fdynamic > Fstatic\n");
                p->SurfaceFdynamic = 0.0;
        }
        if (p->SurfaceKdamping < 0){
                printf("Error: Kdamping = %f < 0\n", p->SurfaceKdamping);
                p->SurfaceKdamping = 0;
        }
        else if (p->SurfaceKdamping > 0.005){
                printf("Error: Kdamping = %f > 0.005\n", p->SurfaceKdamping);
                p->SurfaceKdamping = 0.005f;
        }

}

// This function reinitializes the PHANToM (resets the origin)
void vrpn_Phantom::resetPHANToM(void)
{
	scene->stopServoLoop();
	rootH->removeChild(phantom);
	delete(phantom);
	phantom = new gstPHANToM("Default PHANToM");
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

  /* register ghost error handler */
  setErrorCallback( phantomErrorHandler, (void *)this);
  /* disable popup error dialogs */
  printErrorMessages(FALSE);

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

  SurfaceKspring= 0.29f;
  SurfaceFdynamic = 0.02f;
  SurfaceFstatic = 0.03f;
  SurfaceKdamping = 0.0;

  for(int i=0; i<MAXPLANE; i++ ) {
       planes[i] = new Plane(0,0,1,0);	   
	   planes[i]->setDefaultSurfaceKspring(SurfaceKspring);
       planes[i]->setDefaultSurfaceFdynamic(SurfaceFdynamic);
       planes[i]->setDefaultSurfaceFstatic(SurfaceFstatic);
	   planes[i]->setDefaultSurfaceKdamping(SurfaceKdamping);
       hapticScene->addChild(planes[i]);
  }
  which_plane = 0;
 
  cur_plane = planes[which_plane];

  trimesh = new Trimesh(0,NULL,0,NULL);

  trimesh->addToScene(hapticScene);

  //  status= TRACKER_RESETTING;
  gettimeofday(&(vrpn_ForceDevice::timestamp),NULL);

  if (vrpn_ForceDevice::connection->register_handler(plane_message_id, 
	handle_plane_change_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(startTrimesh_message_id, 
	handle_startTrimesh_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setVertex_message_id, 
	handle_setVertex_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(setTriangle_message_id, 
	handle_setTriangle_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(finishTrimesh_message_id, 
	handle_finishTrimesh_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(transformTrimesh_message_id, 
	handle_transformTrimesh_message, this, vrpn_ForceDevice::my_id)) {
		fprintf(stderr,"vrpn_Phantom:can't register handler\n");
		vrpn_ForceDevice::connection = NULL;
  }
  if (vrpn_ForceDevice::connection->register_handler(set_constraint_message_id,
	handle_constraint_change_message, this, vrpn_ForceDevice::my_id)) {
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
	char    forcebuf[1000];
	int		len;

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
            len = vrpn_ForceDevice::encode_to(forcebuf);
            if(vrpn_ForceDevice::connection->pack_message(len,timestamp,
                force_message_id,vrpn_ForceDevice::my_id,
                forcebuf,vrpn_CONNECTION_LOW_LATENCY)) {
                    fprintf(stderr,"Phantom: cannot write message: tossing\n");
            }
        }
	// Encode the SCP if there is a connection
	if (vrpn_ForceDevice::connection) {
	    len = vrpn_ForceDevice::encode_scp_to(forcebuf);
	    if (vrpn_ForceDevice::connection->pack_message(len, timestamp,
		scp_message_id, vrpn_ForceDevice::my_id,
		forcebuf, vrpn_CONNECTION_LOW_LATENCY)) {
		    fprintf(stderr,"Phantom: cannot write message: tossing\n");
	    }
	}
	
  //      print_report();
    }
}


void phantomErrorHandler (int errorNumber, char *description, void *userdata)
{

	vrpn_Phantom *me = (vrpn_Phantom *)userdata;

	me->handlePHANToM_Error(errorNumber, description);
}

void vrpn_Phantom::handlePHANToM_Error(int errorNumber, char *description)
{
	char errorBuf[100];
    int len;
    fprintf(stderr, "PHANToM Error: %s\n", description);
    // an error condition has occurred; send it to the client
    switch(errorNumber){
	case GST_OUT_OF_RANGE_ERROR:
	  errorCode = FD_VALUE_OUT_OF_RANGE;
	  break;
	case GST_DUTY_CYCLE_ERROR:
	  errorCode = FD_DUTY_CYCLE_ERROR;
	  break;
	case GST_PHANTOM_FORCE_ERROR:
	  errorCode = FD_FORCE_ERROR;
	  break;
	default:
	  errorCode = FD_MISC_ERROR;
	  break;
    }
    if (vrpn_ForceDevice::connection) {
	len = vrpn_ForceDevice::encode_error_to(errorBuf);
	if (vrpn_ForceDevice::connection->pack_message(len, timestamp,
		error_message_id, vrpn_ForceDevice::my_id,
		errorBuf, vrpn_CONNECTION_RELIABLE)) {
		    fprintf(stderr,"Phantom: cannot write message: tossing\n");	
	}
    }
}


void vrpn_Phantom::reset(){
	if(trimesh->displayStatus())
		trimesh->clear();
	for (int i = 0; i < MAXPLANE; i++)
	   if (planes[i]) planes[i]->setInEffect(FALSE);
	phantom->stopEffect();
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
	long *params = (long*)(p.buffer);
	vrpn_Plane_PHANTOMCB	tp;
	vrpn_PHANTOMCHANGELIST *handler = me->plane_change_list;
	long	temp;
	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len !=  (NUM_MESSAGE_PARAMETERS*sizeof(float)) ) {
		fprintf(stderr,"vrpn_Phantom: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, NUM_MESSAGE_PARAMETERS*sizeof(float) );
		return -1;
	}
	tp.msg_time = p.msg_time;

	// Typecasting used to get the byte order correct on the floats
	// that are coming from the other side.
	for (i = 0; i < 4; i++) {
	 	temp = ntohl(params[i]);
		tp.plane[i] = *(float*)(&temp);
	}
	temp = ntohl(params[4]);
	tp.SurfaceKspring = *(float*)(&temp);
	temp = ntohl(params[5]);
	tp.SurfaceKdamping = *(float*)(&temp);
	temp = ntohl(params[6]);
	tp.SurfaceFdynamic = *(float*)(&temp);
	temp = ntohl(params[7]);
	tp.SurfaceFstatic = *(float*)(&temp);
    
	temp = ntohl(params[8]);
	tp.which_plane = *(float*)(&temp);

	// for recovery:
	temp = ntohl(params[9]);
	tp.numRecCycles = temp;

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

	return 0;
}

int vrpn_Phantom::handle_startTrimesh_message(void *userdata, 
					      vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  long *params = (long*)(p.buffer);
  
  long	temp;
  
  int numExpectedParameters=2;
  
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change message payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
	    p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }
  
  temp = ntohl(params[0]);
  int numVerts=temp;
  temp = ntohl(params[1]);
  int numTris=temp;
  
  me->trimesh->startDefining(numVerts,numTris);
  return 0;
}

int vrpn_Phantom::handle_setVertex_message(void *userdata, 
					   vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  long *params = (long*)(p.buffer);
  
  long	temp;
  
  int numExpectedParameters=4;
  
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change message payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
	    p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }

  temp = ntohl(params[0]);
  int vertNum=temp;
  temp = ntohl(params[1]);
  float x = *(float*)(&temp);
  temp = ntohl(params[2]);
  float y = *(float*)(&temp);
  temp = ntohl(params[3]);
  float z = *(float*)(&temp);
    
  if(me->trimesh->setVertex(vertNum,x*1000.0,y*1000.0,z*1000.0))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setVertex\n");
    return -1;
  }
}

int vrpn_Phantom::handle_setTriangle_message(void *userdata, 
					     vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  long *params = (long*)(p.buffer);
  
  long	temp;
 
  int numExpectedParameters=4;
  
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change message payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
	    p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }

  temp = ntohl(params[0]);
  int triNum=temp;
  temp = ntohl(params[1]);
  int v0=temp;
  temp = ntohl(params[2]);
  int v1=temp;
  temp = ntohl(params[3]);
  int v2=temp;

  if(me->trimesh->setTriangle(triNum,v0,v1,v2))
    return 0;
  else{
      fprintf(stderr,"vrpn_Phantom: error in trimesh::setTriangle\n");
    return -1;
  }
}


int vrpn_Phantom::handle_finishTrimesh_message(void *userdata, 
					       vrpn_HANDLERPARAM p){
  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  long *params = (long*)(p.buffer);
  
  long	temp;
  
  int numExpectedParameters=4;
  
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change message payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
	    p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }

  // Typecasting used to get the byte order correct on the floats
  // that are coming from the other side.
  temp = ntohl(params[0]);
  float SurfaceKspring = *(float*)(&temp);
  temp = ntohl(params[1]);
  float SurfaceKdamping = *(float*)(&temp);
  temp = ntohl(params[2]);
  float SurfaceFdynamic = *(float*)(&temp);
  temp = ntohl(params[3]);
  float SurfaceFstatic = *(float*)(&temp);

  Trimesh *myTrimesh=me->trimesh;
  
  myTrimesh->finishDefining();

  myTrimesh->setSurfaceKspring(SurfaceKspring);
  myTrimesh->setSurfaceFstatic(SurfaceFstatic);
  myTrimesh->setSurfaceFdynamic(SurfaceFdynamic); 
  myTrimesh->setSurfaceKdamping(SurfaceKdamping);

  return 0;
}

int vrpn_Phantom::handle_transformTrimesh_message(void *userdata, 
					       vrpn_HANDLERPARAM p){

  vrpn_Phantom *me = (vrpn_Phantom *)userdata;
  long *params = (long*)(p.buffer);
  
  long	temp;

  int numExpectedParameters=16;
  
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change message payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
	    p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }
  
  float xformMatrix[16];

  for(int i=0;i<16;i++){
    // Typecasting used to get the byte order correct on the floats
    // that are coming from the other side.
    temp = ntohl(params[i]);
    xformMatrix[i]= *(float*)(&temp);
  }

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
  long *params = (long*)(p.buffer);
  double pnt[3]; 

  long temp;
  int numExpectedParameters = 5;
  if (p.payload_len !=  (numExpectedParameters*sizeof(float)) ) {
    fprintf(stderr,"vrpn_Phantom: change constraint payload error\n");
    fprintf(stderr,"             (got %d, expected %d)\n",
            p.payload_len, numExpectedParameters*sizeof(float) );
    return -1;
  }

  temp = ntohl(params[0]);
  int enable=temp;
  temp = ntohl(params[1]);
  float x = *(float*)(&temp);
  temp = ntohl(params[2]);
  float y = *(float*)(&temp);
  temp = ntohl(params[3]);
  float z = *(float*)(&temp);
  temp = ntohl(params[4]);
  float kSpr = *(float*)(&temp);
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

// static
int vrpn_Phantom::handle_resetOrigin_change_message(void * userdata,
                                                vrpn_HANDLERPARAM p) {
  vrpn_Phantom * me = (vrpn_Phantom *) userdata;
  me->resetPHANToM();
  return 0;

}


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


void	Usage(char *s)
{
  fprintf(stderr,"Usage: %s [-f filename] [-warn] [-v]\n",s);
  fprintf(stderr,"       -f: Full path to config file (default phantom.cfg)\n");
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail)\n");
  fprintf(stderr,"       -v: Verbose\n");
  exit(-1);
}

void main (unsigned argc, char *argv[])
{
	char	*config_file_name = "phantom.cfg";
	FILE	*config_file;
	int	bail_on_error = 1;
	int	verbose = 0;
	int	realparams = 0;
	unsigned int	i;

	// Parse the command line
	i = 1;
	while (i < argc) {
	  if (!strcmp(argv[i], "-f")) {		// Specify config-file name
		if (++i > argc) { Usage(argv[0]); }
		config_file_name = argv[i];
	  } else if (!strcmp(argv[i], "-warn")) {// Don't bail on errors
		bail_on_error = 0;
	  } else if (!strcmp(argv[i], "-v")) {	// Verbose
		verbose = 1;
	  } else if (argv[i][0] == '-') {	// Unknown flag
		Usage(argv[0]);
	  } else switch (realparams) {		// Non-flag parameters
	    case 0:
	    default:
		Usage(argv[0]);
	  }
	  i++;
	}
	
#ifdef _WIN32
	WSADATA wsaData; 
	int status;
    if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
         fprintf(stderr, "WSAStartup failed with %d\n", status);
    } else if(verbose) {
		 fprintf(stderr, "WSAStartup success\n");
	}
#endif

	vrpn_Synchronized_Connection	connection;
	vrpn_Phantom	*phantoms[MAX_TRACKERS];
	int		num_phantoms = 0;
	// Open the configuration file
	if (verbose) printf("Reading from config file %s\n", config_file_name);
	if ( (config_file = fopen(config_file_name, "r")) == NULL) {
		perror("Cannot open config file");
		fprintf(stderr,"  (filename %s)\n", config_file_name);
		return; //	return -1;
	}

	// Read the configuration file, creating a device for each entry.
	// Each entry is on one line, which starts with the name of the
	//   class of the object that is to be created.
	// If we fail to open a certain device, print a message and decide
	//  whether we should bail.
      {	char	line[512];	// Line read from the input file
	char	s1[512],s2[512];	// String parameters
	int	i1;				// Integer parameters
	float	f1;				// Float parameters

	// Read lines from the file until we run out
	while ( fgets(line, sizeof(line), config_file) != NULL ) {

	  // Make sure the line wasn't too long
	  if (strlen(line) >= sizeof(line)-1) {
		fprintf(stderr,"Line too long in config file: %s\n",line);
		if (bail_on_error) { return; /* return -1; */}
		else { continue; }	// Skip this line
	  }

	  // Figure out the device from the name and handle appropriately
	  #define isit(s) !strncmp(line,s,strlen(s))
	  
	  if (isit("vrpn_Phantom")) {
		// Get the arguments (class, button_name, portno)
		if (sscanf(line,"%511s%511s%d%f",s1,s2,&i1,&f1) != 4) {
		  fprintf(stderr,"Bad vrpn_Phantom line: %s\n",line);
		  if (bail_on_error) { return; /*return -1;*/ }
		  else { continue; }	// Skip this line
		}

		// Make sure there's room for a new phantom
		if (num_phantoms >= MAX_TRACKERS) {
		  fprintf(stderr,"Too many trackers in config file");
		  if (bail_on_error) { return; /*return -1; */}
		  else { continue; }	// Skip this line
		}
		//open the phantom
		if(verbose) printf(
			"Opening vrpn_phantom: %s on port %d\n", s2,i1);
		if ( (phantoms[num_phantoms] =
		     new vrpn_Phantom(s2, &connection, f1)) == NULL){
		  fprintf(stderr,"Can't create new vrpn_phantom\n");
		  if (bail_on_error) { return; /*return -1;*/ }
		  else { continue; }	// Skip this line
		} else {
		  num_phantoms++;
		}
	  }
	  else {	// Never heard of it
		sscanf(line,"%511s",s1);	// Find out the class name
		fprintf(stderr,"Unknown class: %s\n",s1);
		if (bail_on_error) { return; }
		else { continue; }	// Skip this line
	  }
	}
      }

	// Close the configuration file
	fclose(config_file);

	// Loop forever calling the mainloop()s for all devices
	while (1) {
		int	i;

		// Let all the trackers generate reports
		for (i = 0; i < num_phantoms; i++)
			phantoms[i]->mainloop();

		// Send and receive all messages
		connection.mainloop();

		// if we're not connected make sure our phantoms are reset
		if(!connection.connected()){
			for (i = 0; i < num_phantoms; i++) {
				phantoms[i]->reset();
			}
		}
	}

}
