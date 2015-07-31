
/***************************************************************************************************************/
/******************************* vrpn_OpenHaptics class with HLAPI support ******************************************
/****************************************************************************************************************/


/***************************************************************************************************************/
/* HDAPI support has been removed due to the inconvenience of handling both HLAPI & HDAPI in the same program. 
/* Support for Trimesh & Ghost classes has been partially removed, and changed to a new class: RigidBodyForm, that 
/* manages bodies haptic constants and its own mesh characteristics. 
/* Planes that were managed through HLAPI have been removed, and translated to a generic class: RigidBodyForm, that
/* handles any kind of 3D mesh.
/* The main server class runs a window that displays what it's been rendered by the haptic device
/****************************************************************************************************************/

#ifndef VRPN_OpenHaptics_H
#define VRPN_OpenHaptics_H

#include "vrpn_Configure.h"

#ifdef	VRPN_USE_PHANTOM_SERVER

#include "vrpn_OpenHaptics_Structures.h"
#include "glWindow.h"

///Inclusion of headers
// Vrpn files
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
//#include "vrpn_ForceDeviceServer_Uma.h"
#include "vrpn_ForceDeviceServer.h"
#include "vrpn_Connection.h"
//#include "quat.h"

//******************************* MAIN CLASS ***********************************************
//* Class vrpn_Phantom, it inherits from vrpn_ForceDeviceServer_Uma, vrpn_Tracker 
//* and vrpn_Button_Filter
//******************************************************************************************

class vrpn_OpenHaptics: public vrpn_ForceDeviceServer,public vrpn_Tracker_Server,public vrpn_Button_Filter 
{
	friend void phantomErrorHandler( int errnum, char *description,void *userdata);

	public:
	
	// Constructor and destructor of the object
	vrpn_OpenHaptics(char *name, vrpn_Connection *c, float hz = 1.0, float minimumDistance = 1.0 , float minimumRotation = 0.02 , const char * newsconf = "Default PHANToM");
	vrpn_OpenHaptics();
	~vrpn_OpenHaptics();

	// Phantom class mainloop
	virtual void mainloop(void);

	
	protected:
	//******************************************************************************
	//							Variable OpenHaptics
	//******************************************************************************
	// The Phantom hardware device we are using
	HHD		          phantom;	  	
	// Handle to haptic rendering context
	HHLRC             hHLRC; 
				
	//******************************************************************************
	//							Variable Local
	//******************************************************************************
	struct timeval timestamp;// Value of time when the report has been generated
	double update_rate;
	
	//******************************************************************************
	//							Virtual Functions
	//******************************************************************************
	//TODO change float and double for vrpn_float32
	virtual bool setObjectNumber(vrpn_int32 num);
	//virtual bool setVertex (vrpn_int32 objNum, vrpn_float64 *Vertex);
	virtual bool setVertex(vrpn_int32 objNum, vrpn_int32 vertNum, vrpn_float32 x, vrpn_float32 y, vrpn_float32 z);
	virtual bool setTransformMatrix(vrpn_int32 CurrentObject, vrpn_float64 *transformMatrix);		
	virtual bool setEffect(char *type, vrpn_int32 effect_index, vrpn_float64 gain, vrpn_float64 magnitude, vrpn_float64 duration,
		vrpn_float64 frequency, vrpn_float64 position[3], vrpn_float64 direction[3]);
	virtual bool startEffect(vrpn_int32 effect_index);
	virtual bool stopEffect(vrpn_int32 effect_index);		
	virtual bool setObjectIsTouchable(vrpn_int32 objNum, vrpn_bool IsTouchable = true); // make an object touchable or not
	virtual bool setHapticProperty(vrpn_int32 objNum, char *type, vrpn_float32 k);
	/*virtual bool setEnvironmentalParameters(vrpn_float32 gravity, vrpn_float32 inertia);*/
	virtual bool setTouchableFace(vrpn_int32 TFace);
	virtual bool setWorkspaceProjectionMatrix(vrpn_float64 *modelMatrix, vrpn_float64 *matrix);
	virtual bool setWorkspaceBoundingBox(vrpn_float64 *modelMatrix, vrpn_float64 *matrix);
	virtual bool resetDevice(void); // Reset haptic device
	

	// None of the scene-orienting or object-creation methods are supported yet, but
	// we need to create non-empty functions to handle them.

