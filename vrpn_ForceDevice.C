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


int vrpn_ForceDevice::buffer(char **insertPt, vrpn_int32 *buflen, const vrpn_int32 value)
{
    vrpn_int32 netValue = htonl(value);
    vrpn_int32 length = sizeof(netValue);

    if (length > *buflen) {
	    fprintf(stderr, "buffer: buffer not large enough\n");
	    return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

int vrpn_ForceDevice::buffer(char **insertPt, vrpn_int32 *buflen, const vrpn_float32 value)
{
    vrpn_int32 longval = *((vrpn_int32 *)&value);

    return buffer(insertPt, buflen, longval);
}

int vrpn_ForceDevice::buffer(char **insertPt, vrpn_int32 *buflen,const vrpn_float64 value)
{
    vrpn_float64 netValue = htond(value);
    vrpn_int32 length = sizeof(netValue);

    if (length > *buflen) {
	    fprintf(stderr, "buffer: buffer not large enough\n");
	    return -1;
    }

    memcpy(*insertPt, &netValue, length);
    *insertPt += length;
    *buflen -= length;

    return 0;
}

int vrpn_ForceDevice::unbuffer(const char **buffer, vrpn_int32 *lval)
{
    *lval = ntohl(*((vrpn_int32 *)(*buffer)));
    *buffer += sizeof(vrpn_int32);
    return 0;
}

int vrpn_ForceDevice::unbuffer(const char **buffer, vrpn_float32 *fval)
{
    vrpn_int32 lval;
    unbuffer(buffer, &lval);
    *fval = *((vrpn_float32 *) &lval);
    return 0;
}

int vrpn_ForceDevice::unbuffer(const char **buffer, vrpn_float64 *dval){
    *dval = ntohd(*(vrpn_float64 *)(*buffer));
    *buffer += sizeof(vrpn_float64);
    return 0;
}

char *vrpn_ForceDevice::encode_force(vrpn_int32 &length, const vrpn_float64 *force)
{
    // Message includes: vrpn_float64 force[3]
    // Byte order of each needs to be reversed to match network standard

    int i;
    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    length = 3*sizeof(vrpn_float64);
    mlen = length;

    buf = new char [length];
    mptr = buf;

    // Move the force there
    for (i = 0; i < 3; i++){
	    buffer(&mptr, &mlen, force[i]);
    }

    return buf;
}

vrpn_int32 vrpn_ForceDevice::decode_force (const char *buffer, const vrpn_int32 len, 
				vrpn_float64 *force)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len !=  (3*sizeof(vrpn_float64)) ) {
      fprintf(stderr,"vrpn_ForceDevice: force message payload error\n");
      fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 3*sizeof(vrpn_float64) );
      return -1;
    }

    for (i = 0; i < 3; i++)
	    res = unbuffer(&mptr, &(force[i]));

    return res;
}

char *vrpn_ForceDevice::encode_scp(vrpn_int32 &length, 
				const vrpn_float64 *pos, const vrpn_float64 *quat)
{
    int i;
    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    length = 7*sizeof(vrpn_float64);
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

vrpn_int32 vrpn_ForceDevice::decode_scp(const char *buffer, const vrpn_int32 len,
				 vrpn_float64 *pos, vrpn_float64 *quat)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 7*sizeof(vrpn_float64)){
	    fprintf(stderr,"vrpn_ForceDevice: scp message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 7*sizeof(vrpn_float64) );
	    return -1;
    }

    for (i = 0; i < 3; i++)
	    res = unbuffer(&mptr, &(pos[i]));
    for (i = 0; i < 4; i++)
	    res = unbuffer(&mptr, &(quat[i]));

    return res;
}

