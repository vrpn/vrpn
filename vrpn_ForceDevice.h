
#ifndef FORCEDEVICE_H
#define  FORCEDEVICE_H

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

#define MAXPLANE 4   //maximum number of planes in the scene 

// for recovery:
#define DEFAULT_NUM_REC_CYCLES	(10)

// possible values for errorCode:
#define FD_VALUE_OUT_OF_RANGE 0	// surface parameter out of range
#define FD_DUTY_CYCLE_ERROR 1	// servo loop is taking too long
#define FD_FORCE_ERROR 2	// max force exceeded, or motors overheated
				// or amplifiers not enabled
#define FD_MISC_ERROR 3		// everything else
#define FD_OK 4	// no error

class vrpn_ForceDevice {
public:
    vrpn_ForceDevice(char * name, vrpn_Connection *c);
    virtual void mainloop(const struct timeval * timeout=NULL) = 0;
    void print_report(void);
    void print_plane(void);

    void setSurfaceKspring(vrpn_float32 k) { 
				    SurfaceKspring = k; }
    void setSurfaceKdamping(vrpn_float32 d) {SurfaceKdamping =d;}
    void setSurfaceFstatic(vrpn_float32 ks) {SurfaceFstatic = ks;}
    void setSurfaceFdynamic(vrpn_float32 kd) {SurfaceFdynamic =kd;}
    void setRecoveryTime(int rt) {numRecCycles = rt;}

	// additional surface properties
	void setSurfaceKadhesionNormal(vrpn_float32 k) {SurfaceKadhesionNormal = k;}
	void setSurfaceKadhesionLateral(vrpn_float32 k) 
		{SurfaceKadhesionLateral = k;}
	void setSurfaceBuzzFrequency(vrpn_float32 freq) {SurfaceBuzzFreq = freq;}
	void setSurfaceBuzzAmplitude(vrpn_float32 amp) {SurfaceBuzzAmp = amp;}
	void setSurfaceTextureWavelength(vrpn_float32 wl) 
		{SurfaceTextureWavelength = wl;}
	void setSurfaceTextureAmplitude(vrpn_float32 amp) 
		{SurfaceTextureAmplitude = amp;}

    void setFF_Origin(vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
	    ff_origin[0] = x;ff_origin[1] = y; ff_origin[2] = z;}
    void setFF_Force(vrpn_float32 fx, vrpn_float32 fy, vrpn_float32 fz) {
	    ff_force[0] = fx; ff_force[1] = fy; ff_force[2] = fz;}
    void setFF_Jacobian(vrpn_float32 dfxdx, vrpn_float32 dfxdy, vrpn_float32 dfxdz,
			vrpn_float32 dfydx, vrpn_float32 dfydy, vrpn_float32 dfydz,
			vrpn_float32 dfzdx, vrpn_float32 dfzdy, vrpn_float32 dfzdz){
	    ff_jacobian[0][0] = dfxdx; ff_jacobian[0][1] = dfxdy;
	    ff_jacobian[0][2] = dfxdz; ff_jacobian[1][0] = dfydx;
	    ff_jacobian[1][1] = dfydy; ff_jacobian[1][2] = dfydz;
	    ff_jacobian[2][0] = dfzdx; ff_jacobian[2][1] = dfzdy;
	    ff_jacobian[2][2] = dfzdz;}
    void setFF_Radius(vrpn_float32 r) { ff_radius = r;};
    void set_plane(vrpn_float32 *p);
    void set_plane(vrpn_float32 *p, vrpn_float32 d);
    void set_plane(vrpn_float32 a, vrpn_float32 b, vrpn_float32 c,vrpn_float32 d);
    void sendError(int error_code);

    int getRecoveryTime(void) {return numRecCycles;}
    int connectionAvailable(void) {return (connection != NULL);}

protected:
    vrpn_Connection *connection;		// Used to send messages

    vrpn_int32 my_id;			// ID of this force device to connection

    vrpn_int32 force_message_id;	// ID of force message to connection
    vrpn_int32 plane_message_id;	//ID of plane equation message
	vrpn_int32 plane_effects_message_id; // additional plane properties
    vrpn_int32 set_constraint_message_id;// ID of constraint force message
    vrpn_int32 forcefield_message_id; 	// ID of force field message
    vrpn_int32 scp_message_id;		// ID of surface contact point message

