#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


#ifdef linux
#include <linux/lp.h>
#endif

#ifndef _WIN32
#include <strings.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif 

#include "vrpn_ForceDevice.h"


static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}
vrpn_ForceDevice::vrpn_ForceDevice(char *name, vrpn_Connection *c)
{
	//set connection to the one passed in
  char * servicename;
  servicename = vrpn_copy_service_name(name);
	connection = c;

	//register this force device and the needed message types
	if(connection) {
		my_id = connection->register_sender(servicename);
		force_message_id = connection->register_message_type("Force");
		plane_message_id = connection->register_message_type("Plane");
		startTrimesh_message_id = 
		  connection->register_message_type("startTrimesh");
		setVertex_message_id = 
		  connection->register_message_type("setVertex");
		setTriangle_message_id = 
		  connection->register_message_type("setTriangle");
		finishTrimesh_message_id = 
		  connection->register_message_type("finishTrimesh");
		transformTrimesh_message_id = 
		  connection->register_message_type("transformTrimesh");
		scp_message_id = connection->register_message_type("SCP");
		set_constraint_message_id = 
			connection->register_message_type("CONSTRAINT");
	}

	//set the current time to zero
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	//set the force to zero
	force[0] = force[1] = force[2] = 0.0;

  if (servicename)
    delete [] servicename;
}

void vrpn_ForceDevice::print_report(void)
{
  printf("Timestamp:%ld:%ld\n", timestamp.tv_sec, timestamp.tv_usec);
  printf("Force    :%lf, %lf, %lf\n", force[0],force[1],force[2]);
}

int vrpn_ForceDevice::encode_to(char *buf)
{
   // Message includes: double force[3]
   // Byte order of each needs to be reversed to match network standard

   int i;
   double *dBuf = (double *)buf;
   int	index = 0;

   // Move the force there
   for (i = 0; i < 3; i++){
	dBuf[index++] = *(double *)(&force[i]);
   }
   for (i = 0; i < index; i++) {
   	dBuf[i] = htond(dBuf[i]);
   }
   return index*sizeof(double);
}

int vrpn_ForceDevice::encode_scp_to(char *buf)
{
    int i;
    double *dBuf = (double *)buf;
    int index = 0;
    for (i = 0; i < 3; i++) {
        dBuf[index++] = *(double *)(&scp_pos[i]);
    }
    for (i = 0; i < 4; i++) {
        dBuf[index++] = *(double *)(&scp_quat[i]);
    }

    // convert the doubles
    for (i = 0; i < index; i++) {
        dBuf[i] = htond(dBuf[i]);
    }
    return index*sizeof(double);
}


#ifdef _WIN32
#ifndef	VRPN_CLIENT_ONLY

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
                p->SurfaceKspring = 0.001;
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
                p->SurfaceKdamping = 0.005;
        }

}

