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

/* cheezy hack to make sure this enum is defined in the case we didn't 
   include trimesh.h */
#ifndef TRIMESH_H
// the different types of trimeshes we have available for haptic display
enum TrimeshType {GHOST,HCOLLIDE};
#endif

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
		forcefield_message_id = 
			connection->register_message_type("Force Field");

		plane_message_id = connection->register_message_type("Plane");
		setVertex_message_id = 
		  connection->register_message_type("setVertex");
		setNormal_message_id =
		  connection->register_message_type("setNormal");
		setTriangle_message_id = 
		  connection->register_message_type("setTriangle");
		removeTriangle_message_id = 
		  connection->register_message_type("removeTriangle");
		updateTrimeshChanges_message_id = 
		  connection->register_message_type("updateTrimeshChanges");
		transformTrimesh_message_id = 
		  connection->register_message_type("transformTrimesh");
		setTrimeshType_message_id = 
		  connection->register_message_type("setTrimeshType");
		clearTrimesh_message_id = 
		  connection->register_message_type("clearTrimesh");
		scp_message_id = connection->register_message_type("SCP");
		set_constraint_message_id = 
			connection->register_message_type("CONSTRAINT");
		error_message_id = connection->register_message_type
					("Force Error");
	}

	//set the current time to zero
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	//set the force to zero
	force[0] = force[1] = force[2] = 0.0;
	SurfaceKspring= 0.29f;
	SurfaceFdynamic = 0.02f;
	SurfaceFstatic = 0.03f;
	SurfaceKdamping = 0.0f;

  if (servicename)
    delete [] servicename;
}

