#ifndef FORCEDEVICE_H
#define  FORCEDEVICE_H

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"
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

// If defined, springs are implemented in the client as force fields.
// If not, springs are implemented with special messages
// and extra Ghost classes.  Either way support for the messages
// is compiled into the parent class so that servers can support
// both kinds of clients.

// (Springs as force fields require some knotty mathematical programming
// at the clients that I can't seem to get right, but avoid lots of
// extra message types and an awful lot of bug-prone Ghost.)

#define FD_SPRINGS_AS_FIELDS


class VRPN_API vrpn_ForceDevice : public vrpn_BaseClass {

  public:

    vrpn_ForceDevice (const char * name, vrpn_Connection *c);
    virtual ~vrpn_ForceDevice (void);

    void print_report (void);
    void print_plane (void);

    void setSurfaceKspring (vrpn_float32 k) { SurfaceKspring = k; }
    void setSurfaceKdamping (vrpn_float32 d) { SurfaceKdamping =d; }
    void setSurfaceFstatic (vrpn_float32 ks) { SurfaceFstatic = ks; }
    void setSurfaceFdynamic (vrpn_float32 kd) { SurfaceFdynamic =kd; }
    void setRecoveryTime (int rt) { numRecCycles = rt; }

    // additional surface properties
    void setSurfaceKadhesionNormal (vrpn_float32 k)
           { SurfaceKadhesionNormal = k; }
    void setSurfaceKadhesionLateral (vrpn_float32 k) 
           { SurfaceKadhesionLateral = k; }
    void setSurfaceBuzzFrequency (vrpn_float32 freq)
           { SurfaceBuzzFreq = freq; }
    void setSurfaceBuzzAmplitude (vrpn_float32 amp)
           { SurfaceBuzzAmp = amp; }
    void setSurfaceTextureWavelength (vrpn_float32 wl) 
           { SurfaceTextureWavelength = wl; }
    void setSurfaceTextureAmplitude (vrpn_float32 amp) 
           { SurfaceTextureAmplitude = amp; }
    
    void setCustomEffect(vrpn_int32 effectId, vrpn_float32 *params = NULL, vrpn_uint32 nbParams = 0);

    void setFF_Origin (vrpn_float32 x, vrpn_float32 y, vrpn_float32 z)
           { ff_origin[0] = x; ff_origin[1] = y; ff_origin[2] = z; }
    void setFF_Origin (vrpn_float32 x [3])
           { ff_origin[0] = x[0]; ff_origin[1] = x[1]; ff_origin[2] = x[2]; }
    void setFF_Force (vrpn_float32 fx, vrpn_float32 fy, vrpn_float32 fz)
           { ff_force[0] = fx; ff_force[1] = fy; ff_force[2] = fz; }
    void setFF_Force (vrpn_float32 f [3])
           { ff_force[0] = f[0]; ff_force[1] = f[1]; ff_force[2] = f[2]; }
    void setFF_Jacobian
             (vrpn_float32 dfxdx, vrpn_float32 dfxdy, vrpn_float32 dfxdz,
              vrpn_float32 dfydx, vrpn_float32 dfydy, vrpn_float32 dfydz,
              vrpn_float32 dfzdx, vrpn_float32 dfzdy, vrpn_float32 dfzdz)
           { ff_jacobian[0][0] = dfxdx; ff_jacobian[0][1] = dfxdy;
	     ff_jacobian[0][2] = dfxdz; ff_jacobian[1][0] = dfydx;
	     ff_jacobian[1][1] = dfydy; ff_jacobian[1][2] = dfydz;
	     ff_jacobian[2][0] = dfzdx; ff_jacobian[2][1] = dfzdy;
	     ff_jacobian[2][2] = dfzdz; }
    void setFF_Radius (vrpn_float32 r) { ff_radius = r; }

    void set_plane (vrpn_float32 *p);
    void set_plane (vrpn_float32 *p, vrpn_float32 d);
    void set_plane (vrpn_float32 a, vrpn_float32 b, vrpn_float32 c,
                    vrpn_float32 d);

    void sendError (int error_code);

    int getRecoveryTime (void) {return numRecCycles;}
    int connectionAvailable (void) {return (d_connection != NULL);}

    // constants for constraint messages

    enum ConstraintGeometry
            { NO_CONSTRAINT,
              POINT_CONSTRAINT,
              LINE_CONSTRAINT,
              PLANE_CONSTRAINT };

  protected:

    virtual int register_types(void);

    vrpn_int32 force_message_id;	// ID of force message to connection
    vrpn_int32 plane_message_id;	//ID of plane equation message
	vrpn_int32 plane_effects_message_id; // additional plane properties
    vrpn_int32 forcefield_message_id; 	// ID of force field message
    vrpn_int32 scp_message_id;		// ID of surface contact point message


    // constraint messages

