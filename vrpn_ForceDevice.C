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

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"
//#include <strings.h>
//#include <netinet/in.h>
//#include <sys/ioctl.h>

#include "vrpn_ForceDevice.h"

#include <quat.h>  // quaternion for spring stuff

/* cheezy hack to make sure this enum is defined in the case we didn't 
   include trimesh.h */
#ifndef TRIMESH_H
// the different types of trimeshes we have available for haptic display
enum TrimeshType {GHOST,HCOLLIDE};
#endif

#define CHECK(a) if (a == -1) return -1

#if 0
// c = a x b
static void vector_cross (const vrpn_float64 a [3], const vrpn_float64 b [3],
                          vrpn_float64 c [3]) {

  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];

}

// vprime = v * T
static void rotate_vector (const vrpn_float64 v [3], const vrpn_float64 T [9],
                           vrpn_float64 vprime [3]) {
  vprime[0] = v[0] * T[0] + v[1] * T[3] + v[2] * T[6];
  vprime[1] = v[0] * T[1] + v[1] * T[4] + v[2] * T[7];
  vprime[2] = v[0] * T[2] + v[1] * T[5] + v[2] * T[8];
}

static vrpn_float64 vector_dot (const vrpn_float64 a [3],
                                const vrpn_float64 b [3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
#endif

vrpn_ForceDevice::vrpn_ForceDevice(char *name, vrpn_Connection *c) :
vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();

    errorCode = FD_OK;

    //set the current time to zero
    timestamp.tv_sec = 0;
    timestamp.tv_usec = 0;

    //set the force to zero
    //d_force[0] = d_force[1] = d_force[2] = 0.0;
    SurfaceKspring= 0.29f;
    SurfaceFdynamic = 0.02f;
    SurfaceFstatic = 0.03f;
    SurfaceKdamping = 0.0f;

    SurfaceKadhesionNormal = 0.0001f;
    SurfaceKadhesionLateral = 0.0002f;
    SurfaceBuzzFreq = 0.0003f;
    SurfaceBuzzAmp = 0.0004f;
    SurfaceTextureWavelength = 0.01f;
    SurfaceTextureAmplitude = 0.0005f;
}

int vrpn_ForceDevice::register_types(void)
{
    force_message_id =
	d_connection->register_message_type("vrpn_ForceDevice Force");
    forcefield_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice Force_Field");
    plane_message_id =
	d_connection->register_message_type("vrpn_ForceDevice Plane");
    plane_effects_message_id =
	d_connection->register_message_type("vrpn_ForceDevice Plane2");
    setVertex_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice setVertex");
    setNormal_message_id =
	d_connection->register_message_type("vrpn_ForceDevice setNormal");
    setTriangle_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice setTriangle");
    removeTriangle_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice removeTriangle");
    updateTrimeshChanges_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice updateTrimeshChanges");
    transformTrimesh_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice transformTrimesh");
    setTrimeshType_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice setTrimeshType");
    clearTrimesh_message_id = 
	d_connection->register_message_type("vrpn_ForceDevice clearTrimesh");
    scp_message_id =
	d_connection->register_message_type("vrpn_ForceDevice SCP");
    error_message_id =
	d_connection->register_message_type("vrpn_ForceDevice Force_Error");

      enableConstraint_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_enable");
      setConstraintMode_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_mode");
      setConstraintPoint_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_point");
      setConstraintLinePoint_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_linept");
      setConstraintLineDirection_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_linedir");
      setConstraintPlanePoint_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_plpt");
      setConstraintPlaneNormal_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_plnorm");
      setConstraintKSpring_message_id =
            d_connection->register_message_type("vrpn_ForceDevice constraint_KSpring");

      return 0;
}

// virtual
vrpn_ForceDevice::~vrpn_ForceDevice (void) {

}

void vrpn_ForceDevice::print_plane(void)
{
  printf("plane: %f, %f, %f, %f\n", plane[0], plane[1], plane[2], plane[3]);
}

void vrpn_ForceDevice::print_report(void)
{
  // Nothing sets d_force any more!
  printf("Timestamp:%ld:%ld\n", timestamp.tv_sec, timestamp.tv_usec);
  //printf("Force    :%lf, %lf, %lf\n", d_force[0],d_force[1],d_force[2]);
}


// static
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
	    vrpn_buffer(&mptr, &mlen, force[i]);
    }

    return buf;
}

// static
vrpn_int32 vrpn_ForceDevice::decode_force (const char *buffer, const vrpn_int32 len, 
				vrpn_float64 *force)
{
    int i;
    const char *mptr = buffer;

    if (len !=  (3*sizeof(vrpn_float64)) ) {
      fprintf(stderr,"vrpn_ForceDevice: force message payload error\n");
      fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 3*sizeof(vrpn_float64) );
      return -1;
    }

    for (i = 0; i < 3; i++)
	    CHECK(vrpn_unbuffer(&mptr, &(force[i])));

    return 0;
}

