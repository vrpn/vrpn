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


long vrpn_ForceDevice::buffer(char **insertPt, long *buflen, const long value)
{
    long netValue = htonl(value);
    long length = sizeof(netValue);

    if (length > *buflen) {
	    fprintf(stderr, "buffer: buffer not large enough\n");
	    return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

long vrpn_ForceDevice::buffer(char **insertPt, long *buflen, const float value)
{
    long longval = *((long *)&value);

    return buffer(insertPt, buflen, longval);
}

long vrpn_ForceDevice::buffer(char **insertPt, long *buflen,const double value)
{
    double netValue = htond(value);
    long length = sizeof(netValue);

    if (length > *buflen) {
	    fprintf(stderr, "buffer: buffer not large enough\n");
	    return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

long vrpn_ForceDevice::unbuffer(const char **buffer, long *lval)
{
    *lval = ntohl(*((long *)(*buffer)));
    *buffer += sizeof(long);
    return 0;
}

long vrpn_ForceDevice::unbuffer(const char **buffer, float *fval)
{
    long lval;
    unbuffer(buffer, &lval);
    *fval = *((float *) &lval);
    return 0;
}

long vrpn_ForceDevice::unbuffer(const char **buffer, double *dval){
    *dval = ntohd(*(double *)(*buffer));
    *buffer += sizeof(double);
    return 0;
}

char *vrpn_ForceDevice::encode_force(int &length, const double *force)
{
    // Message includes: double force[3]
    // Byte order of each needs to be reversed to match network standard

    int i;
    char *buf;
    char *mptr;
    long mlen;

    length = 3*sizeof(double);
    mlen = length;

    buf = new char [length];
    mptr = buf;

    // Move the force there
    for (i = 0; i < 3; i++){
	    buffer(&mptr, &mlen, force[i]);
    }

    return buf;
}

int vrpn_ForceDevice::decode_force (const char *buffer, const int len, 
				double *force)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len !=  (3*sizeof(double)) ) {
      fprintf(stderr,"vrpn_ForceDevice: force message payload error\n");
      fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 3*sizeof(double) );
      return -1;
    }

    for (i = 0; i < 3; i++)
	    res = unbuffer(&mptr, &(force[i]));

    return res;
}

char *vrpn_ForceDevice::encode_scp(int &length, 
				const double *pos, const double *quat)
{
    int i;
    char *buf;
    char *mptr;
    long mlen;

    length = 7*sizeof(double);
    mlen = length;

    buf = new char [length];
    mptr = buf;

    for (i = 0; i < 3; i++) {
        buffer(&mptr, &mlen, pos[i]);
    }
    for (i = 0; i < 4; i++) {
        buffer(&mptr, &mlen, quat[i]);
    }

    return buf;
}

int vrpn_ForceDevice::decode_scp(const char *buffer, const int len,
				 double *pos, double *quat)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 7*sizeof(double)){
	    fprintf(stderr,"vrpn_ForceDevice: scp message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 7*sizeof(double) );
	    return -1;
    }

    for (i = 0; i < 3; i++)
	    res = unbuffer(&mptr, &(pos[i]));
    for (i = 0; i < 4; i++)
	    res = unbuffer(&mptr, &(quat[i]));

    return res;
}

char *vrpn_ForceDevice::encode_plane(int &len, const float *plane, 
				const float kspring, const float kdamp,
				const float fdyn, const float fstat, 
				const long plane_index, const long n_rec_cycles){
	// Message includes: float plane[4],

	int i;
	char *buf;
	char *mptr;
	long mlen;

	len = 8*sizeof(float)+2*sizeof(long);
	mlen = len;
	
	buf = new char [len];
	mptr = buf;

	for (i = 0; i < 4; i++){
		buffer(&mptr, &mlen, plane[i]);
	}

	buffer(&mptr, &mlen, kspring);
	buffer(&mptr, &mlen, kdamp);
	buffer(&mptr, &mlen, fdyn);
	buffer(&mptr, &mlen, fstat);
	buffer(&mptr, &mlen, plane_index);
	buffer(&mptr, &mlen, n_rec_cycles);

	return buf;
}