    vrpn_int32 enableConstraint_message_id;
    vrpn_int32 setConstraintMode_message_id;
    vrpn_int32 setConstraintPoint_message_id;
    vrpn_int32 setConstraintLinePoint_message_id;
    vrpn_int32 setConstraintLineDirection_message_id;
    vrpn_int32 setConstraintPlanePoint_message_id;
    vrpn_int32 setConstraintPlaneNormal_message_id;
    vrpn_int32 setConstraintKSpring_message_id;
    //vrpn_int32 set_constraint_message_id;// ID of constraint force message

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

    // ajout ONDIM
    vrpn_int32 custom_effect_message_id;
    // fni ajout ONDIM

    // ENCODING
    // ajout ONDIM
    static char *encode_custom_effect(vrpn_int32 &len, vrpn_uint32 effectId, 
		const vrpn_float32 *params, vrpn_uint32 nbParams);
    // fin ajout ONDIM
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

    static char *encode_forcefield(vrpn_int32 &len, const vrpn_float32 origin[3],
	const vrpn_float32 force[3], const vrpn_float32 jacobian[3][3], const vrpn_float32 radius);
    static char *encode_error(vrpn_int32 &len, const vrpn_int32 error_code);


    // DECODING
    // ajout ONDIM
    static vrpn_int32 decode_custom_effect(const char*buffer, const vrpn_int32 len,
		vrpn_uint32 *effectId, vrpn_float32 **params, vrpn_uint32 *nbParams);
    // fin ajout ONDIM
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

    static vrpn_int32 decode_forcefield(const char *buffer,const vrpn_int32 len,
	vrpn_float32 origin[3], vrpn_float32 force[3], vrpn_float32 jacobian[3][3], vrpn_float32 *radius);
    static vrpn_int32 decode_error(const char *buffer, const vrpn_int32 len, 
		vrpn_int32 *error_code);

    // constraint encoding & decoding

    static char * encode_enableConstraint
                   (vrpn_int32 & len,
                    vrpn_int32 enable);
    static vrpn_int32 decode_enableConstraint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_int32 * enable);
                         
    static char * encode_setConstraintMode
                   (vrpn_int32 & len,
                    ConstraintGeometry mode);
    static vrpn_int32 decode_setConstraintMode
                   (const char * buffer,
                    const vrpn_int32 len,
                    ConstraintGeometry * mode);
                         
    static char * encode_setConstraintPoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decode_setConstraintPoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);
                         
    static char * encode_setConstraintLinePoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decode_setConstraintLinePoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);
                         
    static char * encode_setConstraintLineDirection
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decode_setConstraintLineDirection
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);
                         
    static char * encode_setConstraintPlanePoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decode_setConstraintPlanePoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);
                         
    static char * encode_setConstraintPlaneNormal
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decode_setConstraintPlaneNormal
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);
                         
    static char * encode_setConstraintKSpring
                   (vrpn_int32 & len,
                    vrpn_float32 k);
    static vrpn_int32 decode_setConstraintKSpring
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * k);

    // utility functions

    static char * encodePoint
                   (vrpn_int32 & len,
                    vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
    static vrpn_int32 decodePoint
                   (const char * buffer,
                    const vrpn_int32 len,
                    vrpn_float32 * x, vrpn_float32 * y, vrpn_float32 * z);


    struct timeval timestamp;

    vrpn_int32   which_plane;

    vrpn_float64 d_force [3];
      ///< d_force isn't used in vrpn_ForceDevice, but seems to be used
      ///< by derived classes?  What's the meaning?

    vrpn_float64 scp_pos [3];
    vrpn_float64 scp_quat [4];  // for torque
    vrpn_float32 plane [4];

    vrpn_float32 ff_origin [3];
    vrpn_float32 ff_force [3];
    vrpn_float32 ff_jacobian [3][3]; // J[i][j] = dF[i]/dx[j]
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
	
    // ajout ONDIM
    vrpn_int32	customEffectId;
    vrpn_float32 *customEffectParams;
    vrpn_uint32	nbCustomEffectParams;
    // fin ajout ONDIM
};

// User routine to handle position reports for surface contact point (SCP)
// This is in vrpn_ForceDevice rather than vrpn_Tracker because only
// a force feedback device should know anything about SCPs as this is a
// part of the force feedback model. It may be preferable to use the SCP
// rather than the tracker position for graphics so the hand position
// doesn't appear to go below the surface making the surface look very
// compliant. 
typedef struct _vrpn_FORCESCPCB {
	struct		timeval msg_time;	// Time of the report
	vrpn_float64	pos[3];			// position of SCP
	vrpn_float64	quat[4];		// orientation of SCP
} vrpn_FORCESCPCB;
typedef void (VRPN_CALLBACK *vrpn_FORCESCPHANDLER) (void *userdata,
					const vrpn_FORCESCPCB info);