// static
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
        vrpn_buffer(&mptr, &mlen, pos[i]);
    }
    for (i = 0; i < 4; i++) {
        vrpn_buffer(&mptr, &mlen, quat[i]);
    }

    return buf;
}

// static
vrpn_int32 vrpn_ForceDevice::decode_scp(const char *buffer, const vrpn_int32 len,
				 vrpn_float64 *pos, vrpn_float64 *quat)
{
    int i;
    const char *mptr = buffer;
    int  desiredLen = 7 * sizeof(vrpn_float64);

    if (len != desiredLen){
	    fprintf(stderr,"vrpn_ForceDevice: scp message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, desiredLen );
	    return -1;
    }

    for (i = 0; i < 3; i++)
	    CHECK(vrpn_unbuffer(&mptr, &(pos[i])));
    for (i = 0; i < 4; i++)
	    CHECK(vrpn_unbuffer(&mptr, &(quat[i])));

    return 0;
}

// static
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
		vrpn_buffer(&mptr, &mlen, plane[i]);
	}

	vrpn_buffer(&mptr, &mlen, kspring);
	vrpn_buffer(&mptr, &mlen, kdamp);
	vrpn_buffer(&mptr, &mlen, fdyn);
	vrpn_buffer(&mptr, &mlen, fstat);
	vrpn_buffer(&mptr, &mlen, plane_index);
	vrpn_buffer(&mptr, &mlen, n_rec_cycles);

	return buf;
}

// static
vrpn_int32 vrpn_ForceDevice::decode_plane(const char *buffer, const vrpn_int32 len, 
				vrpn_float32 *plane, 
				vrpn_float32 *kspring, vrpn_float32 *kdamp,
				vrpn_float32 *fdyn, vrpn_float32 *fstat, 
				vrpn_int32 *plane_index, vrpn_int32 *n_rec_cycles)
{
    int i;
    const char *mptr = buffer;

    if (len != 8*sizeof(vrpn_float32)+2*sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: plane message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 8*sizeof(vrpn_float32)+2*sizeof(vrpn_int32) );
	    return -1;
    }

    for (i = 0; i < 4; i++)
	    CHECK(vrpn_unbuffer(&mptr, &(plane[i])));
    CHECK(vrpn_unbuffer(&mptr, kspring));
    CHECK(vrpn_unbuffer(&mptr, kdamp));
    CHECK(vrpn_unbuffer(&mptr, fdyn));
    CHECK(vrpn_unbuffer(&mptr, fstat));
    CHECK(vrpn_unbuffer(&mptr, plane_index));
    CHECK(vrpn_unbuffer(&mptr, n_rec_cycles));

    return 0;
}

// static
char *vrpn_ForceDevice::encode_surface_effects(vrpn_int32 &len, 
		    const vrpn_float32 k_adhesion_normal,
			const vrpn_float32 k_adhesion_lateral,
		    const vrpn_float32 tex_amp, const vrpn_float32 tex_wl,
		    const vrpn_float32 buzz_amp, const vrpn_float32 buzz_freq) {

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = 6*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    vrpn_buffer(&mptr, &mlen, k_adhesion_normal);
	vrpn_buffer(&mptr, &mlen, k_adhesion_lateral);
    vrpn_buffer(&mptr, &mlen, tex_amp);
    vrpn_buffer(&mptr, &mlen, tex_wl);
    vrpn_buffer(&mptr, &mlen, buzz_amp);
    vrpn_buffer(&mptr, &mlen, buzz_freq);

    return buf;
}

// static
vrpn_int32 vrpn_ForceDevice::decode_surface_effects(const char *buffer, const vrpn_int32 len,
					vrpn_float32 *k_adhesion_normal,
					vrpn_float32 *k_adhesion_lateral,
					vrpn_float32 *tex_amp, vrpn_float32 *tex_wl,
					vrpn_float32 *buzz_amp, vrpn_float32 *buzz_freq) {

    const char *mptr = buffer;

    if (len != 6*sizeof(vrpn_float32)){
        fprintf(stderr,"vrpn_ForceDevice: surface effects message payload ");
        fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 6*sizeof(vrpn_float32) );
	return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, k_adhesion_normal));
    CHECK(vrpn_unbuffer(&mptr, k_adhesion_lateral));
    CHECK(vrpn_unbuffer(&mptr, tex_amp));
    CHECK(vrpn_unbuffer(&mptr, tex_wl));
    CHECK(vrpn_unbuffer(&mptr, buzz_amp));
    CHECK(vrpn_unbuffer(&mptr, buzz_freq));

    return 0;
}