	// Add an object to the haptic scene as root (parent -1 = default) or as child (ParentNum =the number of the parent)
	virtual bool addObject(vrpn_int32 objNum, vrpn_int32 ParentNum = -1) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// Add an object next to the haptic scene as root 
	virtual bool addObjectExScene(vrpn_int32 objNum) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};
	
	// NOTE: ghost doesn't take normals, 
	//       and normals still aren't implemented for Hcollide
	virtual bool setNormal(vrpn_int32 objNum, vrpn_int32 normNum, vrpn_float32 x, vrpn_float32 y, vrpn_float32 z) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	virtual bool setTriangle(vrpn_int32 objNum, vrpn_int32 triNum, vrpn_int32 vert0, vrpn_int32 vert1, vrpn_int32 vert2,
		vrpn_int32 norm0 = -1, vrpn_int32 norm1 = -1, vrpn_int32 norm2 = -1) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	virtual bool removeTriangle(vrpn_int32 objNum, vrpn_int32 triNum) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// should be called to incorporate the above changes into the 
	// displayed trimesh 
	virtual bool updateTrimeshChanges(vrpn_int32 objNum, vrpn_float32 kspring, vrpn_float32 kdamp, vrpn_float32 fdyn, vrpn_float32 fstat) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// set trimesh type
	virtual bool setTrimeshType(vrpn_int32 objNum, vrpn_int32 type) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// set the trimesh's homogen transform matrix (in row major order)
	virtual bool setTrimeshTransform(vrpn_int32 objNum, vrpn_float32 homMatrix[16]) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// set position of an object
	virtual bool setObjectPosition(vrpn_int32 objNum, vrpn_float32 Pos[3]) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// set orientation of an object
	virtual bool setObjectOrientation(vrpn_int32 objNum, vrpn_float32 axis[3], vrpn_float32 angle) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// set Scale of an object
	virtual bool setObjectScale(vrpn_int32 objNum, vrpn_float32 Scale[3]) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// remove an object from the scene
	virtual bool removeObject(vrpn_int32 objNum) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	virtual bool clearTrimesh(vrpn_int32 objNum) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	/** Functions to organize the scene	**********************************************************/
	// Change The parent of an object
	virtual bool moveToParent(vrpn_int32 objNum, vrpn_int32 ParentNum) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// Set the Origin of the haptic scene
	virtual bool setHapticOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// Set the Scale factor of the haptic scene
	virtual bool setHapticScale(vrpn_float32 Scale) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};

	// Set the Origin of the haptic scene
	virtual bool setSceneOrigin(vrpn_float32 Pos[3], vrpn_float32 axis[3], vrpn_float32 angle) {
		struct timeval now;
		vrpn_gettimeofday(&now, NULL);
		send_text_message("vrpn_OpenHaptics: Called a function not supported", now, vrpn_TEXT_ERROR);
		return false;
	};
	
private:
	//******************************************************************************
	//							Variables
	//******************************************************************************
	// Add a field which contain the name of the PHANToM configuration
	char sconf[512];			//New line to introduce our phantom server in VRPN 0733
	
	//Add a field which contain the parameter of EVENT_MOTION_TOLERANCE
	float eventMotionLinearTolerance;
	float eventMotionAngularTolerance;

	//Button state to use by Vrpn_button
	HLboolean button_State[2];						
	//To store the device model type
	HDstring deviceType;
	// Scene number of objects
	int NumberOfObjects;
	//Scene number of effects
	int NumberOfEffects;
	//Scene number of vertices
	int NumberOfVertex;	
	//To store when phantom device is correctly initialized. (true = correct)
	bool phantomDeviceInitialization;		
	//To store workspace parameters
	workSpaceStruct wsParameters;	
	//To Store the object that is touched in every moment.
	int objTouchingNumber;	
	//Variable to store last value of haptic variable
	last_report lastData;
	
	//Store all the objects and their properties
	std::vector <Object> object;
	//Store all the effects
	std::vector <Effects> effect;
	//Store all data read from PhantomDevice
	phantomVariable phantomVariable;
	
	// Touchable face variable: 0- FRONT 1- BACK Other- FRONT and BACK
	int TouchableFace;	
	// Proxy transform matrix
	double proxyxform[16];	
	// To avoid Several Reset in a Row in case it is received from the Network, several time the Reset cmd
	bool resetReady;
	//Variables to control the vibration and tangencial force
	HDSchedulerHandle callbackHandle;
	bool activeComputeforceCB;
	// Vector containing the gravity force magnitude
	hduVector3Dd gravityVector;
	// Boolean that indicates whether an inertia force has been activated or not
	int inertiaBoolean;
	//Force vector
	//vrpn_float64 d_force[3];

	//******************************************************************************
	//							Functions
	//******************************************************************************
	//Vibration Control
	void startVibration();
	void stopVibration();
	//To compare two variables read from an analog device
	bool AreSame(double a, double b, double EPSILON);
	//To calculate the angle
	void checkAngle();
	//******************************************************************************
	//				Function to send data from the server to the clients
	//******************************************************************************	
	void sendProxyPosOrient(void);
	void sendForce(int objNum);
	void sendDOP(void);
	void sendSCP(void);
	void sendIsTouching(void);
	void sendTouchedObject(void);
	void sendAngle(void);
	void sendButtons(void);

	//******************************************************************************
	//					OpenHaptics Functions - Render Functions
	//******************************************************************************	
	// Init haptic device.
	void initHL(HHD phantom);
	// Render objects
	void renderHL(int objNum);
	// Render effects
	void renderHLeffect(int effectNum);

	//******************************************************************************************
	//********************   OpenHaptics Callback declaration **********************************
	//******************************************************************************************	
	//******************************************************************************
	//   Event callback triggered when an object is being touched
	//******************************************************************************
	static void HLCALLBACK ObjectIsTouched(HLenum event, HLuint objectId, HLenum thread,
		HLcache *cache, void *userdata);
	static void HLCALLBACK ObjectIsUnTouched(HLenum event, HLuint object, HLenum thread,
		HLcache *cache, void *userdata);
	//******************************************************************************
	//Event callback triggered when an the proxy in on motion
	//******************************************************************************
	static void HLCALLBACK ProxyOnMotion(HLenum event, HLuint objectId, HLenum thread,
		HLcache *cache, void *userdata);
	//******************************************************************************
	//   Event callback triggered to calculate Specific Force effect (Vibration and Tangencial forces)
	//******************************************************************************
	static HDCallbackCode HDCALLBACK hdComputeforceCB(void *pUserData);

	//******************************************************************************************
	//******************************* END CALLBACKS Declarations *******************************
	//******************************************************************************************
};

#endif

#endif