typedef	struct _vrpn_FORCECB {
	struct		timeval	msg_time;	// Time of the report
	vrpn_float64	force[3];		// force value
} vrpn_FORCECB;
typedef void (VRPN_CALLBACK *vrpn_FORCECHANGEHANDLER)(void *userdata,
					 const vrpn_FORCECB info);

typedef struct _vrpn_FORCEERRORCB {
	struct		timeval msg_time;	// time of the report
	vrpn_int32		error_code;		// type of error
} vrpn_FORCEERRORCB;
typedef void (VRPN_CALLBACK *vrpn_FORCEERRORHANDLER) (void *userdata,
					const vrpn_FORCEERRORCB info);

class VRPN_API vrpn_ForceDevice_Remote: public vrpn_ForceDevice {
public:

    // The name of the force device to connect to.
    // The connection argument is used only if you already have a connection
    // the device must listen on (it is not normally used).
    vrpn_ForceDevice_Remote(const char *name, vrpn_Connection *cn = NULL);
    virtual ~vrpn_ForceDevice_Remote (void);

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

    // Generalized constraint code.
    // Constrains as a spring connected to a point, sliding along a line
    // (constraint forces in a plane perpendicular to the line), or
    // sliding along a plane (constraint forces only along the plane's
    // normal).  LineDirection and PlaneNormal should be normalized
    // (vector length == 1).

    // Constraints are implemented as force fields, so both cannot
    // run at once.

    // XXX it would be safer if changes (especially enable/disable)
    // had better relaxation support

    void enableConstraint (vrpn_int32 enable);  // zero disables
    void setConstraintMode (ConstraintGeometry mode);
    void setConstraintPoint (vrpn_float32 point [3]);
    void setConstraintLinePoint (vrpn_float32 point [3]);
    void setConstraintLineDirection (vrpn_float32 direction [3]);
    void setConstraintPlanePoint (vrpn_float32 point [3]);
    void setConstraintPlaneNormal (vrpn_float32 normal [3]);
    void setConstraintKSpring (vrpn_float32 k);

    //void sendConstraint (vrpn_int32 enable, vrpn_float32 x,
                         //vrpn_float32 y, vrpn_float32 z, vrpn_float32 kSpr);

    // At the <origin> of the field, user feels the specified <force>.
    // As the user moves away from the origin, the force felt changes
    // according to the jacobian.  If the user moves further than <radius>
    // from <origin>, the field cuts out.

    // XXX it would be safer for the field to attenuate rapidly
    // from the value at the radius if the user moves beyond the radius

    void sendForceField
           (vrpn_float32 origin [3], vrpn_float32 force [3],
	    vrpn_float32 jacobian [3][3], vrpn_float32 radius);
    void sendForceField (void);
    void stopForceField (void);

    // ajout ONDIM
    void startEffect (void);
    void stopEffect (void);
    // fin ajout ONDIM

    // This routine calls the mainloop of the connection it is on
    virtual void mainloop ();

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
    static int VRPN_CALLBACK handle_force_change_message(void *userdata,vrpn_HANDLERPARAM p);

    typedef struct vrpn_RFSCPCS {
	    void                            *userdata;
	    vrpn_FORCESCPHANDLER handler;
	    struct vrpn_RFSCPCS		*next;
    } vrpn_FORCESCPCHANGELIST;
    vrpn_FORCESCPCHANGELIST	*scp_change_list;
    static int VRPN_CALLBACK handle_scp_change_message(void *userdata,
						    vrpn_HANDLERPARAM p);
    typedef struct vrpn_RFERRCS {
	    void				*userdata;
	    vrpn_FORCEERRORHANDLER handler;
	    struct vrpn_RFERRCS		*next;
    } vrpn_FORCEERRORCHANGELIST;
    vrpn_FORCEERRORCHANGELIST *error_change_list;
    static int VRPN_CALLBACK handle_error_change_message(void *userdata,
						    vrpn_HANDLERPARAM p);

    // constraint types

    vrpn_int32 d_conEnabled;
    ConstraintGeometry d_conMode;
    vrpn_float32 d_conPoint [3];
    vrpn_float32 d_conLinePoint [3];
    vrpn_float64 d_conLineDirection [3];  // (assumed) normalized
    vrpn_float32 d_conPlanePoint [3];
    vrpn_float64 d_conPlaneNormal [3];  // (assumed) normalized
    vrpn_float32 d_conKSpring;

    // utility functions

    void send (const char * msgbuf, vrpn_int32 len, vrpn_int32 type);
      // Takes a pointer to a buffer, the length of the buffer, and the
      // vrpn message type id to send.  Sends the buffer reliably
      // over connection AND DELETES THE BUFFER.

#ifdef FD_SPRINGS_AS_FIELDS

    void constraintToForceField (void);
      // takes the current cs_* settings and translates them into
      // a force field.

#endif  // FD_SPRINGS_AS_FIELDS

};

#endif