    // XXX - error messages should be put into the vrpn base class 
    // whenever someone makes one
    vrpn_int32 error_message_id;	// ID of force device error message

    // IDs for trimesh messages
    vrpn_int32 setVertex_message_id;   
    vrpn_int32 setNormal_message_id;   
    vrpn_int32 setTriangle_message_id;   
    vrpn_int32 removeTriangle_message_id;   
    vrpn_int32 updateTrimeshChanges_message_id;   
    vrpn_int32 transformTrimesh_message_id;    
    vrpn_int32 setTrimeshType_message_id;    
    vrpn_int32 clearTrimesh_message_id;    

    //	virtual void get_report(void) = 0;

    // ENCODING
    static char *encode_force(vrpn_int32 &length, const vrpn_float64 *force);
    static char *encode_scp(vrpn_int32 &length,
	    const vrpn_float64 *pos, const vrpn_float64 *quat);
    static char *encode_plane(vrpn_int32 &length,const vrpn_float32 *plane, 
			    const vrpn_float32 kspring, const vrpn_float32 kdamp,
			    const vrpn_float32 fdyn, const vrpn_float32 fstat, 
			    const vrpn_int32 plane_index, const vrpn_int32 n_rec_cycles);
    static char *encode_surface_effects(vrpn_int32 &len, 
			const vrpn_float32 k_adhesion_norm, 
			const vrpn_float32 k_adhesion_lat,
		    const vrpn_float32 tex_amp, const vrpn_float32 tex_wl,
		    const vrpn_float32 buzz_amp, const vrpn_float32 buzz_freq);
    static char *encode_vertex(vrpn_int32 &len, const vrpn_int32 vertNum,
		const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z); 
    static char *encode_normal(vrpn_int32 &len,const vrpn_int32 vertNum,
		const vrpn_float32 x,const vrpn_float32 y,const vrpn_float32 z); 
    static char *encode_triangle(vrpn_int32 &len,const vrpn_int32 triNum,
	      const vrpn_int32 vert0,const vrpn_int32 vert1,const vrpn_int32 vert2,
	      const vrpn_int32 norm0,const vrpn_int32 norm1,const vrpn_int32 norm2);	       
    static char *encode_removeTriangle(vrpn_int32 &len,const vrpn_int32 triNum);
    static char *encode_updateTrimeshChanges(vrpn_int32 &len,
		const vrpn_float32 kspring, const vrpn_float32 kdamp,
		const vrpn_float32 fdyn, const vrpn_float32 fstat);
    static char *encode_setTrimeshType(vrpn_int32 &len,const vrpn_int32 type);
    static char *encode_trimeshTransform(vrpn_int32 &len,
		const vrpn_float32 homMatrix[16]);

    static char *encode_constraint(vrpn_int32 &len, const vrpn_int32 enable, 
	const vrpn_float32 x, const vrpn_float32 y, const vrpn_float32 z, const vrpn_float32 kSpr);
    static char *encode_forcefield(vrpn_int32 &len, const vrpn_float32 origin[3],
	const vrpn_float32 force[3], const vrpn_float32 jacobian[3][3], const vrpn_float32 radius);
    static char *encode_error(vrpn_int32 &len, const vrpn_int32 error_code);