int vrpn_ForceDevice::decode_plane(const char *buffer, const int len, 
				float *plane, 
				float *kspring, float *kdamp,
				float *fdyn, float *fstat, 
				long *plane_index, long *n_rec_cycles)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 8*sizeof(float)+2*sizeof(long)){
	    fprintf(stderr,"vrpn_ForceDevice: plane message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 8*sizeof(float)+2*sizeof(long) );
	    return -1;
    }

    for (i = 0; i < 4; i++)
	    res = unbuffer(&mptr, &(plane[i]));
    res = unbuffer(&mptr, kspring);
    res = unbuffer(&mptr, kdamp);
    res = unbuffer(&mptr, fdyn);
    res = unbuffer(&mptr, fstat);
    res = unbuffer(&mptr, plane_index);
    res = unbuffer(&mptr, n_rec_cycles);

    return res;
}

char *vrpn_ForceDevice::encode_surface_effects(int &len, 
		    const float k_adhesion,
		    const float bump_amp, const float bump_freq,
		    const float buzz_amp, const float buzz_freq) {

    char *buf;
    char *mptr;
    long mlen;

    len = 5*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, k_adhesion);
    buffer(&mptr, &mlen, bump_amp);
    buffer(&mptr, &mlen, bump_freq);
    buffer(&mptr, &mlen, buzz_amp);
    buffer(&mptr, &mlen, buzz_freq);

    return buf;
}

int vrpn_ForceDevice::decode_surface_effects(const char *buffer, const int len,
					float *k_adhesion,
					float *bump_amp, float *bump_freq,
					float *buzz_amp, float *buzz_freq) {

    int res;
    const char *mptr = buffer;

    if (len != 5*sizeof(float)){
        fprintf(stderr,"vrpn_ForceDevice: surface effects message payload ");
        fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 5*sizeof(float) );
	return -1;
    }

    res = unbuffer(&mptr, k_adhesion);
    res = unbuffer(&mptr, bump_amp);
    res = unbuffer(&mptr, bump_freq);
    res = unbuffer(&mptr, buzz_amp);
    res = unbuffer(&mptr, buzz_freq);

    return res;
}

char *vrpn_ForceDevice::encode_vertex(int &len,const long vertNum,
			const float x,const float y,const float z){

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long) + 3*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, vertNum);
    buffer(&mptr, &mlen, x);
    buffer(&mptr, &mlen, y);
    buffer(&mptr, &mlen, z);

    return buf; 
}

int vrpn_ForceDevice::decode_vertex(const char *buffer, 
			    const int len,long *vertNum,
			    float *x,float *y,float *z){
    int res;
    const char *mptr = buffer;

    if (len != sizeof(long) + 3*sizeof(float)){
	    fprintf(stderr,"vrpn_ForceDevice: vertex message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(long) + 3*sizeof(float) );
	    return -1;
    }

    res = unbuffer(&mptr, vertNum);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);

    return res;
}

char *vrpn_ForceDevice::encode_normal(int &len,const long normNum,
		       const float x,const float y,const float z){

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long) + 3*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;
    buffer(&mptr, &mlen, normNum);
    buffer(&mptr, &mlen, x);
    buffer(&mptr, &mlen, y);
    buffer(&mptr, &mlen, z);

    return buf; 
}

int vrpn_ForceDevice::decode_normal(const char *buffer,const int len,
			     long *vertNum,float *x,float *y,float *z){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(long) + 3*sizeof(float)){
	    fprintf(stderr,"vrpn_ForceDevice: normal message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(long) + 3*sizeof(float) );
	    return -1;
    }

    res = unbuffer(&mptr, vertNum);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);

    return res;
}

char *vrpn_ForceDevice::encode_triangle(int &len,const long triNum,
			 const long vert0,const long vert1,const long vert2,
			 const long norm0,const long norm1,const long norm2){
    char *buf;
    char *mptr;
    long mlen;

    len = 7*sizeof(long);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, triNum);
    buffer(&mptr, &mlen, vert0);
    buffer(&mptr, &mlen, vert1);
    buffer(&mptr, &mlen, vert2);
    buffer(&mptr, &mlen, norm0);
    buffer(&mptr, &mlen, norm1);
    buffer(&mptr, &mlen, norm2);

    return buf; 
}

int vrpn_ForceDevice::decode_triangle(const char *buffer,
				const int len,long *triNum,
			    long *vert0,long *vert1,long *vert2,
			    long *norm0,long *norm1,long *norm2)
{
    int res;
    const char *mptr = buffer;

    if (len != 7*sizeof(long)){
	    fprintf(stderr,"vrpn_ForceDevice: triangle message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 7*sizeof(long) );
	    return -1;
    }

    res = unbuffer(&mptr, triNum);
    res = unbuffer(&mptr, vert0);
    res = unbuffer(&mptr, vert1);
    res = unbuffer(&mptr, vert2);
    res = unbuffer(&mptr, norm0);
    res = unbuffer(&mptr, norm1);
    res = unbuffer(&mptr, norm2);

    return res;
}

char *vrpn_ForceDevice::encode_removeTriangle(int &len,const long triNum){

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, triNum);

    return buf; 
}