// static
char *vrpn_ForceDevice::encode_vertex(vrpn_int32 &len,const vrpn_int32 vertNum,
			const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32) + 3*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    vrpn_buffer(&mptr, &mlen, vertNum);
    vrpn_buffer(&mptr, &mlen, x);
    vrpn_buffer(&mptr, &mlen, y);
    vrpn_buffer(&mptr, &mlen, z);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_vertex(const char *buffer, 
			    const vrpn_int32 len,vrpn_int32 *vertNum,
			    vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z){
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32) + 3*sizeof(vrpn_float32)){
	    fprintf(stderr,"vrpn_ForceDevice: vertex message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) + 3*sizeof(vrpn_float32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, vertNum));
    CHECK(vrpn_unbuffer(&mptr, x));
    CHECK(vrpn_unbuffer(&mptr, y));
    CHECK(vrpn_unbuffer(&mptr, z));

    return 0;
}

// static
char *vrpn_ForceDevice::encode_normal(vrpn_int32 &len,const vrpn_int32 normNum,
		       const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32) + 3*sizeof(vrpn_float32);
    mlen = len;

    buf = new char [len];
    mptr = buf;
    vrpn_buffer(&mptr, &mlen, normNum);
    vrpn_buffer(&mptr, &mlen, x);
    vrpn_buffer(&mptr, &mlen, y);
    vrpn_buffer(&mptr, &mlen, z);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_normal(const char *buffer,const vrpn_int32 len,
			     vrpn_int32 *vertNum,vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z){

    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32) + 3*sizeof(vrpn_float32)){
	    fprintf(stderr,"vrpn_ForceDevice: normal message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) + 3*sizeof(vrpn_float32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, vertNum));
    CHECK(vrpn_unbuffer(&mptr, x));
    CHECK(vrpn_unbuffer(&mptr, y));
    CHECK(vrpn_unbuffer(&mptr, z));

    return 0;
}

// static
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

    vrpn_buffer(&mptr, &mlen, triNum);
    vrpn_buffer(&mptr, &mlen, vert0);
    vrpn_buffer(&mptr, &mlen, vert1);
    vrpn_buffer(&mptr, &mlen, vert2);
    vrpn_buffer(&mptr, &mlen, norm0);
    vrpn_buffer(&mptr, &mlen, norm1);
    vrpn_buffer(&mptr, &mlen, norm2);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_triangle(const char *buffer,
				const vrpn_int32 len,vrpn_int32 *triNum,
			    vrpn_int32 *vert0,vrpn_int32 *vert1,vrpn_int32 *vert2,
			    vrpn_int32 *norm0,vrpn_int32 *norm1,vrpn_int32 *norm2)
{
    const char *mptr = buffer;

    if (len != 7*sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: triangle message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 7*sizeof(vrpn_int32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, triNum));
    CHECK(vrpn_unbuffer(&mptr, vert0));
    CHECK(vrpn_unbuffer(&mptr, vert1));
    CHECK(vrpn_unbuffer(&mptr, vert2));
    CHECK(vrpn_unbuffer(&mptr, norm0));
    CHECK(vrpn_unbuffer(&mptr, norm1));
    CHECK(vrpn_unbuffer(&mptr, norm2));

    return 0;
}

// static
char *vrpn_ForceDevice::encode_removeTriangle(vrpn_int32 &len,const vrpn_int32 triNum){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    vrpn_buffer(&mptr, &mlen, triNum);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_removeTriangle(const char *buffer,
				const vrpn_int32 len,vrpn_int32 *triNum){
    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	fprintf(stderr,"vrpn_ForceDevice: remove triangle message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, triNum));

    return 0;
}

// static
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

    vrpn_buffer(&mptr, &mlen, kspring);
    vrpn_buffer(&mptr, &mlen, kdamp);
    vrpn_buffer(&mptr, &mlen, fstat);
    vrpn_buffer(&mptr, &mlen, fdyn);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_updateTrimeshChanges(const char *buffer,
			const vrpn_int32 len, vrpn_float32 *kspring, vrpn_float32 *kdamp, 
			vrpn_float32 *fstat, vrpn_float32 *fdyn){

    const char *mptr = buffer;

    if (len != 4*sizeof(vrpn_float32)){
	fprintf(stderr,"vrpn_ForceDevice: update trimesh message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, 4*sizeof(vrpn_float32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, kspring));
    CHECK(vrpn_unbuffer(&mptr, kdamp));
    CHECK(vrpn_unbuffer(&mptr, fstat));
    CHECK(vrpn_unbuffer(&mptr, fdyn));

    return 0;
}

// static
char *vrpn_ForceDevice::encode_setTrimeshType(vrpn_int32 &len,const vrpn_int32 type){

    char *buf;
    char *mptr;
    vrpn_int32 mlen;

    len = sizeof(vrpn_int32);
    mlen = len;

    buf = new char [len];
    mptr = buf;

    vrpn_buffer(&mptr, &mlen, type);

    return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_setTrimeshType(const char *buffer,const vrpn_int32 len,
					   vrpn_int32 *type){

    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh type message payload");
	    fprintf(stderr," error\n             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, type));

    return 0;
}

// static
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
		vrpn_buffer(&mptr, &mlen, homMatrix[i]);    

	return buf; 
}

// static
vrpn_int32 vrpn_ForceDevice::decode_trimeshTransform(const char *buffer,
				  const vrpn_int32 len, vrpn_float32 homMatrix[16]){
    int i;
    const char *mptr = buffer;

    if (len != 16*sizeof(vrpn_float32)){
	fprintf(stderr,"vrpn_ForceDevice: trimesh transform message payload ");
	    fprintf(stderr,"error\n             (got %d, expected %d)\n",
		    len, 16*sizeof(vrpn_float32) );
	    return -1;
    }

    for (i = 0; i < 16; i++)
	    CHECK(vrpn_unbuffer(&mptr, &(homMatrix[i])));

    return 0;
}

// static
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
    vrpn_buffer(&mptr, &mlen, origin[i]);

    for (i=0;i<3;i++)
    vrpn_buffer(&mptr, &mlen, force[i]);

    for (i=0;i<3;i++)
	    for (j=0;j<3;j++)
		    vrpn_buffer(&mptr, &mlen, jacobian[i][j]);

    vrpn_buffer(&mptr, &mlen, radius);

    return buf;
}

// static
vrpn_int32 vrpn_ForceDevice::decode_forcefield(const char *buffer,
			  const vrpn_int32 len,vrpn_float32 origin[3],
			  vrpn_float32 force[3], vrpn_float32 jacobian[3][3], vrpn_float32 *radius){
    int i,j;
    const char *mptr = buffer;

    if (len != 16*sizeof(vrpn_float32)){
       fprintf(stderr,"vrpn_ForceDevice: force field message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, 16*sizeof(vrpn_float32) );
	    return -1;
    }

    for (i=0;i<3;i++)
	    CHECK(vrpn_unbuffer(&mptr, &(origin[i])));

    for (i=0;i<3;i++)
	    CHECK(vrpn_unbuffer(&mptr, &(force[i])));

    for (i=0;i<3;i++)
	    for (j=0;j<3;j++)
		    CHECK(vrpn_unbuffer(&mptr, &(jacobian[i][j])));

    CHECK(vrpn_unbuffer(&mptr, radius));

    return 0;
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

    vrpn_buffer(&mptr, &mlen, error_code);

    return buf;
}

vrpn_int32 vrpn_ForceDevice::decode_error(const char *buffer,
				   const vrpn_int32 len,vrpn_int32 *error_code)
{

    const char *mptr = buffer;

    if (len != sizeof(vrpn_int32)){
	    fprintf(stderr,"vrpn_ForceDevice: error message payload error\n");
	    fprintf(stderr,"             (got %d, expected %d)\n",
		    len, sizeof(vrpn_int32) );
	    return -1;
    }

    CHECK(vrpn_unbuffer(&mptr, error_code));

    return 0;
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

void vrpn_ForceDevice::sendError(int error_code) {
    char	*msgbuf;
    vrpn_int32	len;
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    if(d_connection) {
	msgbuf = encode_error(len, error_code);
	if(d_connection->pack_message(len,timestamp,error_message_id,
				d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	  fprintf(stderr,"Phantom: cannot write message: tossing\n");
	}
	delete [] msgbuf;
    }
}

// constraint message encode/decode methods


//static
char * vrpn_ForceDevice::encode_enableConstraint
                   (vrpn_int32 & len,
                    vrpn_int32 enable) {
  char * buf;
  char * mptr;
  vrpn_int32 mlen;

  len = sizeof(vrpn_int32);
  mlen = len;

  buf = new char [len];
  mptr = buf;

  vrpn_buffer(&mptr, &mlen, enable);

  return buf;
}

//static
vrpn_int32 vrpn_ForceDevice::decode_enableConstraint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_int32 * enable) {
  const char * mptr = buffer;

  if (len != sizeof(vrpn_int32)) {
    fprintf(stderr,"vrpn_ForceDevice:  "
                   "enable constraint message payload error\n"
                   "             (got %d, expected %d)\n",
            len, sizeof(vrpn_int32));
    return -1;
  }
  
  CHECK(vrpn_unbuffer(&mptr, enable));

  return 0;
}


//static
char * vrpn_ForceDevice::encode_setConstraintMode
                   (vrpn_int32 & len,
                    ConstraintGeometry mode) {
  char * buf;
  char * mptr;
  vrpn_int32 modeint;
  vrpn_int32 mlen;

  len = sizeof(vrpn_int32);
  mlen = len;

  buf = new char [len];
  mptr = buf;

  switch (mode) {
    case NO_CONSTRAINT:  modeint = 0;  break;
    case POINT_CONSTRAINT:  modeint = 1;  break;
    case LINE_CONSTRAINT:  modeint = 2;  break;
    case PLANE_CONSTRAINT:  modeint = 3;  break;
    default:
      fprintf(stderr, "vrpn_ForceDevice:  "
                      "Unknown or illegal constraint mode.\n");
      modeint = 0;  break;
  }

  vrpn_buffer(&mptr, &mlen, modeint);

  return buf;
}

//static
vrpn_int32 vrpn_ForceDevice::decode_setConstraintMode
                   (const char * buffer,
                    const vrpn_int32 len,
                    ConstraintGeometry * mode) {
  const char * mptr = buffer;
  vrpn_int32 modeint;

  if (len != sizeof(vrpn_int32)) {
    fprintf(stderr,"vrpn_ForceDevice:  "
                   "constraint mode payload error\n"
                   "             (got %d, expected %d)\n",
            len, sizeof(vrpn_int32));
    return -1;
  }
  
  CHECK(vrpn_unbuffer(&mptr, &modeint));

  switch (modeint) {
    case 0:  *mode = NO_CONSTRAINT;  break;
    case 1:  *mode = POINT_CONSTRAINT;  break;
    case 2:  *mode = LINE_CONSTRAINT;  break;
    case 3:  *mode = PLANE_CONSTRAINT;  break;
    default:
      fprintf(stderr, "vrpn_ForceDevice:  "
                      "Unknown or illegal constraint mode.\n");
      *mode = NO_CONSTRAINT;
      return -1;
  }

  return 0;
}


//static
char * vrpn_ForceDevice::encode_setConstraintPoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  return encodePoint(len, x, y, z);
}

//static
vrpn_int32 vrpn_ForceDevice::decode_setConstraintPoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z) {
  return decodePoint(buffer, len, x, y, z);
}


//static
char * vrpn_ForceDevice::encode_setConstraintLinePoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  return encodePoint(len, x, y, z);
}

//static
vrpn_int32 vrpn_ForceDevice::decode_setConstraintLinePoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z) {
  return decodePoint(buffer, len, x, y, z);
}


//static
char * vrpn_ForceDevice::encode_setConstraintLineDirection
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  return encodePoint(len, x, y, z);
}

//static
vrpn_int32 vrpn_ForceDevice::decode_setConstraintLineDirection
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z) {
  return decodePoint(buffer, len, x, y, z);
}

//static
char * vrpn_ForceDevice::encode_setConstraintPlanePoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  return encodePoint(len, x, y, z);
}