    // DECODING
    static vrpn_int32 decode_force (const char *buffer, const vrpn_int32 len, 
							vrpn_float64 *force);
    static vrpn_int32 decode_scp(const char *buffer, const vrpn_int32 len,
					vrpn_float64 *pos, vrpn_float64 *quat);
    static vrpn_int32 decode_plane(const char *buffer, const vrpn_int32 len,
	    vrpn_float32 *plane, 
	    vrpn_float32 *kspring, vrpn_float32 *kdamp,vrpn_float32 *fdyn, vrpn_float32 *fstat, 
	    vrpn_int32 *plane_index, vrpn_int32 *n_rec_cycles);
    static vrpn_int32 decode_surface_effects(const char *buffer,
	    const vrpn_int32 len, vrpn_float32 *k_adhesion_norm,
	    vrpn_float32 *k_adhesion_lat,
		vrpn_float32 *tex_amp, vrpn_float32 *tex_wl,
	    vrpn_float32 *buzz_amp, vrpn_float32 *buzz_freq);
    static vrpn_int32 decode_vertex(const char *buffer, const vrpn_int32 len,
		vrpn_int32 *vertNum,
		vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z); 
    static vrpn_int32 decode_normal(const char *buffer,const vrpn_int32 len,
		vrpn_int32 *vertNum,vrpn_float32 *x,vrpn_float32 *y,vrpn_float32 *z); 
    static vrpn_int32 decode_triangle(const char *buffer,const vrpn_int32 len,vrpn_int32 *triNum,
		vrpn_int32 *vert0,vrpn_int32 *vert1,vrpn_int32 *vert2,
		vrpn_int32 *norm0,vrpn_int32 *norm1,vrpn_int32 *norm2);	       
    static vrpn_int32 decode_removeTriangle(const char *buffer,const vrpn_int32 len,
						vrpn_int32 *triNum);
    static vrpn_int32 decode_updateTrimeshChanges(const char *buffer,const vrpn_int32 len,
		vrpn_float32 *kspring, vrpn_float32 *kdamp, vrpn_float32 *fdyn, vrpn_float32 *fstat);
    static vrpn_int32 decode_setTrimeshType(const char *buffer,const vrpn_int32 len,
						vrpn_int32 *type);
    static vrpn_int32 decode_trimeshTransform(const char *buffer,const vrpn_int32 len,
						vrpn_float32 homMatrix[16]);

    static vrpn_int32 decode_constraint(const char *buffer,const vrpn_int32 len, 
		vrpn_int32 *enable, vrpn_float32 *x, vrpn_float32 *y, vrpn_float32 *z, vrpn_float32 *kSpr);
    static vrpn_int32 decode_forcefield(const char *buffer,const vrpn_int32 len,
	vrpn_float32 origin[3], vrpn_float32 force[3], vrpn_float32 jacobian[3][3], vrpn_float32 *radius);
    static vrpn_int32 decode_error(const char *buffer, const vrpn_int32 len, 
		vrpn_int32 *error_code);

    struct timeval timestamp;

    vrpn_int32   which_plane;
    vrpn_float64 force[3];
    vrpn_float64 scp_pos[3];
    vrpn_float64 scp_quat[4];  // for torque
    vrpn_float32 plane[4];

    vrpn_float32 ff_origin[3];
    vrpn_float32 ff_force[3];
    vrpn_float32 ff_jacobian[3][3]; // J[i][j] = dF[i]/dx[j]
    vrpn_float32 ff_radius;

    vrpn_float32 SurfaceKspring;
    vrpn_float32 SurfaceKdamping;
    vrpn_float32 SurfaceFstatic;
    vrpn_float32 SurfaceFdynamic;
    vrpn_int32 numRecCycles;
    vrpn_int32 errorCode;

	vrpn_float32 SurfaceKadhesionLateral;
	vrpn_float32 SurfaceKadhesionNormal;
	vrpn_float32 SurfaceBuzzFreq;
	vrpn_float32 SurfaceBuzzAmp;
	vrpn_float32 SurfaceTextureWavelength;
	vrpn_float32 SurfaceTextureAmplitude;

};

// User routine to handle position reports for surface contact point (SCP)
// This is in vrpn_ForceDevice rather than vrpn_Tracker because only
// a force feedback device should know anything about SCPs as this is a
// part of the force feedback model. It may be preferable to use the SCP
// rather than the tracker position for graphics so the hand position
// doesn't appear to go below the surface making the surface look very
// compliant. 
typedef struct {
	struct		timeval msg_time;	// Time of the report
	vrpn_float64	pos[3];			// position of SCP
	vrpn_float64	quat[4];		// orientation of SCP
} vrpn_FORCESCPCB;
typedef void (*vrpn_FORCESCPHANDLER) (void *userdata,
					const vrpn_FORCESCPCB info);

