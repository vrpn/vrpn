
#ifndef FORCEDEVICE_H
#define  FORCEDEVICE_H

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

#define	FORCE_DEVICE_READY  0
#define FORCE_DEVICE_BROKEN 1

#define MAXPLANE 4   //maximum number of plane in the scene 

// for recovery:
#define DEFAULT_NUM_REC_CYCLES	(10)

// number of floating point values encoded in a message
#define NUM_MESSAGE_PARAMETERS (10)

// possible values for errorCode:
#define FD_VALUE_OUT_OF_RANGE 0	// surface parameter out of range
#define FD_DUTY_CYCLE_ERROR 1	// servo loop is taking too long
#define FD_FORCE_ERROR 2	// max force exceeded, or motors overheated
				// or amplifiers not enabled
#define FD_MISC_ERROR 3		// everything else

class vrpn_ForceDevice {
public:
	vrpn_ForceDevice(char * name, vrpn_Connection *c);
	virtual void mainloop(void) = 0;
	void print_report(void);
	void print_plane(void);

	void setSurfaceKspring(float k) { 
					SurfaceKspring = k; }
	void setSurfaceKdamping(float d) {SurfaceKdamping =d;}
	void setSurfaceFstatic(float ks) {SurfaceFstatic = ks;}
	void setSurfaceFdynamic(float kd) {SurfaceFdynamic =kd;}
	void setRecoveryTime(int rt) {numRecCycles = rt;}
	void setFF_Origin(float x, float y, float z) {
		ff_origin[0] = x;ff_origin[1] = y; ff_origin[2] = z;}
	void setFF_Force(float fx, float fy, float fz) {
		ff_force[0] = fx; ff_force[1] = fy; ff_force[2] = fz;}
	void setFF_Jacobian(float dfxdx, float dfxdy, float dfxdz,
				float dfydx, float dfydy, float dfydz,
				float dfzdx, float dfzdy, float dfzdz){
		ff_jacobian[0][0] = dfxdx; ff_jacobian[0][1] = dfxdy;
		ff_jacobian[0][2] = dfxdz; ff_jacobian[1][0] = dfydx;
		ff_jacobian[1][1] = dfydy; ff_jacobian[1][2] = dfydz;
		ff_jacobian[2][0] = dfzdx; ff_jacobian[2][1] = dfzdy;
		ff_jacobian[2][2] = dfzdz;}
	void setFF_Radius(float r) { ff_radius = r;};
	int getRecoveryTime(void) {return numRecCycles;}
	int connectionAvailable(void) {return (connection != NULL);}

protected:
	vrpn_Connection *connection;		// Used to send messages
	int status;  //status of force device
//	virtual void get_report(void) = 0;
	virtual int encode_to(char *buf);
	virtual int encode_scp_to(char *buf);

	struct timeval timestamp;
	long my_id;		// ID of this force device to connection
	long force_message_id;	// ID of force message to connection
	long plane_message_id;  //ID of plane equation message
	long forcefield_message_id; 	// ID of force field message
	long scp_message_id;	// ID of surface contact point message

	// XXX - error messages should be put into the vrpn base class 
	// whenever someone makes one
	long error_message_id;	// ID of force device error message

        // IDs for trimesh messages
        long setVertex_message_id;   
        long setNormal_message_id;   
        long setTriangle_message_id;   
        long removeTriangle_message_id;   
        long updateTrimeshChanges_message_id;   
        long transformTrimesh_message_id;    
        long setTrimeshType_message_id;    
        long clearTrimesh_message_id;    

	long set_constraint_message_id;

	int   which_plane;
	double force[3];
	double scp_pos[3];
	double scp_quat[4];  // I think this would only be used on 6DOF device
	float plane[4];

	float ff_origin[3];
	float ff_force[3];
	float ff_jacobian[3][3]; // J[i][j] = dF[i]/dx[j]
	float ff_radius;

	float SurfaceKspring;
	float SurfaceKdamping;
	float SurfaceFstatic;
	float SurfaceFdynamic;
	int numRecCycles;
	int errorCode;

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
	double		pos[3];			// position of SCP
	double		quat[4];		// orientation of SCP
} vrpn_FORCESCPCB;
typedef void (*vrpn_FORCESCPHANDLER) (void *userdata,
					const vrpn_FORCESCPCB info);

typedef	struct {
	struct		timeval	msg_time;	// Time of the report
	double		force[3];		// force value
} vrpn_FORCECB;
typedef void (*vrpn_FORCECHANGEHANDLER)(void *userdata,
					 const vrpn_FORCECB info);

typedef struct {
	struct		timeval msg_time;	// time of the report
	int		error_code;		// type of error
} vrpn_FORCEERRORCB;
typedef void (*vrpn_FORCEERRORHANDLER) (void *userdata,
					const vrpn_FORCEERRORCB info);

class vrpn_ForceDevice_Remote: public vrpn_ForceDevice {
public:

	// The name of the force device to connect to
	vrpn_ForceDevice_Remote(char *name);
 	void set_plane(float *p);
	void set_plane(float *p, float d);
	void set_plane(float a, float b, float c,float d);

	void sendSurface(void);
	void startSurface(void);
	void stopSurface(void);

        // vertNum normNum and triNum start at 0
        void setVertex(int vertNum,float x,float y,float z);
        // NOTE: ghost dosen't take normals, 
        //       and normals still aren't implemented for Hcollide
        void setNormal(int normNum,float x,float y,float z);
        void setTriangle(int triNum,int vert0,int vert1,int vert2,
			  int norm0=-1,int norm1=-1,int norm2=-1);
        void removeTriangle(int triNum); 
        // should be called to incorporate the above changes into the displayed trimesh 
        void updateTrimeshChanges();
        // set the trimesh's homogen transform matrix (in row major order)
        void setTrimeshTransform(float homMatrix[16]);
  	void clearTrimesh(void);
  
        // the next time we send a trimesh we will use the following type
        void useHcollide();
        void useGhost();

	void sendConstraint(int enable, float x, float y, float z, float kSpr);

	void sendForceField(float origin[3], float force[3],
		float jacobian[3][3], float radius);
	void sendForceField(void);
	void stopForceField();

	char *encode_plane(int &len);
        char *encode_vertex(int &len,int vertNum,float x,float y,float z); 
        char *encode_normal(int &len,int vertNum,float x,float y,float z); 
        char *encode_triangle(int &len,int triNum,
			      int vert0,int vert1,int vert2,
			      int norm0,int norm1,int norm2);	       
        char *encode_removeTriangle(int &len,int triNum);
        char *encode_updateTrimeshChanges(int &len);
        char *encode_setTrimeshType(int &len,int type);
        char *encode_trimeshTransform(int &len,float homMatrix[16]);

	char *encode_constraint(int &len, int enable, float x, float y, float z,
				float kSpr);
	char *encode_forcefield(int &len, float origin[3],
		float force[3], float jacobian[3][3], float radius);
	// This routine calls the mainloop of the connection it's own
	virtual void mainloop(void);

	// (un)Register a callback handler to handle a force change
	// and plane equation change and trimesh change
	virtual int register_change_handler(void *userdata,
		vrpn_FORCECHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
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
	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);

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