//static
 vrpn_int32 vrpn_ForceDevice::decode_setConstraintPlanePoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z) {
  return decodePoint(buffer, len, x, y, z);
}

//static
char * vrpn_ForceDevice::encode_setConstraintPlaneNormal
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  return encodePoint(len, x, y, z);
}

//static
 vrpn_int32 vrpn_ForceDevice::decode_setConstraintPlaneNormal
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z) {
  return decodePoint(buffer, len, x, y, z);
}

//static
char * vrpn_ForceDevice::encode_setConstraintKSpring
                   (vrpn_int32 & len,
                    vrpn_float32 k) {
  char * buf;
  char * mptr;
  vrpn_int32 mlen;

  len = sizeof(vrpn_float32);
  mlen = len;

  buf = new char [len];
  mptr = buf;

  vrpn_buffer(&mptr, &mlen, k);

  return buf;
}

//static
vrpn_int32 vrpn_ForceDevice::decode_setConstraintKSpring
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * k) {
  const char * mptr = buffer;

  if (len != sizeof(vrpn_float32)) {
    fprintf(stderr,"vrpn_ForceDevice:  "
                   "set constraint spring message payload error\n"
                   "             (got %d, expected %d)\n",
            len, sizeof(vrpn_float32));
    return -1;
  }

  CHECK(vrpn_unbuffer(&mptr, k));

  return 0;
}