typedef	struct {
	struct		timeval	msg_time;	// Time of the report
	vrpn_float64	force[3];		// force value
} vrpn_FORCECB;
typedef void (*vrpn_FORCECHANGEHANDLER)(void *userdata,
					 const vrpn_FORCECB info);

typedef struct {
	struct		timeval msg_time;	// time of the report
	vrpn_int32		error_code;		// type of error
} vrpn_FORCEERRORCB;
typedef void (*vrpn_FORCEERRORHANDLER) (void *userdata,
					const vrpn_FORCEERRORCB info);

class vrpn_ForceDevice_Remote: public vrpn_ForceDevice {
public:

    // The name of the force device to connect to
    vrpn_ForceDevice_Remote(char *name);

    void sendSurface(void);
    void startSurface(void);
    void stopSurface(void);

    // vertNum normNum and triNum start at 0
    void setVertex(vrpn_int32 vertNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z);
    // NOTE: ghost dosen't take normals, 
    //       and normals still aren't implemented for Hcollide
    void setNormal(vrpn_int32 normNum,vrpn_float32 x,vrpn_float32 y,vrpn_float32 z);
    void setTriangle(vrpn_int32 triNum,vrpn_int32 vert0,vrpn_int32 vert1,vrpn_int32 vert2,
		  vrpn_int32 norm0=-1,vrpn_int32 norm1=-1,vrpn_int32 norm2=-1);
    void removeTriangle(vrpn_int32 triNum); 
    // should be called to incorporate the above changes into the 
    // displayed trimesh 
    void updateTrimeshChanges();
    // set the trimesh's homogen transform matrix (in row major order)
    void setTrimeshTransform(vrpn_float32 homMatrix[16]);
  	void clearTrimesh(void);
  
    // the next time we send a trimesh we will use the following type
    void useHcollide();
    void useGhost();

    void sendConstraint(vrpn_int32 enable, vrpn_float32 x, vrpn_float32 y, vrpn_float32 z, vrpn_float32 kSpr);

    void sendForceField(vrpn_float32 origin[3], vrpn_float32 force[3],
	    vrpn_float32 jacobian[3][3], vrpn_float32 radius);
    void sendForceField(void);
    void stopForceField();

    // This routine calls the mainloop of the connection it's own
    virtual void mainloop(const struct timeval * timeout=NULL);

    // (un)Register a callback handler to handle a force change
    // and plane equation change and trimesh change
    virtual int register_force_change_handler(void *userdata,
	    vrpn_FORCECHANGEHANDLER handler);
    virtual int unregister_force_change_handler(void *userdata,
	    vrpn_FORCECHANGEHANDLER handler);

    virtual int register_scp_change_handler(void *userdata,
	    vrpn_FORCESCPHANDLER handler);
    virtual int unregister_scp_change_handler(void *userdata,
	    vrpn_FORCESCPHANDLER handler);

    virtual int register_error_handler(void *userdata,
	    vrpn_FORCEERRORHANDLER handler);
    virtual int unregister_error_handler(void *userdata,
	    vrpn_FORCEERRORHANDLER handler);
protected:

    typedef	struct vrpn_RFCS {
	    void				*userdata;
	    vrpn_FORCECHANGEHANDLER	handler;
	    struct vrpn_RFCS		*next;
    } vrpn_FORCECHANGELIST;
    vrpn_FORCECHANGELIST	*change_list;
    static int handle_force_change_message(void *userdata,vrpn_HANDLERPARAM p);

    typedef struct vrpn_RFSCPCS {
	    void                            *userdata;
	    vrpn_FORCESCPHANDLER handler;
	    struct vrpn_RFSCPCS		*next;
    } vrpn_FORCESCPCHANGELIST;
    vrpn_FORCESCPCHANGELIST	*scp_change_list;
    static int handle_scp_change_message(void *userdata,
						    vrpn_HANDLERPARAM p);
    typedef struct vrpn_RFERRCS {
	    void				*userdata;
	    vrpn_FORCEERRORHANDLER handler;
	    struct vrpn_RFERRCS		*next;
    } vrpn_FORCEERRORCHANGELIST;
    vrpn_FORCEERRORCHANGELIST *error_change_list;
    static int handle_error_change_message(void *userdata,
						    vrpn_HANDLERPARAM p);
};

#endif



