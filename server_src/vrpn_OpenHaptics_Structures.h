/***************************************************************************************************************/
// The vrpn-OpenHaptics is a device-specific class to handle devices that use OpenHaptics SDK like Phantom devices. 
// The implementation is based on the OpenHaptics v3.0 HLAPI layer. First, it defines methods to initialize, close 
// and reset the haptic device.  Moreover, the class implements a set of virtual methods declared in the 
// vrpn_Tracker_Server, vrpn_Button_Server and vrpn_ForceDevice_Server classes to receive data to set the 
// haptic scene. Data is stored in specific structures or data types, many of them are OpenHaptics types, 
// and used to carry out the haptic rendering. 

//Developed by:
//		Maria Cuevas Rodriguez 					mariacuevas@uma.es
//		Daniel Gonzalez Toledo 					dgonzalezt@uma.es
//Diana Research Group  	-  www.diana.uma.es/index.php?lang=en
//Electronic Technology Dept.
//School of Telecommunications Engineering
//University of Malaga
//Spain
/****************************************************************************************************************/

#ifndef VRPN_OpenHaptics_STRUCTURES_H
#define VRPN_OpenHaptics_STRUCTURES_H

#ifdef	VRPN_USE_PHANTOM_SERVER

// OpenHaptics files
#include <HDU/hduVector.h>
#include <HDU/hduQuaternion.h>
#include <HD/hd.h>
#include <HDU/hduError.h>
#include <HL/hl.h>
#include <HDU/hduMatrix.h>
#include <HLU/hlu.h>
#include <HDU/hdu.h>
#include <HDU/hduMath.h>

// OpenGL files
#include <GL/gl.h>
#include <windows.h>
//#include <GL/glut.h>

// C++ functions
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#ifndef	_WIN32
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif


#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif


//****************************************************************************
//******************** Haptic Scene Struct ***********************************
//****************************************************************************
//Struct to hold the last values sent
struct last_report{
	double last_pos[3];
	double last_quat[4];      
	double last_scpPos[3];
	double last_scpQuat[4];	
	double last_dop;	
	double last_angle;	
	int last_objectNumber;
	HLboolean button0;
	HLboolean button1;
	hduVector3Dd last_ReactionForce;
} ;

//Struct to hold the haptic properties of one object
struct objEffect{
	float stiffness;
	float damping;
	float staticf;
	float dynamicf;
	float popthrough;
	float mass;
};

//Vertex type
struct XYZ{
    float x,y,z;
};

//Struct to hold the haptic object
struct Object{
	HLuint objId;			//This Id is given by openhaptics
	//char objName;
	double transform[16];
	objEffect objEffect;
	bool isTouchable;
	//Object's vertices		
	std::vector <XYZ> objVertex;	
	//Reaction Force Vector
	hduVector3Dd reactionForce;
	hduVector3Dd shapeReactionForce;
};

// Struct to hold the effects
struct Effects{
	int effect_index;		//This Id is given by the Vrpn Client
	HLuint effectId;		//This Id is given by openhaptics
	char type[20];	
	double gain;	
	double magnitude;
	double duration;
	double frequency;
	double position[3];
	double direction[3];
	bool effect_active;
	bool flagStateChanged;
};

//Struc to hold the workspace 
struct workSpaceStruct{
	HLdouble minPoint[3];
	HLdouble maxPoint[3];
	HLdouble modelviewMatrix[16];
	double projectionMatrix[16];
};


struct phantomVariable{
	
	//Proxy onMotion control
		bool proxyOnMotionTracker;
		bool proxyOnMotionForceDevice;		
	//****************************************
	//Variable to be use in Force Modelization
	//****************************************		
		//Depth of Penetration
		HLdouble dop;
		HLdouble deltaDop;	
		//Device and Proxy position and orientation		
		hduVector3Dd devicePos;
		hduQuaternion deviceRot;
		hduVector3Dd proxyPos;
		hduQuaternion proxyRot;
		//Proxy Contact State - isTouching?
		HLboolean contactState;
		//Proxy Touch normal
		hduVector3Dd normalProxy;
		//Modification on the 24/02/11
		hduVector3Dd unitNormalProxy;
		//Proxy Direction vector for Straight tool
		hduVector3Dd proxyDirVector;
		//Proxy Up vector for Angle tool
		hduVector3Dd proxyUpVector;		
		//Damping & Stiffness parameters
		HLdouble dampingK;
		HLdouble StiffnessK;
		HDdouble proxyT[16];
	//****************************************
	//Vibration Model
	//****************************************
		int effectVibMotorIndex;
		int effectVibContactIndex;		
		hduVector3Dd motorVibForce;
		hduVector3Dd contactVibForce;
		hduVector3Dd motorVibForceX;
		hduVector3Dd VibrationDirectionX;
		hduVector3Dd XproxyVibDirection;
		hduVector3Dd motorVibForceY;
		hduVector3Dd VibrationDirectionY;
		hduVector3Dd YproxyVibDirection;
		hduVector3Dd contactVibDirection;
		hduVector3Dd ZproxyContactVibDirection;	
	//****************************************
	//Angle Calculation
	//****************************************		
		double angle;
		double angleInDegree;
};
#endif
#endif