//
//  utility functions
//

//static
char * vrpn_ForceDevice::encodePoint (vrpn_int32 & len,
                vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
  char * buf;
  char * mptr;
  vrpn_int32 mlen;

  len = 3 * sizeof(vrpn_float32);
  mlen = len;

  buf = new char [len];
  mptr = buf;

  vrpn_buffer(&mptr, &mlen, x);
  vrpn_buffer(&mptr, &mlen, y);
  vrpn_buffer(&mptr, &mlen, z);

  return buf;
}

//static
vrpn_int32 vrpn_ForceDevice::decodePoint (const char * buffer,
                const vrpn_int32 len, vrpn_float32 * x, vrpn_float32 * y,
                vrpn_float32 * z) {
  const char * mptr = buffer;

  if (len != 3 * sizeof(vrpn_float32)) {
    fprintf(stderr,"vrpn_ForceDevice:  "
                   "decode point message payload error\n"
                   "             (got size %d, expected %d)\n",
            len, 3 * sizeof(vrpn_float32));
    return -1;
  }
  
  CHECK(vrpn_unbuffer(&mptr, x));
  CHECK(vrpn_unbuffer(&mptr, y));
  CHECK(vrpn_unbuffer(&mptr, z));

  return 0;
}


/* ******************** vrpn_ForceDevice_Remote ********************** */