int vrpn_ForceDevice::decode_removeTriangle(const char *buffer,
				const int len,long *triNum){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(long)){
	fprintf(stderr,"vrpn_ForceDevice: remove triangle message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(long) );
	    return -1;
    }

    res = unbuffer(&mptr, triNum);

    return res;
}

// this is where we send down our surface parameters
char *vrpn_ForceDevice::encode_updateTrimeshChanges(int &len, 
			const float kspring, const float kdamp, 
			const float fstat, const float fdyn){

    char *buf;
    char *mptr;
    long mlen;

    len = 4*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, kspring);
    buffer(&mptr, &mlen, kdamp);
    buffer(&mptr, &mlen, fstat);
    buffer(&mptr, &mlen, fdyn);

    return buf; 
}

int vrpn_ForceDevice::decode_updateTrimeshChanges(const char *buffer,
			const int len, float *kspring, float *kdamp, 
			float *fstat, float *fdyn){

    int res;
    const char *mptr = buffer;

    if (len != 4*sizeof(float)){
	fprintf(stderr,"vrpn_ForceDevice: update trimesh message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, 4*sizeof(float) );
	    return -1;
    }

    res = unbuffer(&mptr, kspring);
    res = unbuffer(&mptr, kdamp);
    res = unbuffer(&mptr, fstat);
    res = unbuffer(&mptr, fdyn);

    return res;
}

char *vrpn_ForceDevice::encode_setTrimeshType(int &len,const long type){

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, type);

    return buf; 
}

int vrpn_ForceDevice::decode_setTrimeshType(const char *buffer,const int len,
					   long *type){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(long)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh type message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(long) );
	    return -1;
    }

    res = unbuffer(&mptr, type);

    return res;
}

char *vrpn_ForceDevice::encode_trimeshTransform(int &len,
				const float homMatrix[16]){
	int i;
	char *buf;
	char *mptr;
	long mlen;

	len = 16*sizeof(float);
	mlen = len;
	
	buf = new char [len];
	mptr = buf;
  
	for(i = 0; i < 16; i++)
		buffer(&mptr, &mlen, homMatrix[i]);    

	return buf; 
}

int vrpn_ForceDevice::decode_trimeshTransform(const char *buffer,
				  const int len, float homMatrix[16]){
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 16*sizeof(float)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh transform message payload ");
	    fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 16*sizeof(float) );
	    return -1;
    }

    for (i = 0; i < 16; i++)
	    res = unbuffer(&mptr, &(homMatrix[i]));

    return res;
}

char *vrpn_ForceDevice::encode_constraint(int &len, const long enable,
		const float x, const float y, const float z,const float kSpr){

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long) + 4*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, enable);
    buffer(&mptr, &mlen, x);
    buffer(&mptr, &mlen, y);
    buffer(&mptr, &mlen, z);
    buffer(&mptr, &mlen, kSpr);

    return buf;
}

int vrpn_ForceDevice::decode_constraint(const char *buffer, 
			     const int len,long *enable, 
			     float *x, float *y, float *z, float *kSpr){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(long) + 4*sizeof(float)){
	fprintf(stderr,"vrpn_ForceDevice: constraint message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(long) + 4*sizeof(float) );
	    return -1;
    }

    res = unbuffer(&mptr, enable);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);
    res = unbuffer(&mptr, kSpr);

    return res;
}

char *vrpn_ForceDevice::encode_forcefield(int &len, const float origin[3],
	const float force[3], const float jacobian[3][3], const float radius)
{
    int i,j;
    char *buf;
    char *mptr;
    long mlen;

    len = 16*sizeof(float);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    for (i=0;i<3;i++)
    buffer(&mptr, &mlen, origin[i]);

    for (i=0;i<3;i++)
    buffer(&mptr, &mlen, force[i]);

    for (i=0;i<3;i++)
	    for (j=0;j<3;j++)
		    buffer(&mptr, &mlen, jacobian[i][j]);

    buffer(&mptr, &mlen, radius);

    return buf;
}