char *vrpn_ForceDevice::encode_plane(vrpn_int32 &len, const vrpn_float32 *plane, 
				const vrpn_float32 kspring, const vrpn_float32 kdamp,
				const vrpn_float32 fdyn, const vrpn_float32 fstat, 
				const vrpn_int32 plane_index, const vrpn_int32 n_rec_cycles){
	// Message includes: vrpn_float32 plane[4],

	int i;
	char *buf;
	char *mptr;
	vrpn_int32 mlen;

	len = 8*sizeof(vrpn_float32)+2*sizeof(vrpn_int32);
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

vrpn_int32 vrpn_ForceDevice::decode_plane(const char *buffer, const vrpn_int32 len, 
				vrpn_float32 *plane, 
				vrpn_float32 *kspring, vrpn_float32 *kdamp,
				vrpn_float32 *fdyn, vrpn_float32 *fstat, 
				vrpn_int32 *plane_index, vrpn_int32 *n_rec_cycles)
{
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 8*sizeof(vrpn_float32)+2*sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: plane message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 8*sizeof(vrpn_float32)+2*sizeof(vrpn_int32) );
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

char *vrpn_ForceDevice::encode_surface_effects(vrpn_int32 &len, 
		    const vrpn_float32 k_adhesion,
		    const vrpn_float32 bump_amp, const vrpn_float32 bump_freq,
		    const vrpn_float32 buzz_amp, const vrpn_float32 buzz_freq) {

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = 5*sizeof(vrpn_float32);
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

vrpn_int32 vrpn_ForceDevice::decode_surface_effects(const char *buffer, const vrpn_int32 len,
					vrpn_float32 *k_adhesion,
					vrpn_float32 *bump_amp, vrpn_float32 *bump_freq,
					vrpn_float32 *buzz_amp, vrpn_float32 *buzz_freq) {

    int res;
    const char *mptr = buffer;

    if (len != 5*sizeof(vrpn_float32)){
        fprintf(stderr,"vrpn_ForceDevice: surface effects message payload ");
        fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 5*sizeof(vrpn_float32) );
	return -1;
    }

    res = unbuffer(&mptr, k_adhesion);
    res = unbuffer(&mptr, bump_amp);
    res = unbuffer(&mptr, bump_freq);
    res = unbuffer(&mptr, buzz_amp);
    res = unbuffer(&mptr, buzz_freq);

    return res;
}

char *vrpn_ForceDevice::encode_vertex(vrpn_int32 &len,const vrpn_int32 vertNum,
			const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32) + 3*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, vertNum);
    buffer(&mptr, &mlen, x);
    buffer(&mptr, &mlen, y);
    buffer(&mptr, &mlen, z);

    return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_vertex(const char *buffer, 
			    const vrpn_int32 len,vrpn_int32 *vertNum,
			    vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z){
    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32) + 3*sizeof(vrpn_float32)){
	    fprintf(stderr,"vrpn_ForceDevice: vertex message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) + 3*sizeof(vrpn_float32) );
	    return -1;
    }

    res = unbuffer(&mptr, vertNum);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);

    return res;
}

char *vrpn_ForceDevice::encode_normal(vrpn_int32 &len,const vrpn_int32 normNum,
		       const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32) + 3*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;
    buffer(&mptr, &mlen, normNum);
    buffer(&mptr, &mlen, x);
    buffer(&mptr, &mlen, y);
    buffer(&mptr, &mlen, z);

    return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_normal(const char *buffer,const vrpn_int32 len,
			     vrpn_int32 *vertNum,vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32) + 3*sizeof(vrpn_float32)){
	    fprintf(stderr,"vrpn_ForceDevice: normal message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) + 3*sizeof(vrpn_float32) );
	    return -1;
    }

    res = unbuffer(&mptr, vertNum);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);

    return res;
}