// This function reinitializes the PHANToM (resets the origin)
void vrpn_Phantom::resetPHANToM(void)
{
	scene->stopServoLoop();
	rootH->removeChild(phantom);
	delete(phantom);
	phantom = new gstPHANToM("phantom.ini");
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

  /* Create the root separator. */
  rootH = new gstSeparator;
  scene->setRoot(rootH);
  
  /* Create the phantom object.  When this line is processed, 
     the phantom position is zeroed. */
  phantom = new gstPHANToM("phantom.ini");
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

  SurfaceKspring= 0.29;
  SurfaceFdynamic = 0.02;
  SurfaceFstatic = 0.03;
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

  this->register_plane_change_handler(this, handle_plane);


  if (vrpn_Tracker::register_server_handlers())
    fprintf(stderr, "vrpn_Phantom: couldn't register xform request handlers\n");


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
	q_type p_quat, v_quat, a_quat; // angular information (pos, vel, acc)
	double angVelNorm, angAccNorm;
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

#endif  // VRPN_CLIENT_ONLY
#endif  // _WIN32

vrpn_ForceDevice_Remote::vrpn_ForceDevice_Remote(char *name):
	vrpn_ForceDevice(name,vrpn_get_connection_by_name(name)),
	change_list(NULL), scp_change_list(NULL) 
{
    which_plane = 0;

    // Make sure that we have a valid connection
    if (connection == NULL) {
		fprintf(stderr,"vrpn_ForceDevice_Remote: No connection\n");
		return;
     }

     // Register a handler for the change callback from this device.
     if (connection->register_handler(force_message_id, handle_change_message,
	this, my_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    connection = NULL;
     }

     // Register a handler for the scp change callback from this device.
     if (connection->register_handler(scp_message_id, handle_scp_change_message,
	this, my_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    connection = NULL;
     }

     // Find out what time it is and put this into the timestamp
     gettimeofday(&timestamp, NULL);
}


char *vrpn_ForceDevice_Remote::encode_plane(int &len){
   // Message includes: float force[3]
   // Byte order of each needs to be reversed to match network standard
   // This moving is done by horrible typecast hacks.  Please forgive.

  len=10*sizeof(unsigned long); // currently a plane has 10 parameters
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

   // Move the force there
   for (i = 0; i < 4; i++){
   	longbuf[index++] = *(unsigned long*)(void*)(&plane[i]);
   }

   longbuf[index++] = *(unsigned long*)(void*)(&SurfaceKspring);
   longbuf[index++] = *(unsigned long*)(void*)(&SurfaceKdamping);
   longbuf[index++] = *(unsigned long*)(void*)(&SurfaceFdynamic);
   longbuf[index++] = *(unsigned long*)(void*)(&SurfaceFstatic);
   longbuf[index++] = *(unsigned long*)(void*)(&which_plane);

   longbuf[index++] = *(unsigned long*)(void*)(&numRecCycles);

   for (i = 0; i < index; i++) {
   	longbuf[i] = htonl(longbuf[i]);
   }
   if(index*sizeof(unsigned long)!=len){
     fprintf(stderr,
	     "ERROR: incorrect # of parameters in vrpn_ForceDevice_Remote::encode_plane\n");
   }
   return buf;
}

char *vrpn_ForceDevice_Remote::encode_startTrimesh(int &len,
						int numVerts,int numTris){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (2)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&numVerts);
  longbuf[index++] = *(unsigned long*)(void*)(&numTris);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }

  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_vertex(int &len,int vertNum,
					     float x,float y,float z){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (4)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&vertNum);
  longbuf[index++] = *(unsigned long*)(void*)(&x);
  longbuf[index++] = *(unsigned long*)(void*)(&y);
  longbuf[index++] = *(unsigned long*)(void*)(&z);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_triangle(int &len,int triNum,
					       int vert0,int vert1,int vert2){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (4)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&triNum);
  longbuf[index++] = *(unsigned long*)(void*)(&vert0);
  longbuf[index++] = *(unsigned long*)(void*)(&vert1);
  longbuf[index++] = *(unsigned long*)(void*)(&vert2);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

// this is where we send down our surface parameters
char *vrpn_ForceDevice_Remote::encode_finishTrimesh(int &len){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (4)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&SurfaceKspring);
  longbuf[index++] = *(unsigned long*)(void*)(&SurfaceKdamping);
  longbuf[index++] = *(unsigned long*)(void*)(&SurfaceFdynamic);
  longbuf[index++] = *(unsigned long*)(void*)(&SurfaceFstatic);
  
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_trimeshTransform(int &len,
						       float homMatrix[16]){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (16)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index;
  
  for(index=0;index<16;index++){
    longbuf[index] = *(unsigned long*)(void*)(&(homMatrix[index]));    
  }
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_constraint(int &len, int enable,
			float x, float y, float z,float kSpr){
  len = 5*sizeof(unsigned long);
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int   index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&enable);
  longbuf[index++] = *(unsigned long*)(void*)(&x);
  longbuf[index++] = *(unsigned long*)(void*)(&y);
  longbuf[index++] = *(unsigned long*)(void*)(&z);
  longbuf[index++] = *(unsigned long*)(void*)(&kSpr);
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf;
}

void vrpn_ForceDevice_Remote::set_plane(float *p)
{ 
	for(int i=0;i<4;i++ ) {
		plane[i]= p[i];
	}
}

void vrpn_ForceDevice_Remote::set_plane(float a, float b, float c,
										float d)
{ plane[0] = a; 
  plane[1] = b;
  plane[2] = c;
  plane[3] = d;
}

void vrpn_ForceDevice_Remote::set_plane(float *normal, float d)
{ 
	for(int i =0;i<3;i++ ) {
	  plane[i] = normal[i];
	}

     plane[3] = d;
}

void vrpn_ForceDevice_Remote::sendSurface(void)
{ // Encode the plane if there is a connection
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;
  
  if(connection) {
    msgbuf = encode_plane(len);
    if(connection->pack_message(len,timestamp,plane_message_id,
				my_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }

}

void vrpn_ForceDevice_Remote::startSurface(void)
{  //Encode the plane if there is a connection
    char	*msgbuf;
    int		len;
    struct timeval current_time;
    
    gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;
    
    if(connection){
      msgbuf = encode_plane(len);
      if (connection->pack_message(len,timestamp,plane_message_id,
				   my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      connection->mainloop();
      delete msgbuf;
    }
}

void vrpn_ForceDevice_Remote::stopSurface(void)
{  //Encode the plane if there is a connection
    char	*msgbuf;
    int		len;
    struct timeval current_time;
    
    gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;
    
    set_plane(0,0,0,0);
  
    if(connection) {
      msgbuf = encode_plane(len);
      if (connection->pack_message(len,timestamp,plane_message_id,
				   my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      connection->mainloop();
      delete msgbuf;
    }
}



void vrpn_ForceDevice_Remote::startSendingTrimesh(int numVerts,int numTris){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_startTrimesh(len,numVerts,numTris);
    if (connection->pack_message(len,timestamp,startTrimesh_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

void vrpn_ForceDevice_Remote::sendVertex(int vertNum,float x,float y,float z){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_vertex(len,vertNum,x,y,z);
    if (connection->pack_message(len,timestamp,setVertex_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

void vrpn_ForceDevice_Remote::sendTriangle(int triNum,
					   int vert0,int vert1,int vert2){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_triangle(len,triNum,vert0,vert1,vert2);
    if (connection->pack_message(len,timestamp,setTriangle_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

void vrpn_ForceDevice_Remote::finishSendingTrimesh(){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_finishTrimesh(len);
    if (connection->pack_message(len,timestamp,finishTrimesh_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

// set the trimesh's homogen transform matrix (in row major order)
void vrpn_ForceDevice_Remote::sendTrimeshTransform(float homMatrix[16]){
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_trimeshTransform(len,homMatrix);
    if (connection->pack_message(len,timestamp,transformTrimesh_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

// stop the triMesh if connection
void vrpn_ForceDevice_Remote::stopTrimesh(void){
  
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_startTrimesh(len,0,0);
    if (connection->pack_message(len,timestamp,startTrimesh_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

void vrpn_ForceDevice_Remote::sendConstraint(int enable, 
					float x, float y, float z, float kSpr)
{
  char *msgbuf;
  int	len;
  struct timeval current_time;

  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_constraint(len, enable, x, y, z, kSpr);
    if (connection->pack_message(len,timestamp,set_constraint_message_id,
				my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

void	vrpn_ForceDevice_Remote::mainloop(void)
{/*  	float p[4];
    for(int i=0;i<4;i++)
		p[i]=i;
	set_plane(p);
	send_plane();
	printf("remote sending plane\n");
   */	if (connection) { connection->mainloop(); }
}

int vrpn_ForceDevice_Remote::register_change_handler(void *userdata,
			vrpn_FORCECHANGEHANDLER handler)
{
	vrpn_FORCECHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
		    "vrpn_ForceDevice_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_FORCECHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_ForceDevice_Remote::register_handler: Out of memory\n");
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

int vrpn_ForceDevice_Remote::unregister_change_handler(void *userdata,
			vrpn_FORCECHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_FORCECHANGELIST	*victim, **snitch;

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
		fprintf(stderr,
		  "vrpn_ForceDevice_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_ForceDevice_Remote::register_scp_change_handler(void *userdata,
                        vrpn_FORCESCPHANDLER handler)
{
        vrpn_FORCESCPCHANGELIST    *new_entry;

        // Ensure that the handler is non-NULL
        if (handler == NULL) {
            fprintf(stderr,
              "vrpn_ForceDevice_Remote::register_scp_handler: NULL handler\n");
            return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_FORCESCPCHANGELIST) == NULL) {
            fprintf(stderr,
             "vrpn_ForceDevice_Remote::register_scp_handler: Out of memory\n");
            return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = scp_change_list;
        scp_change_list = new_entry;

        return 0;
}

int vrpn_ForceDevice_Remote::unregister_scp_change_handler(void *userdata,
                        vrpn_FORCESCPHANDLER handler)
{
        // The pointer at *snitch points to victim
        vrpn_FORCESCPCHANGELIST    *victim, **snitch;

        // Find a handler with this registry in the list (any one will do,
        // since all duplicates are the same).
        snitch = &scp_change_list;
        victim = *snitch;
        while ( (victim != NULL) &&
                ( (victim->handler != handler) ||
                  (victim->userdata != userdata) )) {
            snitch = &( (*snitch)->next );
            victim = victim->next;
        }

        // Make sure we found one
        if (victim == NULL) {
         fprintf(stderr,
          "vrpn_ForceDevice_Remote::unregister_scphandler: No such handler\n");
          return -1;
        }

        // Remove the entry from the list
        *snitch = victim->next;
        delete victim;

        return 0;
}


int vrpn_ForceDevice_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_ForceDevice_Remote *me = (vrpn_ForceDevice_Remote *)userdata;
	double *params = (double*)(p.buffer);
	vrpn_FORCECB	tp;
	vrpn_FORCECHANGELIST *handler = me->change_list;

	int	i;

	// Fill in the parameters to the tracker from the message
	if (p.payload_len !=  (3*sizeof(double)) ) {
	  fprintf(stderr,"vrpn_ForceDevice: change message payload error\n");
	  fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 3*sizeof(double) );
	  return -1;
	}
	tp.msg_time = p.msg_time;

	// Typecasting used to get the byte order correct on the floats
	// that are coming from the other side.
	for (i = 0; i < 3; i++) {
		tp.force[i] = ntohd(params[i]);
	}

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}

	return 0;
}

int vrpn_ForceDevice_Remote::handle_scp_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
        vrpn_ForceDevice_Remote *me = (vrpn_ForceDevice_Remote *)userdata;
        double *params = (double*)(p.buffer);
        vrpn_FORCESCPCB tp;
        vrpn_FORCESCPCHANGELIST *handler = me->scp_change_list;
        int i;

        // Fill in the parameters to the tracker from the message
        if (p.payload_len != (7*sizeof(double))) {
                fprintf(stderr, "vrpn_ForceDevice: scp message payload");
                fprintf(stderr, " error\n(got %d, expected %d)\n",
                        p.payload_len, 7*sizeof(double));
                return -1;
        }
        tp.msg_time = p.msg_time;
        // Typecasting used to get the byte order correct on the doubles
        // that are coming from the other side.
        for (i = 0; i < 3; i++) {
                tp.pos[i] = ntohd(*params++);
        }
        for (i = 0; i < 4; i++) {
                tp.quat[i] =  ntohd(*params++);
        }

        // Go down the list of callbacks that have been registered.
        // Fill in the parameter and call each.
        while (handler != NULL) {
                handler->handler(handler->userdata, tp);
                handler = handler->next;
        }

        return 0;
}