int vrpn_ForceDevice::decode_forcefield(const char *buffer,
			  const int len,float origin[3],
			  float force[3], float jacobian[3][3], float *radius){
    int i,j;
    int res;
    const char *mptr = buffer;

    if (len != 16*sizeof(float)){
       fprintf(stderr,"vrpn_ForceDevice: force field message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 16*sizeof(float) );
	    return -1;
    }

    for (i=0;i<3;i++)
	    res = unbuffer(&mptr, &(origin[i]));

    for (i=0;i<3;i++)
	    res = unbuffer(&mptr, &(force[i]));

    for (i=0;i<3;i++)
	    for (j=0;j<3;j++)
		    res = unbuffer(&mptr, &(jacobian[i][j]));

    res = unbuffer(&mptr, radius);

    return res;
}

char *vrpn_ForceDevice::encode_error(int &len, const long error_code)
{

    char *buf;
    char *mptr;
    long mlen;

    len = sizeof(long);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, error_code);

    return buf;
}

int vrpn_ForceDevice::decode_error(const char *buffer,
				   const int len,long *error_code){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(long)){
	    fprintf(stderr,"vrpn_ForceDevice: error message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(long) );
	    return -1;
    }

    res = unbuffer(&mptr, error_code);

    return res;
}

void vrpn_ForceDevice::set_plane(float *p)
{ 
    for(int i=0;i<4;i++ ) {
	    plane[i]= p[i];
    }
}

void vrpn_ForceDevice::set_plane(float a, float b, float c,float d)
{ plane[0] = a; 
  plane[1] = b;
  plane[2] = c;
  plane[3] = d;
}

void vrpn_ForceDevice::set_plane(float *normal, float d)
{ 
    for(int i =0;i<3;i++ ) {
      plane[i] = normal[i];
    }

    plane[3] = d;
}

void vrpn_ForceDevice::sendError(int error_code){
    char	*msgbuf;
    int		len;
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if(connection) {
	msgbuf = encode_error(len, error_code);
	if(connection->pack_message(len,timestamp,error_message_id,
				my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	  fprintf(stderr,"Phantom: cannot write message: tossing\n");
	}
	connection->mainloop();
	delete msgbuf;
    }
}

/* ******************** vrpn_ForceDevice_Remote ********************** */

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
     if (connection->register_handler(force_message_id, 
			handle_force_change_message,this, my_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    connection = NULL;
     }

     // Register a handler for the scp change callback from this device.
     if (connection->register_handler(scp_message_id,
	handle_scp_change_message, this, my_id)) {
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

void vrpn_ForceDevice_Remote::sendSurface(void)
{ // Encode the plane if there is a connection
  char	*msgbuf;
  int		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;
  
  if(connection) {
    msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
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
      msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
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
      msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
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
	  int vert0,int vert1,int vert2,int norm0,int norm1,int norm2){

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
    msgbuf = encode_updateTrimeshChanges(len, SurfaceKspring, SurfaceKdamping,
			SurfaceFstatic, SurfaceFdynamic);
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
    //delete []msgbuf;
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
    delete []msgbuf;
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
    delete []msgbuf;
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
    delete []msgbuf;
  }
}


void	vrpn_ForceDevice_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); }
}

int vrpn_ForceDevice_Remote::register_force_change_handler(void *userdata,
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

int vrpn_ForceDevice_Remote::unregister_force_change_handler(void *userdata,
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

int vrpn_ForceDevice_Remote::handle_force_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    vrpn_ForceDevice_Remote *me = (vrpn_ForceDevice_Remote *)userdata;
    vrpn_FORCECB	tp;
    vrpn_FORCECHANGELIST *handler = me->change_list;

    tp.msg_time = p.msg_time;
    decode_force(p.buffer, p.payload_len, tp.force);

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
    vrpn_FORCESCPCB tp;
    vrpn_FORCESCPCHANGELIST *handler = me->scp_change_list;

    tp.msg_time = p.msg_time;
    decode_scp(p.buffer, p.payload_len, tp.pos, tp.quat);

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
    vrpn_FORCEERRORCB tp;
    vrpn_FORCEERRORCHANGELIST *handler = me->error_change_list;

    if (p.payload_len != sizeof(long)) {
	    fprintf(stderr, "vrpn_ForceDevice: error message payload",
		    " error\n(got %d, expected %d)\n",
		    p.payload_len, sizeof(long));
	    return -1;
    }
    tp.msg_time = p.msg_time;
    decode_error(p.buffer, p.payload_len, &(tp.error_code));

    // Go down the list of callbacks that have been registered.
    while (handler != NULL) {
	    handler->handler(handler->userdata, tp);
	    handler = handler->next;
    }
    return 0;
}