char *vrpn_ForceDevice::encode_triangle(vrpn_int32 &len,const vrpn_int32 triNum,
			 const vrpn_int32 vert0,const vrpn_int32 vert1,const vrpn_int32 vert2,
			 const vrpn_int32 norm0,const vrpn_int32 norm1,const vrpn_int32 norm2){
    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = 7*sizeof(vrpn_int32);
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

vrpn_int32 vrpn_ForceDevice::decode_triangle(const char *buffer,
				const vrpn_int32 len,vrpn_int32 *triNum,
			    vrpn_int32 *vert0,vrpn_int32 *vert1,vrpn_int32 *vert2,
			    vrpn_int32 *norm0,vrpn_int32 *norm1,vrpn_int32 *norm2)
{
    int res;
    const char *mptr = buffer;

    if (len != 7*sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: triangle message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 7*sizeof(vrpn_int32) );
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

char *vrpn_ForceDevice::encode_removeTriangle(vrpn_int32 &len,const vrpn_int32 triNum){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, triNum);

    return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_removeTriangle(const char *buffer,
				const vrpn_int32 len,vrpn_int32 *triNum){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	fprintf(stderr,"vrpn_ForceDevice: remove triangle message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    res = unbuffer(&mptr, triNum);

    return res;
}

// this is where we send down our surface parameters
char *vrpn_ForceDevice::encode_updateTrimeshChanges(vrpn_int32 &len, 
			const vrpn_float32 kspring, const vrpn_float32 kdamp, 
			const vrpn_float32 fstat, const vrpn_float32 fdyn){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = 4*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, kspring);
    buffer(&mptr, &mlen, kdamp);
    buffer(&mptr, &mlen, fstat);
    buffer(&mptr, &mlen, fdyn);

    return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_updateTrimeshChanges(const char *buffer,
			const vrpn_int32 len, vrpn_float32 *kspring, vrpn_float32 *kdamp, 
			vrpn_float32 *fstat, vrpn_float32 *fdyn){

    int res;
    const char *mptr = buffer;

    if (len != 4*sizeof(vrpn_float32)){
	fprintf(stderr,"vrpn_ForceDevice: update trimesh message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, 4*sizeof(vrpn_float32) );
	    return -1;
    }

    res = unbuffer(&mptr, kspring);
    res = unbuffer(&mptr, kdamp);
    res = unbuffer(&mptr, fstat);
    res = unbuffer(&mptr, fdyn);

    return res;
}

char *vrpn_ForceDevice::encode_setTrimeshType(vrpn_int32 &len,const vrpn_int32 type){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, type);

    return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_setTrimeshType(const char *buffer,const vrpn_int32 len,
					   vrpn_int32 *type){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh type message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    res = unbuffer(&mptr, type);

    return res;
}

char *vrpn_ForceDevice::encode_trimeshTransform(vrpn_int32 &len,
				const vrpn_float32 homMatrix[16]){
	int i;
	char *buf;
	char *mptr;
	vrpn_int32 mlen;

	len = 16*sizeof(vrpn_float32);
	mlen = len;
	
	buf = new char [len];
	mptr = buf;
  
	for(i = 0; i < 16; i++)
		buffer(&mptr, &mlen, homMatrix[i]);    

	return buf; 
}

vrpn_int32 vrpn_ForceDevice::decode_trimeshTransform(const char *buffer,
				  const vrpn_int32 len, vrpn_float32 homMatrix[16]){
    int i;
    int res;
    const char *mptr = buffer;

    if (len != 16*sizeof(vrpn_float32)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh transform message payload ");
	    fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 16*sizeof(vrpn_float32) );
	    return -1;
    }

    for (i = 0; i < 16; i++)
	    res = unbuffer(&mptr, &(homMatrix[i]));

    return res;
}

char *vrpn_ForceDevice::encode_constraint(vrpn_int32 &len, const vrpn_int32 enable,
		const vrpn_float32 x, const vrpn_float32 y, const vrpn_float32 z,const vrpn_float32 kSpr){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32) + 4*sizeof(vrpn_float32);
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

vrpn_int32 vrpn_ForceDevice::decode_constraint(const char *buffer, 
			     const vrpn_int32 len,vrpn_int32 *enable, 
			     vrpn_float32 *x, vrpn_float32 *y, vrpn_float32 *z, vrpn_float32 *kSpr){

    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32) + 4*sizeof(vrpn_float32)){
	fprintf(stderr,"vrpn_ForceDevice: constraint message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) + 4*sizeof(vrpn_float32) );
	    return -1;
    }

    res = unbuffer(&mptr, enable);
    res = unbuffer(&mptr, x);
    res = unbuffer(&mptr, y);
    res = unbuffer(&mptr, z);
    res = unbuffer(&mptr, kSpr);

    return res;
}

char *vrpn_ForceDevice::encode_forcefield(vrpn_int32 &len, const vrpn_float32 origin[3],
	const vrpn_float32 force[3], const vrpn_float32 jacobian[3][3], const vrpn_float32 radius)
{
    int i,j;
    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = 16*sizeof(vrpn_float32);
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

vrpn_int32 vrpn_ForceDevice::decode_forcefield(const char *buffer,
			  const vrpn_int32 len,vrpn_float32 origin[3],
			  vrpn_float32 force[3], vrpn_float32 jacobian[3][3], vrpn_float32 *radius){
    int i,j;
    int res;
    const char *mptr = buffer;

    if (len != 16*sizeof(vrpn_float32)){
       fprintf(stderr,"vrpn_ForceDevice: force field message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 16*sizeof(vrpn_float32) );
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

char *vrpn_ForceDevice::encode_error(vrpn_int32 &len, const vrpn_int32 error_code)
{

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    buffer(&mptr, &mlen, error_code);

    return buf;
}

vrpn_int32 vrpn_ForceDevice::decode_error(const char *buffer,
				   const vrpn_int32 len,vrpn_int32 *error_code)
{

    int res;
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: error message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    res = unbuffer(&mptr, error_code);

    return res;
}

void vrpn_ForceDevice::set_plane(vrpn_float32 *p)
{ 
    for(int i=0;i<4;i++ ) {
	    plane[i]= p[i];
    }
}

void vrpn_ForceDevice::set_plane(vrpn_float32 a, vrpn_float32 b, vrpn_float32 c,vrpn_float32 d)
{ plane[0] = a; 
  plane[1] = b;
  plane[2] = c;
  plane[3] = d;
}

void vrpn_ForceDevice::set_plane(vrpn_float32 *normal, vrpn_float32 d)
{ 
    for(int i =0;i<3;i++ ) {
      plane[i] = normal[i];
    }

    plane[3] = d;
}

void vrpn_ForceDevice::sendError(int error_code){
    char	*msgbuf;
    vrpn_int32	len;
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
  vrpn_int32		len;
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
    vrpn_int32		len;
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
    vrpn_int32		len;
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

void vrpn_ForceDevice_Remote::setVertex(vrpn_int32 vertNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z){

  char	*msgbuf;
  vrpn_int32		len;
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

void vrpn_ForceDevice_Remote::setNormal(vrpn_int32 normNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z){
  char	*msgbuf;
  vrpn_int32		len;
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

void vrpn_ForceDevice_Remote::setTriangle(vrpn_int32 triNum,
	  vrpn_int32 vert0,vrpn_int32 vert1,vrpn_int32 vert2,vrpn_int32 norm0,vrpn_int32 norm1,vrpn_int32 norm2){

  char	*msgbuf;
  vrpn_int32		len;
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

void vrpn_ForceDevice_Remote::removeTriangle(vrpn_int32 triNum){

  char	*msgbuf;
  vrpn_int32		len;
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
  vrpn_int32		len;
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
void vrpn_ForceDevice_Remote::setTrimeshTransform(vrpn_float32 homMatrix[16]){
  char	*msgbuf;
  vrpn_int32		len;
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
  vrpn_int32		len=0;
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
  vrpn_int32		len;
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
  vrpn_int32		len;
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


void vrpn_ForceDevice_Remote::sendConstraint(vrpn_int32 enable, 
					vrpn_float32 x, vrpn_float32 y, vrpn_float32 z, vrpn_float32 kSpr)
{
  char *msgbuf;
  vrpn_int32	len;
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

void vrpn_ForceDevice_Remote::sendForceField(vrpn_float32 origin[3],
	vrpn_float32 force[3], vrpn_float32 jacobian[3][3], vrpn_float32 radius)
{
  char *msgbuf;
  vrpn_int32   len;
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
  vrpn_float32 origin[3] = {0,0,0};
  vrpn_float32 force[3] = {0,0,0};
  vrpn_float32 jacobian[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
  vrpn_float32 radius = 0;

  char *msgbuf;
  vrpn_int32   len;
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


void	vrpn_ForceDevice_Remote::mainloop(const struct timeval * timeout)
{
	if (connection) { connection->mainloop(timeout); }
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

    if (p.payload_len != sizeof(vrpn_int32)) {
	    fprintf(stderr, "vrpn_ForceDevice: error message payload"
		    " error\n(got %d, expected %d)\n",
		    p.payload_len, sizeof(vrpn_int32));
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