vrpn_ForceDevice_Remote::vrpn_ForceDevice_Remote(char *name, vrpn_Connection *cn):
	vrpn_ForceDevice(name, cn),
	change_list (NULL),
	scp_change_list (NULL),
	error_change_list (NULL),
	d_conEnabled (0),
	d_conMode (POINT_CONSTRAINT)
{
    which_plane = 0;

    // Make sure that we have a valid connection
    if (d_connection == NULL) {
		fprintf(stderr,"vrpn_ForceDevice_Remote: No connection\n");
		return;
     }

     // Register a handler for the change callback from this device.
     if (register_autodeleted_handler(force_message_id, 
			handle_force_change_message,this, d_sender_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    d_connection = NULL;
     }

     // Register a handler for the scp change callback from this device.
     if (register_autodeleted_handler(scp_message_id,
	handle_scp_change_message, this, d_sender_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    d_connection = NULL;
     }

     // Register a handler for the error change callback from this device.
     if (register_autodeleted_handler(error_message_id, 
			handle_error_change_message, this, d_sender_id)) {
	    fprintf(stderr,"vrpn_ForceDevice_Remote:can't register handler\n");
	    d_connection = NULL;
     }

     // Find out what time it is and put this into the timestamp
     gettimeofday(&timestamp, NULL);
}

// virtual
vrpn_ForceDevice_Remote::~vrpn_ForceDevice_Remote (void)
{
	vrpn_FORCECHANGELIST	*nextforce;
	vrpn_FORCESCPCHANGELIST	*nextscp;
	vrpn_FORCEERRORCHANGELIST *nexterror;

	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the lists
	while (change_list != NULL) {
		nextforce = change_list->next;
		delete change_list;
		change_list = nextforce;
	}
	while (scp_change_list != NULL) {
		nextscp = scp_change_list->next;
		delete scp_change_list;
		scp_change_list = nextscp;
	}
	while (error_change_list != NULL) {
		nexterror = error_change_list->next;
		delete error_change_list;
		error_change_list = nexterror;
	}
}

void vrpn_ForceDevice_Remote::sendSurface(void)
{ // Encode the plane if there is a connection
  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;
  
  if(d_connection) {
      msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
      if(d_connection->pack_message(len,timestamp,plane_message_id,
		d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      delete [] msgbuf;

      msgbuf = encode_surface_effects(len, SurfaceKadhesionNormal,
		SurfaceKadhesionLateral, SurfaceTextureAmplitude,
		SurfaceTextureWavelength, SurfaceBuzzAmp, 
		SurfaceBuzzFreq);
      if(d_connection->pack_message(len,timestamp,plane_effects_message_id,
		d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
		fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      delete [] msgbuf;
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
    
    if(d_connection){
      msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
      if (d_connection->pack_message(len,timestamp,plane_message_id,
			   d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      delete [] msgbuf;
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
  
    if(d_connection) {
      msgbuf = encode_plane(len, plane, SurfaceKspring, SurfaceKdamping,
		SurfaceFdynamic, SurfaceFstatic, which_plane, numRecCycles);
      if (d_connection->pack_message(len,timestamp,plane_message_id,
				   d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
      }
      delete [] msgbuf;
    }
}

void vrpn_ForceDevice_Remote::setVertex(vrpn_int32 vertNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z){

  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_vertex(len,vertNum,x,y,z);
    if (d_connection->pack_message(len,timestamp,setVertex_message_id,
		 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}

void vrpn_ForceDevice_Remote::setNormal(vrpn_int32 normNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z){
  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_normal(len,normNum,x,y,z);
    if (d_connection->pack_message(len,timestamp,setNormal_message_id,
			 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
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

  if(d_connection){
    msgbuf = encode_triangle(len,triNum,vert0,vert1,vert2,norm0,norm1,norm2);
    if (d_connection->pack_message(len,timestamp,setTriangle_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}

void vrpn_ForceDevice_Remote::removeTriangle(vrpn_int32 triNum){

  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_removeTriangle(len,triNum);
    if (d_connection->pack_message(len,timestamp,removeTriangle_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}

void vrpn_ForceDevice_Remote::updateTrimeshChanges(){

  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_updateTrimeshChanges(len, SurfaceKspring, SurfaceKdamping,
			SurfaceFstatic, SurfaceFdynamic);
    if (d_connection->pack_message(len,timestamp,updateTrimeshChanges_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
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

  if(d_connection){
    msgbuf = encode_trimeshTransform(len,homMatrix);
    if (d_connection->pack_message(len,timestamp,transformTrimesh_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
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

  if(d_connection){
    if (d_connection->pack_message(len,timestamp,clearTrimesh_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    //delete [] msgbuf;
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

  if(d_connection){
    msgbuf = encode_setTrimeshType(len,HCOLLIDE);
    if (d_connection->pack_message(len,timestamp,setTrimeshType_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}

void vrpn_ForceDevice_Remote::useGhost(void){
  char	*msgbuf;
  vrpn_int32		len;
  struct timeval current_time;
  
  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_setTrimeshType(len,GHOST);
    if (d_connection->pack_message(len,timestamp,setTrimeshType_message_id,
				 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}


//
// constraint methods
//


#ifndef FD_SPRINGS_AS_FIELDS

#if 0
void vrpn_ForceDevice_Remote::enableConstraint (vrpn_int32 enable) {
}

void vrpn_ForceDevice_Remote::setConstraintMode (ConstraintGeometry mode) {
}

void vrpn_ForceDevice_Remote::setConstraintPoint
}

void vrpn_ForceDevice_Remote::setConstraintLinePoint
}

void vrpn_ForceDevice_Remote::setConstraintLineDirection
}

void vrpn_ForceDevice_Remote::setConstraintPlanePoint
}

void vrpn_ForceDevice_Remote::setConstraintPlaneNormal
}

void vrpn_ForceDevice_Remote::setConstraintKSpring (vrpn_float32 k) {
}
#endif  // 0

#else

void vrpn_ForceDevice_Remote::enableConstraint (vrpn_int32 enable) {
  if (enable == d_conEnabled) return;
  d_conEnabled = enable;

  switch (d_conEnabled) {
    case 0:
      stopForceField();
      break;
    case 1:
      constraintToForceField();
      sendForceField();
      break;
    default:
      fprintf(stderr, "vrpn_ForceDevice_Remote::enableConstraint:  "
                      "Illegal value of enable (%d).\n", enable);
      break;
  }
}

void vrpn_ForceDevice_Remote::setConstraintMode (ConstraintGeometry mode) {
  d_conMode = mode;
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintPoint
                  (vrpn_float32 point [3]) {
  d_conPoint[0] = point[0];
  d_conPoint[1] = point[1];
  d_conPoint[2] = point[2];
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintLinePoint
                  (vrpn_float32 point [3]) {
  d_conLinePoint[0] = point[0];
  d_conLinePoint[1] = point[1];
  d_conLinePoint[2] = point[2];
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintLineDirection
                  (vrpn_float32 direction [3]) {
  d_conLineDirection[0] = direction[0];
  d_conLineDirection[1] = direction[1];
  d_conLineDirection[2] = direction[2];
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintPlanePoint
                  (vrpn_float32 point [3]) {
  d_conPlanePoint[0] = point[0];
  d_conPlanePoint[1] = point[1];
  d_conPlanePoint[2] = point[2];
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintPlaneNormal
                  (vrpn_float32 normal [3]) {
  d_conPlaneNormal[0] = normal[0];
  d_conPlaneNormal[1] = normal[1];
  d_conPlaneNormal[2] = normal[2];
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

void vrpn_ForceDevice_Remote::setConstraintKSpring (vrpn_float32 k) {
  d_conKSpring = k;
  constraintToForceField();
  if (d_conEnabled) { sendForceField(); }
}

#endif  // FD_SPRINGS_AS_FIELDS

void vrpn_ForceDevice_Remote::sendForceField (void)
{
    sendForceField(ff_origin, ff_force, ff_jacobian, ff_radius);
}

void vrpn_ForceDevice_Remote::sendForceField
              (vrpn_float32 origin [3], vrpn_float32 force [3],
               vrpn_float32 jacobian [3][3], vrpn_float32 radius)
{
  char *msgbuf;
  vrpn_int32   len;
  struct timeval current_time;

  gettimeofday(&current_time, NULL);
  timestamp.tv_sec = current_time.tv_sec;
  timestamp.tv_usec = current_time.tv_usec;

  if(d_connection){
    msgbuf = encode_forcefield(len, origin, force, jacobian, radius);
    if (d_connection->pack_message(len,timestamp, forcefield_message_id,
		d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}

// same as sendForceField but sets force to 0 and sends RELIABLY
void vrpn_ForceDevice_Remote::stopForceField (void)
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

  if(d_connection){
    msgbuf = encode_forcefield(len, origin, force, jacobian, radius);
    if (d_connection->pack_message(len,timestamp, forcefield_message_id,
                d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
	fprintf(stderr,"Phantom: cannot write message: tossing\n");
    }
    delete [] msgbuf;
  }
}


void	vrpn_ForceDevice_Remote::mainloop()
{
	if (d_connection) { d_connection->mainloop(); }
	client_mainloop();
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

void vrpn_ForceDevice_Remote::send (const char * msgbuf, vrpn_int32 len,
                      vrpn_int32 type) {
  struct timeval now;

  gettimeofday(&now, NULL);
  timestamp.tv_sec = now.tv_sec;
  timestamp.tv_usec = now.tv_usec;

  if (d_connection) {
    if (d_connection->pack_message(len, now, type,
                                 d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr, "vrpn_ForceDevice_Remote::send:  Can't pack message.\n");
    }
  }

  // HP compiler won't let you delete a const argument.
  delete [] (char *) msgbuf;
}


#ifdef FD_SPRINGS_AS_FIELDS


void vrpn_ForceDevice_Remote::constraintToForceField (void) {

  vrpn_float32 c [9];  // scratch matrix
  vrpn_float64 s [3];  // scratch vectors
  int i, j;

  // quatlib wants 64-bit reals;  the forcefield code wants 32-bit
  // reals.  We do most of our math in 64 bits, then copy into 32
  // for output to force field.

  const float largeRadius = 100.0f;  // infinity

  switch (d_conMode) {

    case NO_CONSTRAINT:
	break;

    case POINT_CONSTRAINT:
      setFF_Origin(d_conPoint);
      setFF_Force(0.0f, 0.0f,  0.0f);
      setFF_Jacobian(-d_conKSpring,  0.0f,  0.0f,
                      0.0f, -d_conKSpring,  0.0f,
                      0.0f,  0.0f, -d_conKSpring);
      setFF_Radius(largeRadius);
      break;

    case LINE_CONSTRAINT:
      setFF_Origin(d_conLinePoint);
      setFF_Force(0.0f, 0.0f,  0.0f);

      // Paul Grayson (pdg@mit.edu) points out that rotating the Jacobian
      // won't work and we should use something simpler here.
      // Unfortunately, it's because of the apparent difficulty of this
      // case that we'd tried rotating a Jacobian in the first place.
      // Time to hit the books again looking for something that'll work.

      // The direction we want (modulo instability in computation)
      // is (userPosition - d_conLinePoint) -
      //        d_conLineDirection x
      //          (userPosition - d_conLinePoint) * d_conLineDirection;
      // for force, multiply that by -d_conKSpring.

      // So we fudge the partial derivatives:
      //   dFx/dx = dx (1 - d_conLineDirection[0]^2)
      //   dFx/dy = dy ( - d_conLineDirection[0]^2)
      //   dFx/dz = dz ( - d_conLineDirection[0]^2)

      s[0] = d_conLineDirection[0] * d_conLineDirection[0];
      s[1] = d_conLineDirection[1] * d_conLineDirection[1];
      s[2] = d_conLineDirection[2] * d_conLineDirection[2];

      c[0] = (vrpn_float32)(1.0 - s[0]);
      c[1] = (vrpn_float32)(- s[0]);
      c[2] = (vrpn_float32)(- s[0]);
      c[3] = (vrpn_float32)(- s[1]);
      c[4] = (vrpn_float32)(1.0 - s[1]);
      c[5] = (vrpn_float32)(- s[1]);
      c[6] = (vrpn_float32)(- s[2]);
      c[7] = (vrpn_float32)(- s[2]);
      c[8] = (vrpn_float32)(1.0 - s[2]);

      setFF_Jacobian(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8]);

      setFF_Radius(largeRadius);
      break;

    case PLANE_CONSTRAINT:
      setFF_Origin(d_conPlanePoint);
      setFF_Force(0.0f, 0.0f,  0.0f);

      // Paul Grayson (pdg@mit.edu) points out that rotating the Jacobian
      // won't work and we should use something simpler here.  The below
      // is his code.

      for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
          c[i + j * 3] = (vrpn_float32)(-d_conKSpring * d_conPlaneNormal[i]
                                       * d_conPlaneNormal[j]);

      setFF_Jacobian(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8]);

      setFF_Radius(largeRadius);
      break;
  }
}


#endif  // FD_SPRINGS_AS_FIELDS