void vrpn_ForceDevice::print_plane(void)
{
  printf("plane: %f, %f, %f, %f\n", plane[0], plane[1], plane[2], plane[3]);
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

vrpn_ForceDevice_Remote::vrpn_ForceDevice_Remote(char *name):
	vrpn_ForceDevice(name,vrpn_get_connection_by_name(name)),
	change_list(NULL), scp_change_list(NULL), error_change_list(NULL)
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

     // Register a handler for the error change callback from this device.
     if (connection->register_handler(error_message_id, 
	handle_error_change_message, this, my_id)) {
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
   if(index*sizeof(unsigned long)!=(unsigned)len){
     fprintf(stderr,
	     "ERROR: incorrect # of parameters in vrpn_ForceDevice_Remote::encode_plane\n");
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

char *vrpn_ForceDevice_Remote::encode_normal(int &len,int normNum,
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

  longbuf[index++] = *(unsigned long*)(void*)(&normNum);
  longbuf[index++] = *(unsigned long*)(void*)(&x);
  longbuf[index++] = *(unsigned long*)(void*)(&y);
  longbuf[index++] = *(unsigned long*)(void*)(&z);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_triangle(int &len,int triNum,
					       int vert0,int vert1,int vert2,
					       int norm0,int norm1,int norm2){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (7)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&triNum);
  longbuf[index++] = *(unsigned long*)(void*)(&vert0);
  longbuf[index++] = *(unsigned long*)(void*)(&vert1);
  longbuf[index++] = *(unsigned long*)(void*)(&vert2);
  longbuf[index++] = *(unsigned long*)(void*)(&norm0);
  longbuf[index++] = *(unsigned long*)(void*)(&norm1);
  longbuf[index++] = *(unsigned long*)(void*)(&norm2);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

char *vrpn_ForceDevice_Remote::encode_removeTriangle(int &len,int triNum){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = (1)*sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&triNum);
 
  for (i = 0; i < index; i++) {
    longbuf[i] = htonl(longbuf[i]);
  }
  return buf; 
}

// this is where we send down our surface parameters
char *vrpn_ForceDevice_Remote::encode_updateTrimeshChanges(int &len){
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

char *vrpn_ForceDevice_Remote::encode_setTrimeshType(int &len,int type){
  // Byte order of each needs to be reversed to match network standard
  // This moving is done by horrible typecast hacks. 
  // someone did this in the encode_plane(), so I just moused it on down (AG)

  // count the number of parameters we're sending
  len = sizeof(unsigned long);
  // allocate the buffer for the message
  char *buf=new char[len];

  int i;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int	index = 0;

  longbuf[index++] = *(unsigned long*)(void*)(&type);

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

char *vrpn_ForceDevice_Remote::encode_forcefield(int &len, float origin[3],
	float force[3], float jacobian[3][3], float radius)
{
  len = 16*sizeof(unsigned long);
  char *buf = new char[len];

  int i,j;
  unsigned long *longbuf = (unsigned long*)(void*)(buf);
  int index = 0;

  for (i=0;i<3;i++){
    longbuf[index] = *(unsigned long *)(void*)(&(origin[i]));
    index++;
  }
  for (i=0;i<3;i++){
    longbuf[index] = *(unsigned long *)(void*)(&(force[i]));
    index++;
  }
  for (i=0;i<3;i++)
     for (j=0;j<3;j++){
	longbuf[index] = *(unsigned long *)(void*)(&(jacobian[i][j]));
	index++;
     }
  longbuf[index] = *(unsigned long *)(void*)(&radius);

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

void vrpn_ForceDevice_Remote::setVertex(int vertNum,float x,float y,float z){

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
    delete []msgbuf;
  }
}

void vrpn_ForceDevice_Remote::setNormal(int normNum,float x,float y,float z){
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_normal(len,normNum,x,y,z);
    if (connection->pack_message(len,timestamp,setNormal_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

void vrpn_ForceDevice_Remote::setTriangle(int triNum,
					  int vert0,int vert1,int vert2,
					  int norm0,int norm1,int norm2){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_triangle(len,triNum,vert0,vert1,vert2,norm0,norm1,norm2);
    if (connection->pack_message(len,timestamp,setTriangle_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

void vrpn_ForceDevice_Remote::removeTriangle(int triNum){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_removeTriangle(len,triNum);
    if (connection->pack_message(len,timestamp,removeTriangle_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

void vrpn_ForceDevice_Remote::updateTrimeshChanges(){

  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_updateTrimeshChanges(len);
    if (connection->pack_message(len,timestamp,updateTrimeshChanges_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

// set the trimesh's homogen transform matrix (in row major order)
void vrpn_ForceDevice_Remote::setTrimeshTransform(float homMatrix[16]){
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
    delete []msgbuf;
  }
}

// clear the triMesh if connection
void vrpn_ForceDevice_Remote::clearTrimesh(void){
  char	*msgbuf=NULL;
  int		len=0;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    if (connection->pack_message(len,timestamp,clearTrimesh_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

// the next time we send a trimesh we will use the following type
void vrpn_ForceDevice_Remote::useHcollide(void){
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_setTrimeshType(len,HCOLLIDE);
    if (connection->pack_message(len,timestamp,setTrimeshType_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
  }
}

void vrpn_ForceDevice_Remote::useGhost(void){
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_setTrimeshType(len,GHOST);
    if (connection->pack_message(len,timestamp,setTrimeshType_message_id,
				 my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete []msgbuf;
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

void vrpn_ForceDevice_Remote::sendForceField()
{
    sendForceField(ff_origin, ff_force, ff_jacobian, ff_radius);
}

void vrpn_ForceDevice_Remote::sendForceField(float origin[3],
	float force[3], float jacobian[3][3], float radius)
{
  char *msgbuf;
  int   len;
  struct timeval current_time;

  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_forcefield(len, origin, force, jacobian, radius);
    if (connection->pack_message(len,timestamp, forcefield_message_id,
		my_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    connection->mainloop();
    delete msgbuf;
  }
}

// same as sendForceField but sets force to 0 and sends RELIABLY
void vrpn_ForceDevice_Remote::stopForceField()
{
  float origin[3] = {0,0,0};
  float force[3] = {0,0,0};
  float jacobian[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
  float radius = 0;

  char *msgbuf;
  int   len;
  struct timeval current_time;

  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(connection){
    msgbuf = encode_forcefield(len, origin, force, jacobian, radius);
    if (connection->pack_message(len,timestamp, forcefield_message_id,
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

int vrpn_ForceDevice_Remote::register_error_handler(void *userdata,
                        vrpn_FORCEERRORHANDLER handler)
{
        vrpn_FORCEERRORCHANGELIST    *new_entry;

        // Ensure that the handler is non-NULL
        if (handler == NULL) {
            fprintf(stderr,
             "vrpn_ForceDevice_Remote::register_error_handler: NULL handler\n");
            return -1;
        }

        // Allocate and initialize the new entry
        if ( (new_entry = new vrpn_FORCEERRORCHANGELIST) == NULL) {
           fprintf(stderr,
            "vrpn_ForceDevice_Remote::register_error_handler: Out of memory\n");
           return -1;
        }
        new_entry->handler = handler;
        new_entry->userdata = userdata;

        // Add this handler to the chain at the beginning (don't check to see
        // if it is already there, since duplication is okay).
        new_entry->next = error_change_list;
        error_change_list = new_entry;

        return 0;
}

int vrpn_ForceDevice_Remote::unregister_error_handler(void *userdata,
                        vrpn_FORCEERRORHANDLER handler)
{
        // The pointer at *snitch points to victim
        vrpn_FORCEERRORCHANGELIST    *victim, **snitch;

        // Find a handler with this registry in the list (any one will do,
        // since all duplicates are the same).
        snitch = &error_change_list;
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
         "vrpn_ForceDevice_Remote::unregister_errorhandler: No such handler\n");
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


int vrpn_ForceDevice_Remote::handle_error_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_ForceDevice_Remote *me = (vrpn_ForceDevice_Remote *)userdata;
	long *params = (long *)(p.buffer);
	vrpn_FORCEERRORCB tp;
	vrpn_FORCEERRORCHANGELIST *handler = me->error_change_list;
	int i;

	if (p.payload_len != sizeof(long)) {
		fprintf(stderr, "vrpn_ForceDevice: error message payload",
			" error\n(got %d, expected %d)\n",
			p.payload_len, sizeof(long));
		return -1;
	}
	tp.msg_time = p.msg_time;
	tp.error_code = ntohl(*params);

	// Go down the list of callbacks that have been registered.
	while (handler != NULL) {
		handler->handler(handler->userdata, tp);
		handler = handler->next;
	}
	return 0;
}
