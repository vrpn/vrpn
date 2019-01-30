/***************************************************************************************************************/
/******************************* vrpn_OpenHaptics class with HLAPI support *************************************
/****************************************************************************************************************/

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

#include  "vrpn_Configure.h"

#ifdef	VRPN_USE_PHANTOM_SERVER
#include "vrpn_OpenHaptics.h"


static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


//**********************************************************************************************************
//****************************************  CONSTRUCTOR  ***************************************************
//**********************************************************************************************************
vrpn_OpenHaptics::vrpn_OpenHaptics(char *name, vrpn_Connection *c, float hz, float minimumDistance, float minimumRotation, const char * newsconf)
		:vrpn_Tracker_Server(name, c,2),vrpn_Button_Filter(name,c),
		 vrpn_ForceDeviceServer(name,c), update_rate(hz)
{  
	printf("\n---------------------------------------------");
	printf("\nHello and Welcome to Vrpn_OpenHaptics_Server");
	printf("\n---------------------------------------------");
	//************************************************************************
	//**********************    Create one empty object  *********************
	//************************************************************************
	Object temp;
	//Set the object touchable by default
	temp.isTouchable = true;
	//Init Haptic Properties
	temp.objEffect.stiffness = 0;
	temp.objEffect.damping = 0;
	temp.objEffect.staticf = 0;
	temp.objEffect.dynamicf = 0;
	temp.objEffect.popthrough = 0;
	temp.objEffect.mass = 0;
	
	//Add the empty object to the struct	
	object.push_back(temp);

	//*************************************************************************
	//************************ Init Variable Section **************************
	//*************************************************************************
	vrpn_Button_Filter::num_buttons = 2; 
	//Variable to save the number of objects that there are, the value of it variable is recieved with setObjectNumber
	NumberOfObjects = 1;
	//Variable to count the number of effects that there are, it increases each time the client calls setEffect
	NumberOfEffects=0;
	//Variable to count the number of vertexs
	NumberOfVertex = 0;
	//None object touched
	objTouchingNumber = -1;
	//phantomDeviceInitialization It's true when the device is initialize correctly.
	phantomDeviceInitialization = false;
	//proxyOnMotionTracker It's true when the proxy is on move.
	phantomVariable.proxyOnMotionTracker = true; 
	//proxyOnMotionFD It's true when the proxy is on move.
	phantomVariable.proxyOnMotionForceDevice = false;
	//The device can not be reseted.
	resetReady = false;
	// Boolean that indicates whether an inertia force has been activated or not
	inertiaBoolean = 0;
	//initialize last report dop to 0
	lastData.last_dop = 0;
	//Initialize the Vibration control var to off
	activeComputeforceCB = false;
	//Hannilliate Vibration Components
	phantomVariable.effectVibMotorIndex = -1;
	phantomVariable.effectVibContactIndex = -1;
	//Init setObjNumberReady parameter to enable start set objnumber function
	//setObjNumberReady = true;
	
	// Create Our OpenGL Window to help in the haptic render functions.
	CreateGLWindow("HAPTIC SERVER", 1024, 768, 8, false);
	
	//*************************************************************************
	//***************************** Time Stamp ********************************
	//*************************************************************************
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;  
	vrpn_gettimeofday(&(vrpn_ForceDevice::timestamp),NULL);
	
	//*************************************************************************
	//************************ Phantom Initialization *************************
	//*************************************************************************
	printf("\nTrying to Initialize the Phantom Device....");
	HDErrorInfo error;
	// Initialize the 'sconf' field
	strcpy(sconf, newsconf);					//New line to introduce our phantom server in VRPN 0733		
	phantom = hdInitDevice(sconf);				//New line to introduce our phantom server in VRPN 0733	
	if (HD_DEVICE_ERROR(error = hdGetError())) {
		hduPrintError(stderr, &error, "\n\nFailed to initialize haptic device.\n");
		phantom = -1;
		phantomDeviceInitialization = false;
	} else {		
		eventMotionLinearTolerance = minimumDistance;
		eventMotionAngularTolerance = minimumRotation;
		initHL(phantom);
		phantomDeviceInitialization = true;
		resetReady = true;
		
		//Find and show Device type and other parameters.
		deviceType = hdGetString(HD_DEVICE_MODEL_TYPE);		
		//printf("Device model: %s\n", deviceType);						
		HDdouble firmware = 0;
		hdGetDoublev(HD_DEVICE_FIRMWARE_VERSION, &firmware);
		printf("\n  Hdapi : %s\n  Device Model: %s\n  Vendor: %s\n  Driver: %s\n  Firmware: %g\n  Serial: %s",
			hdGetString(HD_VERSION),		
			deviceType,
			hdGetString(HD_DEVICE_VENDOR),
			hdGetString(HD_DEVICE_DRIVER_VERSION),
			firmware,
			hdGetString(HD_DEVICE_SERIAL_NUMBER)
			);
		printf("\n\nWaiting Client....");
	}
}


///**********************************************************************************************************
//****************************************  DESTRUCTOR  *****************************************************
//***********************************************************************************************************

vrpn_OpenHaptics::~vrpn_OpenHaptics()
{
		
	//Stop Vibration effect.
	stopVibration();
	//Delete HL Shapes and Context 
	for (unsigned int i = 0; i<object.size(); i++)
		hlDeleteShapes(object[i].objId, 1);
	for (unsigned int j = 0; j<effect.size(); j++){
		hlStopEffect(effect[j].effectId);
		hlDeleteEffects(effect[j].effectId, 1);
	}
	//Clear all the lists
	for (unsigned int i = 0; i<object.size(); i++)
		object[i].objVertex.clear();
	object.clear();
	effect.clear();
	//Release the phantom device
	if (phantom != -1) {
		// free up the haptic rendering context
		hlMakeCurrent(NULL);
		if (hHLRC != NULL)
		{
			hlDeleteContext(hHLRC);
		}
		//free up the haptic device
		if (phantom != HD_INVALID_HANDLE)
		{
			hdDisableDevice(phantom);
		}
	}
	printf("\nBye Bye.\n");
}
	
///**************************************************************************************************
/// <summary> MAIN LOOP. Haptic Render and Get parameters from Phantom by OpenHaptics.<\summary>
///**************************************************************************************************
void vrpn_OpenHaptics::mainloop(void) 
{
	struct timeval current_time;
	
	
	
	// Allow the base server class to do its thing
	server_mainloop();
	
	
	if (phantomDeviceInitialization){		
		///////////////////////////////////////////////////////////////////////////
		///////////////////////// HAPTIC RENDER PASS //////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		hlBeginFrame();
			/////////////////////
			//  Render objects
			/////////////////////			
			for (unsigned int i = 0; i < object.size(); i++)
			{
				if (object[i].isTouchable)
					renderHL(i);
			}

			/////////////////////
			//  Render effects
			/////////////////////			
			for (unsigned int j = 0; j < effect.size(); j++)
			{
				if (effect[j].flagStateChanged)
					renderHLeffect(j);			
			}

			/////////////////////////////////////
			// Get parameters from OpenHaptics
			/////////////////////////////////////

			//Get State of Contact Proxy
			hlGetBooleanv(HL_PROXY_IS_TOUCHING, &phantomVariable.contactState);

			// Get and Send reaction force 		
			if (phantomVariable.contactState){				
				//objTouchingNumber is changed by a callback asynchronous. 
				//Just in case, we store its value in a temporary varible to use it to index the array.
				int i = objTouchingNumber;
				if (i != -1){
					//Get the reaction force as:
					//Vector of 3 doubles representing the contribution of this shape to the overall reaction force
					//that was sent to the haptic device during the last frame. If the proxy was not in contact with
					//this shape last frame, this vector will be zero

					//Modification on the 25/02/11
					//hlGetShapeDoublev(object[i].objId, HL_REACTION_FORCE, object[i].reactionForce);
					hlGetShapeDoublev(object[i].objId, HL_REACTION_FORCE, object[i].shapeReactionForce);
					//Calculation the contact normal to the shape
					hduVecNormalize(phantomVariable.unitNormalProxy, phantomVariable.normalProxy);
					double res = hduVecDotProduct(object[i].shapeReactionForce, phantomVariable.unitNormalProxy);
					hduVecScale(object[i].reactionForce, phantomVariable.unitNormalProxy, res);

					//Send the reaction force applied over the object.				
					//printf("object %d - reactionForce %lf %lf %lf\n",i,object[i].reactionForce[0], object[i].reactionForce[1], object[i].reactionForce[2] );				
					if (object[i].reactionForce[0] != 0 || object[i].reactionForce[1] != 0 || object[i].reactionForce[2] != 0)
					{
						// Send the reaction force applied over the object.
						sendForce(i);			
					}
				
				}			
			}				
			//Get Device Position
			hlGetDoublev(HL_DEVICE_POSITION, phantomVariable.devicePos);
			//Get Device Orientation - It is equal to the Proxy rotation
			hlGetDoublev(HL_DEVICE_ROTATION, phantomVariable.deviceRot);
			//Get Proxy Position
			hlGetDoublev(HL_PROXY_POSITION, phantomVariable.proxyPos);
			//Get Proxy Orientation - It is equal to the Device rotation
			hlGetDoublev(HL_PROXY_ROTATION, phantomVariable.proxyRot);		
			//Get Depth of Penetration
			hlGetDoublev(HL_DEPTH_OF_PENETRATION, &phantomVariable.dop);
			//Switch dop from mm to m: dop = hduVecMagnitude(DevicePos-ProxyPos);
			phantomVariable.dop = phantomVariable.dop / 1000;

			//Calculate Velocity of penetration
			phantomVariable.deltaDop = phantomVariable.dop - lastData.last_dop;
		
			//Get button 1 state   
			hlGetBooleanv(HL_BUTTON1_STATE, &button_State[0]);
			//Get button 2 state	TODO check type of device
			hlGetBooleanv(HL_BUTTON2_STATE, &button_State[1]);
	
			//Get the force
			//hlGetDoublev(HL_DEVICE_FORCE, d_force);

			//Modification on the 25/02/11
			//Get the Normal to shape when proxy touches this shape
			hlGetDoublev(HL_PROXY_TOUCH_NORMAL, phantomVariable.normalProxy);
			hlGetDoublev(HL_PROXY_TRANSFORM, proxyxform);
					

			//Get the Proxy transformation matrix
			hdGetDoublev(HD_CURRENT_TRANSFORM, phantomVariable.proxyT);		
			//Get Motor Temperature for 6DOF encoder device
			/*HDdouble temperature[6];
			hdGetDoublev(HD_MOTOR_TEMPERATURE, temperature);
			printf("temperature %f %f %f %f %f %f \n", temperature[0], temperature[1], temperature[2], temperature[3], temperature[4], temperature[5]);
			*/		
		hlEndFrame();
		///////////////////////////////////////////////////////////////////////////////
		///////////////////////// END HAPTIC RENDER PASS //////////////////////////////
		///////////////////////////////////////////////////////////////////////////////
		

		//////////////////////////////////////////////////////////////////////////////
		////////////////// Check for events on the Client thread /////////////////////
		//////////////////////////////////////////////////////////////////////////////
		//Have to be outside of Frame. (14-2 Api referece)	
		hlCheckEvents();

		//////////////////////////////////////////////////////////////////////////////
		///////////////////   Sending data takes from phantom.   /////////////////////
		//////////////////////////////////////////////////////////////////////////////
		// Put the OpenGL state back where it was.
		vrpn_gettimeofday(&current_time, NULL);
		if (duration(current_time, timestamp) >= 1000000.0 / update_rate)
		{
			//update the time
			timestamp.tv_sec = current_time.tv_sec;
			timestamp.tv_usec = current_time.tv_usec;
			
			//****************************************
			//******** SEND vrpn_Tracker *************
			//****************************************   
				//Check: if the position have changed, we will send it.
				if (phantomVariable.proxyOnMotionTracker){				
					//Encode and send the message about the position and the orientation of the Tracker
					sendProxyPosOrient();
					////We are not going to send it again until the tracker is moved.
					phantomVariable.proxyOnMotionTracker = false;
				}
			//****************************************
			//******** SEND vrpn_button **************
			//**************************************** 
				//Check: if the buttons state have changed, we will send them.
				if ((lastData.button0 != button_State[0]) || (lastData.button1 != button_State[1])){
					lastData.button0 = button_State[0];
					lastData.button1 = button_State[1];
					//Send buttons state
					sendButtons();
				}				
			//****************************************
			//******** SEND vrpn_ForceDevice *********
			//***************************************** 
				//Send IsTouching - Send always
					//Encode the information about is an object is being touched in that moment
					sendIsTouching();
				//Send Touched Object - Will send if have change the object that is touched
					//Encode the identifier of the object which is touched in that moment. If no one is touched we send -1			
					//This value is get by the CALLBACK: ObjectIsTouched and ObjectIsUnTouched
					//Check: if have change the object that is touched.
					if (lastData.last_objectNumber != objTouchingNumber){
						lastData.last_objectNumber = objTouchingNumber;
						sendTouchedObject();
					}
				//Send Reaction Force 
					//If there are contact, the force has already been sent. But if not, we have to send a force of 0.
					if (!phantomVariable.contactState)					
						sendForce(-1);																	
				//Encode and Send DOP (depth of penetration). If there are no contact DOP is 0
					if (!phantomVariable.contactState)
						phantomVariable.dop = 0;					
					sendDOP();				
				//Encode and Send the SCP (surface contact point) and 
				//Encode and Send the angular information between ProxyDirVector and NormalProxy.
					if (phantomVariable.proxyOnMotionForceDevice){
						sendSCP();						
						checkAngle();
						sendAngle();						
						phantomVariable.proxyOnMotionForceDevice = false;
					}			
		}
	}
}//END Haptic Render


//******************************************************************************************
//Functions to manage the haptic device.</summary>
//******************************************************************************************

/////////////////////////////////////////////
/// <summary> Reset haptic device.</summary>
/////////////////////////////////////////////
bool vrpn_OpenHaptics::resetDevice(void)
{
	if(resetReady)
	{
		resetReady = false;

		printf("\n\nReset Haptic Scene in process...");
		
		//Stop Vibration effect.  
		stopVibration(); 		
		
		//Delete HL Shapes and Context		
		for (unsigned int i = 0; i<object.size(); i++)
			hlDeleteShapes(object[i].objId, 1);
		
		//Stop and Delete HL effects		
		for(unsigned int j=0;j<effect.size();j++)
		{
			hlStopEffect(effect[j].effectId);
			hlDeleteEffects(effect[j].effectId, 1);
		}
		
		//Clear the lists of objects and effects
		for (unsigned int i = 0; i<object.size(); i++)
			object[i].objVertex.clear();
		object.clear();
		effect.clear();

		//Clear Haptic context.
		hlMakeCurrent(NULL);
		hlDeleteContext(hHLRC);

		//************************************************************************
		//**********************    Create one empty object  *********************
		//************************************************************************
		Object temp;
		//Set the object touchable by default
		temp.isTouchable = true;
		//Init Haptic Properties
		temp.objEffect.stiffness = 0;
		temp.objEffect.damping = 0;
		temp.objEffect.staticf = 0;
		temp.objEffect.dynamicf = 0;
		temp.objEffect.popthrough = 0;
		temp.objEffect.mass = 0;

		//Add the empty object to the struct	
		object.push_back(temp);

		//*************************************************************************
		//************************ Init Variable Section **************************
		//*************************************************************************		
		
		//Variable to save the number of objects that there are, the value of it variable is recieved with setObjectNumber
		NumberOfObjects = 1;
		//Variable to count the number of effects that there are, it increases each time the client calls setEffect
		NumberOfEffects = 0;
		//Variable to count the number of vertexs
		NumberOfVertex = 0;
		//None object touched
		objTouchingNumber = -1;
		//phantomDeviceInitialization It's true when the device is initialize correctly.
		phantomDeviceInitialization = false;
		//proxyOnMotionTracker It's true when the proxy is on move.
		phantomVariable.proxyOnMotionTracker = true;
		//proxyOnMotionFD It's true when the proxy is on move.
		phantomVariable.proxyOnMotionForceDevice = false;				
		// Boolean that indicates whether an inertia force has been activated or not
		inertiaBoolean = 0;
		//initialize last report dop to 0
		lastData.last_dop = 0;

		//Hannilliate Vibration Components
		phantomVariable.effectVibMotorIndex = -1;
		phantomVariable.effectVibContactIndex = -1;
		
		//Initialize haptic rendering library.
		initHL(phantom);
		phantomDeviceInitialization = true;
		//Since now the reset its available again.
		resetReady = true;
				
	}
	return true;
}



//************************************************************************************
//***********************   Render Functions  ****************************************
//************************************************************************************

///*******************************************************************************
/// <summary>Setup/initialize haptic rendering library.</summary>
///*******************************************************************************/
void vrpn_OpenHaptics::initHL(HHD phantom)
{
	// Matrixs to manage the grafic scene
	double matrixUnit[16];	
	//ManuVAR 6/10/2010 - Set Modelview & Projection Matrix to Unity
	for (int i = 0; i<16; i++)
	{
		matrixUnit[i] = 0;
		if (i % 5 == 0)
			matrixUnit[i] = 1;
	}
	//Set the workspace with the Projection Matrix
	setWorkspaceProjectionMatrix(matrixUnit, matrixUnit);
	
	/*Create a haptic context for the device. The haptic context maintains the state that persists
	between frame intervals and is used forhaptic rendering*/
	hHLRC = hlCreateContext(phantom);
	hlMakeCurrent(hHLRC);

	printf("\n\nPhantom UMA initialized successfully!!!");

	/*Enable the haptic camera view, in this case the HLAPI will automatically adjust the OpenGL viewing
	parameters based on the motion and mapping of the haptic device in the scene*/

	//Enable optimization of the viewing parameters when rendering geometry for OpenHaptics.
	hlEnable(HL_HAPTIC_CAMERA_VIEW);


	// Get IDs for the empty object that was create in the constructor.
	object[0].objId = hlGenShapes(1);

	//Link a callback with the empy object. 
	//This callback is going to return if the object is touching.
	//Pass all the class as userdata.
	hlAddEventCallback(HL_EVENT_TOUCH,
		object[0].objId,
		HL_COLLISION_THREAD,
		static_cast<HLeventProc>(ObjectIsTouched),
		static_cast<void *>(this));
	
	//Create a callback that is going to return if we leave of touch a object.
	//This one is link with all the objects
	hlAddEventCallback(HL_EVENT_UNTOUCH,
		HL_OBJECT_ANY,
		HL_COLLISION_THREAD,
		static_cast<HLeventProc>(ObjectIsUnTouched),
		static_cast<void *>(this));

	//Create a callback that is going to return if the proxy is on move.			
	//Callback will be called when either the proxy has moved more than the
	//HL_EVENT_MOTION_LINEAR_TOLERANCE millimeters in workspace coordinates
	//from the position when the last motion event was triggered or when the proxy has been
	//rotated more than HL_EVENT_MOTION_ANGULAR_TOLERANCE radians from the
	//rotation of the proxy last time a motion event was triggered.	
	
	hlEventd(HL_EVENT_MOTION_LINEAR_TOLERANCE, eventMotionLinearTolerance);
	hlEventd(HL_EVENT_MOTION_ANGULAR_TOLERANCE, eventMotionAngularTolerance);

	hlAddEventCallback(HL_EVENT_MOTION,
		HL_OBJECT_ANY,
		HL_COLLISION_THREAD,
		static_cast<HLeventProc>(ProxyOnMotion),
		static_cast<void *>(this));

	

	//Map workspace into physical Phantom Workspace, a box 160mm in X, 150 mm in Y, and 60 mm in Z
	hlWorkspace(-80, -80, -70, 80, 80, 20);
	
}

///************************************************************************************
///<summary> Objects rendering.</summary>
///************************************************************************************
void vrpn_OpenHaptics::renderHL(int objNum)
{
    hlTouchModel(HL_CONTACT);
	
	//Depth buffer shapes use the OpenGL depth buffer to capture shape geometry. While the
	//feedback buffer shape stores points, line segments and polygons to use for haptic
	//rendering.

	//For large numbers of primitives, depth buffer shapes are more efficient and use less
	//memory since the haptic rendering engine only needs to use the depth image for haptic
	//rendering. The complexity of haptic rendering on a depth image is independent of the
	//number of primitives used to generate the image. At the same time, for small numbers of
	//primitives, feedback buffer shapes are more efficient and use less memory due to the
	//overhead required to generate and store the depth image.

	//Memory allocation: It is therefore better to over allocate than it is
	//to under allocate.

	//if(object[objNum].objVertex.counter < 65535)
	if (object[objNum].objVertex.size() < 65535)
	{
		//When using the feedback buffer shapes, you should use the hlHinti() command with
		//HL_SHAPE_FEEDBACK_BUFFER_VERTICES to tell the API the number of vertices
		//that will be rendered.		
		//hlHinti(HL_SHAPE_FEEDBACK_BUFFER_VERTICES,object[objNum].objVertex.counter);
		hlHinti(HL_SHAPE_FEEDBACK_BUFFER_VERTICES, object[objNum].objVertex.size());
		
		//Feedback buffer shapes use the OpenGL feedback buffer to capture geometric primitives
		//for haptic rendering. HLAPI automatically allocates a feedback buffer and sets the OpenGL
		//rendering mode to feedback mode. When in feedback buffer mode, rather than rendering
		//geometry to the screen, the geometric primitives that would be rendered are saved into the
		//feedback buffer. All OpenGL commands that generate points, lines and polygons will be
		//captured.
		hlBeginShape(HL_SHAPE_FEEDBACK_BUFFER, object[objNum].objId);
	}		
	else
	{
		hlBeginShape(HL_SHAPE_DEPTH_BUFFER, object[objNum].objId);
		// haptic rendering requires a clear depth buffer
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	//Haptic propierties rendering
	hlMaterialf(HL_FRONT_AND_BACK, HL_STIFFNESS, object[objNum].objEffect.stiffness);
	hlMaterialf(HL_FRONT_AND_BACK, HL_DAMPING, object[objNum].objEffect.damping);
	hlMaterialf(HL_FRONT_AND_BACK, HL_STATIC_FRICTION,object[objNum].objEffect.staticf);
	hlMaterialf(HL_FRONT_AND_BACK, HL_DYNAMIC_FRICTION, object[objNum].objEffect.dynamicf);
	hlMaterialf(HL_FRONT_AND_BACK, HL_POPTHROUGH, object[objNum].objEffect.popthrough);

	glPushMatrix();	
	glMultMatrixd(object[objNum].transform);

	glBegin(GL_TRIANGLES);
		
		for (unsigned int i = 0; i < object[objNum].objVertex.size(); i++)
		{  			
			glVertex3d(object[objNum].objVertex[i].x, object[objNum].objVertex[i].y, object[objNum].objVertex[i].z);
		} 

	glEnd();

   	glPopMatrix();
    
    // End the haptic shape.
    hlEndShape();
}
///************************************************************************************
///<summary>Effects rendering.</summary>
///************************************************************************************
void vrpn_OpenHaptics::renderHLeffect(int effectNum)
{
	effect[effectNum].flagStateChanged = false;

	if(effect[effectNum].effect_active){		
		hlEffectd(HL_EFFECT_PROPERTY_MAGNITUDE, effect[effectNum].magnitude);
		hlEffectd(HL_EFFECT_PROPERTY_FREQUENCY, effect[effectNum].frequency);  // Not defined yet
		hlEffectd(HL_EFFECT_PROPERTY_DURATION,  effect[effectNum].duration);

		if(strcmp(effect[effectNum].type,"constant")==0)
		{
			hlEffectdv(HL_EFFECT_PROPERTY_DIRECTION, effect[effectNum].direction);
			if(effect[effectNum].duration!=0){
				hlTriggerEffect(HL_EFFECT_CONSTANT); 
				effect[effectNum].effect_active=false;
			}
			else
				hlStartEffect(HL_EFFECT_CONSTANT, effect[effectNum].effectId);
		}

		if(strcmp(effect[effectNum].type,"spring")==0)
		{
			hlEffectd(HL_EFFECT_PROPERTY_GAIN, effect[effectNum].gain);
			hlEffectdv(HL_EFFECT_PROPERTY_POSITION, effect[effectNum].position);
			if(effect[effectNum].duration!=0){
				hlTriggerEffect(HL_EFFECT_SPRING); 
				effect[effectNum].effect_active=false;
			}
			else
				hlStartEffect(HL_EFFECT_SPRING, effect[effectNum].effectId);
		}

		if(strcmp(effect[effectNum].type,"viscous")==0)
		{
			hlEffectd(HL_EFFECT_PROPERTY_GAIN, effect[effectNum].gain);
			if(effect[effectNum].duration!=0){
				hlTriggerEffect(HL_EFFECT_VISCOUS); 
				effect[effectNum].effect_active=false;
			}
			else
				hlStartEffect(HL_EFFECT_VISCOUS, effect[effectNum].effectId);
		}

		if(strcmp(effect[effectNum].type,"friction")==0)
		{
			hlEffectd(HL_EFFECT_PROPERTY_GAIN, effect[effectNum].gain);
			if(effect[effectNum].duration!=0){
				hlTriggerEffect(HL_EFFECT_FRICTION); 
				effect[effectNum].effect_active=false;
			}
			else
				hlStartEffect(HL_EFFECT_FRICTION, effect[effectNum].effectId);
		}
	}

	//if(!effect[effectNum].effect_active)
	else
	{		
		hlStopEffect(effect[effectNum].effectId);
	}
}

//************************************************************************************
//***************************  OtherFunctions  ***************************************
//************************************************************************************

///***********************************************************************************************************
///<summary>Calculate the angle between proxy and normal vector to the surface in the contact point.</summary>
///***********************************************************************************************************
void vrpn_OpenHaptics::checkAngle(void)
{	
	//Angle Calculation
	hduVector3Dd ProxyVector;
	//For straight tool - 21.1.11
	hduVector3Dd ProxyDirVector;
	//For angle tool - 21.1.11
	hduVector3Dd ProxyUpVector;
	hduMatrix proxyM;

	if (phantomVariable.contactState){		
		proxyM = hduMatrix(proxyxform);		
		
		ProxyUpVector.set(-1 * proxyM[1][0], -1 * proxyM[1][1], -1 * proxyM[1][2]);		

		//Reset Orientation	
		hduVecScale(ProxyVector, ProxyUpVector, -1);//,ProxyDirVector,-1);
		
		//Angle= arcos(dotProduct(V1,V2)/(Magnitude(V1)*Magnitide(V2)))
		phantomVariable.angle = acos(hduVecDotProduct(ProxyVector, phantomVariable.normalProxy) / (hduVecMagnitude(ProxyVector)* hduVecMagnitude(phantomVariable.normalProxy)));		

		//Convert to degree
		phantomVariable.angleInDegree = 180 * phantomVariable.angle / M_PI;
	}
	
	if (!phantomVariable.contactState)
		phantomVariable.angleInDegree = 0.0;
	//printf("Angle in degree %f\n",phantomVariable.angleInDegree );
}

///----------------------------------------------------------------------------------------------------
///------------------------------ Send data from Server to Client -------------------------------------
///----------------------------------------------------------------------------------------------------

///*******************************************************************************
///<summary>Function to compare parameters read from an analog device.</summary>
///*******************************************************************************
bool vrpn_OpenHaptics::AreSame(double a, double b, double EPSILON){
	return fabs(a - b) < EPSILON;

}//END AreSame

///**********************************************************************************************************
///<summary> Function to send Proxy position and orientation data from the server. Use VRPN_TRACKER</summary>
///**********************************************************************************************************
void vrpn_OpenHaptics::sendProxyPosOrient(void)
{
	char    msgbuf[1000];
    vrpn_int32	len;
	
	//Copy Proxy Position and orientation from our structure to Vprn_Tracker Structure
	for (int i = 0; i < 3; i++)
	{
		//Convert Proxy Position coordinates from mm to meters;	
		pos[i] = phantomVariable.proxyPos[i] / 1000.0;
		d_quat[i] = phantomVariable.proxyRot[i];
	}
	d_quat[3] = phantomVariable.proxyRot[3];

	//Sending Tracker	
	if(vrpn_Tracker::d_connection) {
		len = vrpn_Tracker::encode_to(msgbuf);
		if(vrpn_Tracker::d_connection->pack_message(len,
			timestamp,vrpn_Tracker::position_m_id,
			vrpn_Tracker::d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Phantom: cannot write message: tossing\n");
			}
	}	
}//END sendProxyPosOrient

///***********************************************************************************
///<summary> Function to send Buttons State from the server. Use VRPN_Button</summary>
///***********************************************************************************
void vrpn_OpenHaptics::sendButtons(void)
{		
	//Copy button state to vrpn_Button structure and send
	buttons[0] = button_State[0];
	buttons[1] = button_State[1];
				
	//Send buttons state
	vrpn_Button_Filter::report_changes();
}

///*****************************************************************************
///<summary> Function to send the force applied over a touched object.</summary>
///*****************************************************************************
void vrpn_OpenHaptics::sendForce(int objNum)
{
	vrpn_int32	len;
	char    *buf;
	hduVector3Dd temp;
	
	//Initialize the value to send
	temp.set(0, 0, 0);
	//If any object has been touched we are going to send the force value
	if (objNum != -1){
		if (_finite(object[objNum].reactionForce[0]) && _finite(object[objNum].reactionForce[1]) && _finite(object[objNum].reactionForce[2])){			
			for (int i = 0; i < 3; i++){
				temp[i] = object[objNum].reactionForce[i];
			}
		}
	}	

	//Send the force
	if ((lastData.last_ReactionForce[0] != temp[0]) || (lastData.last_ReactionForce[1] != temp[1]) || (lastData.last_ReactionForce[2] != temp[2]))
	{
		//printf("Sendforce - reactionForce: %d - %lf %lf %lf\n", objNum, object[objNum].reactionForce[0], object[objNum].reactionForce[1], object[objNum].reactionForce[2]);
		//printf("Sendforce - reactionForce: %d - %lf %lf %lf\n", objNum, temp[0], temp[1], temp[2]);
		if (vrpn_ForceDevice::d_connection) {
			buf = vrpn_ForceDevice::encode_force(len, temp);
			if (vrpn_ForceDevice::d_connection->pack_message(len, timestamp,
				force_message_id, vrpn_ForceDevice::d_sender_id,
				buf, vrpn_CONNECTION_LOW_LATENCY)) {
				fprintf(stderr, "Phantom: cannot write message: tossing\n");
			}
			delete buf;
		}
		//Update last data sent with the current value.
		for (int i = 0; i < 3; i++){			
			lastData.last_ReactionForce[i] = temp[i];
		}
	}
}

///***************************************************************************************
///<summary> Function to send the DOP (Deph of Penetration) on a touched object.</summary>
///***************************************************************************************
void vrpn_OpenHaptics::sendDOP(void)
{
	vrpn_int32	len;
	char    *buf;
	
	//If there are no contact DOP is 0
	//We check to send only when DOP has changed	
	if(!AreSame(lastData.last_dop, phantomVariable.dop, 0.00001))
	{
		//printf("%lf\n", phantomVariable.dop);
		if (vrpn_ForceDevice::d_connection) {
			buf = vrpn_ForceDevice::encode_dop(len, phantomVariable.dop);
			if (vrpn_ForceDevice::d_connection->pack_message(len, timestamp,
				dop_message_id, vrpn_ForceDevice::d_sender_id,
				buf, vrpn_CONNECTION_LOW_LATENCY)) {
					fprintf(stderr,"Phantom: cannot write message: tossing\n");
			}
			delete buf;
		}
		//Update last data sent with the current value.
		lastData.last_dop = phantomVariable.dop;
	}
}
///*****************************************************************************************
///<summary> Function to send the SCP (Surface Contact Point) on a touched object.</summary>
///*****************************************************************************************
void vrpn_OpenHaptics::sendSCP(void)
{
	vrpn_int32	len;
	char    *buf;	
	//To store the SCP vectors
	vrpn_float64 scpPos[3];
	vrpn_float64 scpQuat[4];

	//Check if there are contact. 
	if (!phantomVariable.contactState){
		//SCP is 0, if there are not contact
		for (int i = 0; i < 3; i++){
			scpPos[i] = 0.0;
			scpQuat[i] = 0.0;
		}
		scpQuat[3] = 0.0;
	}
	else
	{
		// SCP is the proxy position when there are contact
		//Copy Proxy position and orientation to SCP			
		for (int i = 0; i < 3; i++)
		{
			scpPos[i] = phantomVariable.proxyPos[i] / 1000.0;  //mm to meters;
			scpQuat[i] = phantomVariable.proxyRot[i];
		}
		scpQuat[3] = phantomVariable.proxyRot[3];
	}

	//Send SCP
	if( (lastData.last_scpPos[0] != scpPos[0]) || (lastData.last_scpPos[1] != scpPos[1]) || (lastData.last_scpPos[2] != scpPos[2]))
	{
		//printf("%lf, %lf, %lf\n", scpPos[0], scpPos[1], scpPos[2]);
		if (vrpn_ForceDevice::d_connection) {
			buf = vrpn_ForceDevice::encode_scp(len, scpPos, scpQuat);
			if (vrpn_ForceDevice::d_connection->pack_message(len, timestamp,
				scp_message_id, vrpn_ForceDevice::d_sender_id,
				buf, vrpn_CONNECTION_LOW_LATENCY)) {
					fprintf(stderr,"Phantom: cannot write message: tossing\n");
			}
			delete buf;
		}
		//Update last data sent with the current value.
		for (int i=0;i<3; i++)
		{
			lastData.last_scpPos[i] = scpPos[i];
			lastData.last_scpQuat[i] = scpQuat[i];
		}
		lastData.last_scpQuat[3] = scpQuat[3];
	}
}

///***********************************************************************************
///<summary> Function to send if an object is being touched right now.</summary>
///***********************************************************************************
void vrpn_OpenHaptics::sendIsTouching(void)
{
	vrpn_int32	len;
	char    *buf;	
	if(vrpn_ForceDevice::d_connection) {
		buf = vrpn_ForceDevice::encode_isTouching(len, phantomVariable.contactState);
		if(vrpn_ForceDevice::d_connection->pack_message(len,timestamp,
			isTouching_message_id,vrpn_ForceDevice::d_sender_id,
			buf,vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Phantom: cannot write message: tossing\n");
		}
		delete buf;
	}
}
///***********************************************************************************
///<summary> Function to send the id (client_id) of the touched object.</summary>
///***********************************************************************************
void vrpn_OpenHaptics::sendTouchedObject(void)
{
	vrpn_int32	len;
	char    *buf;	
	if(vrpn_ForceDevice::d_connection) {
		buf = vrpn_ForceDevice::encode_TouchedObject(len, objTouchingNumber);
		if(vrpn_ForceDevice::d_connection->pack_message(len,timestamp,
			TouchedObject_message_id,vrpn_ForceDevice::d_sender_id,
			buf,vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Phantom: cannot write message: tossing\n");
		}
		delete buf;
	}
}
///***************************************************************************************
///<summary> Function to send the angle in degrees.</summary>
///***************************************************************************************
void vrpn_OpenHaptics::sendAngle(void)
{
	vrpn_int32	len;
	char    *buf;

	if(lastData.last_angle!=phantomVariable.angleInDegree)
	{
		if(vrpn_ForceDevice::d_connection) {
			buf = vrpn_ForceDevice::encode_angle(len, phantomVariable.angleInDegree,phantomVariable.contactState);
			if(vrpn_ForceDevice::d_connection->pack_message(len,timestamp,
				angle_message_id,vrpn_ForceDevice::d_sender_id,
				buf,vrpn_CONNECTION_LOW_LATENCY)) {
				fprintf(stderr,"Phantom: cannot write message: tossing\n");
			}
			delete buf;
		}
		lastData.last_angle = phantomVariable.angleInDegree;
	}
}




///----------------------------------------------------------------------------------------------------
///------------------------------ Store data send from Client -----------------------------------------
///----------------------------------------------------------------------------------------------------

/// ******************************************************************************
/// <summary>Sets/Store the number of the objects that will be rendered.</summary>
/// ******************************************************************************
bool vrpn_OpenHaptics::setObjectNumber(vrpn_int32 num)
{
	//if(setObjNumberReady)
	//{
		if (num > NumberOfObjects)
		{
			//Modification - 27.01.11
			//hlEnable(HL_HAPTIC_CAMERA_VIEW);
			
			printf("\nNumber of objects: %i", num);
			
			//Create a template if an empty object
				Object temp;
				//Set the object touchable by default
				temp.isTouchable = true;
				//Init Haptic Properties of this temporal object
				temp.objEffect.stiffness = 0;
				temp.objEffect.damping = 0;
				temp.objEffect.staticf = 0;
				temp.objEffect.dynamicf = 0;
				temp.objEffect.popthrough = 0;
				temp.objEffect.mass = 0;

			
			//We insert a number of empty objects (NumberOfObjects -1) in the data structure.
			//and we register the callbacks that we'll need.
			for (int i = NumberOfObjects; i <num; i++)
			{								
				//Introduce the empty object in the structure				
				object.push_back(temp);
				// Get OpenHaptics ID for this object
				object[i].objId = hlGenShapes(1);				

				// Create a callback that is going to return the number of the object is touching.
				//Pass the index of the object as userdata.				
				hlAddEventCallback(HL_EVENT_TOUCH,
					object[i].objId,
					HL_COLLISION_THREAD,
					static_cast<HLeventProc>(ObjectIsTouched),
					static_cast<void *>(this));
			}
			
			//Re-set the Reset Readiness Parameter
			resetReady = true;			
			//setObjNumberReady = false;
			NumberOfObjects = num;
		}
	//}
	return true;
}//END vrpn_OpenHaptics::setObjectNumber

/// *****************************************************************************
/// <summary>Sets/Store one vertex of one object that will be rendered.</summary>
/// *****************************************************************************
//bool vrpn_OpenHaptics::setVertex (vrpn_int32 objectID, vrpn_float64 *Vertex)
bool vrpn_OpenHaptics::setVertex(vrpn_int32 objNum, vrpn_int32 vertNum, vrpn_float32 x, vrpn_float32 y, vrpn_float32 z)
{
	XYZ temp;
	if (objNum < vrpn_int32(object.size())){
		temp.x = x;
		temp.y = y;
		temp.z = z;
	}
			
	//Insert the vertex in the data structure
	object[objNum].objVertex.push_back(temp);
	
	//Show the vertex counter	
	if (NumberOfVertex == 0)
		std::printf("\n");
	NumberOfVertex++;
	std::printf("\rNumber of Vertexs: %d", NumberOfVertex);	
	
	return true;
}

/// **************************************************************************************
/// <summary>Sets/Store the transform matrix of one object that will be rendered.</summary>
/// **************************************************************************************
bool vrpn_OpenHaptics::setTransformMatrix(vrpn_int32 objectID, vrpn_float64 *transformMatrix)
{
		for (int j =0;j<16;j++)
		{
			object[objectID].transform[j] = transformMatrix[j];
			//printf("transformMatrix[%d] %lf\n", j, transformMatrix[j]);
		}
		//std::printf("\nTransform Matrix received.");
	return true;
}

/// *****************************************************************************************
/// <summary>Set if one object will be rendered.</summary>
/// *****************************************************************************************
bool vrpn_OpenHaptics::setObjectIsTouchable(vrpn_int32 objectID, vrpn_bool isTouchable)
{
	if (objectID < vrpn_int32(object.size()))
	{
		if (isTouchable){
			object[objectID].isTouchable = true;
		}
		else
		{
			object[objectID].isTouchable = false;
		}
	}
	return true;
}
/// *****************************************************************************************
/// <summary>Set/Store the Haptics propierties of one object that will be rendered.</summary>
/// *****************************************************************************************
bool vrpn_OpenHaptics::setHapticProperty(vrpn_int32 objectID, char *type, vrpn_float32 value)
{
	

	if (strcmp(type, "stiffness") == 0)
	{
		object[objectID].objEffect.stiffness = value;
		//printf(" - stiffness %f\n",object[objNum].objEffect.stiffness);
	}
	else if (strcmp(type, "damping") == 0)
	{
		object[objectID].objEffect.damping = value;
		//printf(" - damping %f\n", object[objNum].objEffect.damping);
	}
	else if (strcmp(type, "static friction") == 0)
	{
		object[objectID].objEffect.staticf = value;
		//printf(" - static friction %f\n",object[objNum].objEffect.staticf);
	}
	else if (strcmp(type, "dynamic friction") == 0)
	{
		object[objectID].objEffect.dynamicf = value;
	}
	else if (strcmp(type, "popthrough") == 0)
	{
		object[objectID].objEffect.popthrough = value;
	}
	else if (strcmp(type, "mass") == 0)
	{
		object[objectID].objEffect.mass = value;
	}	
	return true;
}

/// ***************************************************************
/// <summary>Sets/Store one effect that will be rendered.</summary>
/// ***************************************************************
bool vrpn_OpenHaptics::setEffect(char *effect_name, vrpn_int32 effect_index, vrpn_float64 gain, vrpn_float64 magnitude, vrpn_float64 duration, vrpn_float64 frequency, vrpn_float64 position[3], vrpn_float64 direction[3])
{
	

	Effects temp;
	// Get IDs from OpenHaptics to this effect
	temp.effectId = hlGenEffects(1);
	//Copy the name
	for (int i = 0; i<20; i++)
		temp.type[i] = effect_name[i];
	temp.effect_index = effect_index;
	temp.gain = gain;
	temp.magnitude = magnitude;
	temp.duration = duration;
	temp.frequency = frequency;
	temp.effect_active = false;
	temp.flagStateChanged = false;

	for(int i = 0; i<3; i++){
		temp.position[i] = position[i];
		temp.direction[i] = direction[i];
	}
	//We insert the effect on the structure
	effect.push_back(temp);	
	
	// Show the vertex counter	
	if (NumberOfEffects == 0)
		std::printf("\n");
	NumberOfEffects++;
	std::printf("\rNumber of Effects: %d", NumberOfEffects);

	return true;
}
/// ***************************************************************************
/// <summary>Start the vibration when this effect has been activated.</summary>
/// ***************************************************************************
void vrpn_OpenHaptics::startVibration(){
	//Switch ON the vibration and tangecial force.
	//First we check if is defined the effect by the client.
	if (phantomVariable.effectVibMotorIndex != -1 || phantomVariable.effectVibContactIndex != -1){
		//Second we check if whe have to start or to stop.		
		if (activeComputeforceCB == false){
			//We program a callback that manage the vibration and tangencial force.
			callbackHandle = hdScheduleAsynchronous(hdComputeforceCB,
				static_cast<void *>(this),
				HD_DEFAULT_SCHEDULER_PRIORITY);
			activeComputeforceCB = true;
			printf("\nStart Vibration");
		}
		else{
		}
	}
	else{
	}
}//startVibration

/// ****************************************************************************
/// <summary>Stop the vibration when this effect has been deactivated.</summary>
/// ****************************************************************************
void vrpn_OpenHaptics::stopVibration(){
	//SwitchOFF the vibration and tangecial force.
	//First we check if is defined the effect by the client.
	if (phantomVariable.effectVibMotorIndex != -1 || phantomVariable.effectVibContactIndex != -1){

		//Second we check if whe have to start or to stop.		
		if (activeComputeforceCB == true){
			//We program-off the callback of Vibration and tangencial force.
			hdUnschedule(callbackHandle);
			activeComputeforceCB = false;
			printf("\nStop Vibration");
		}//END If Vibration & TgForce
	}
}//stopVibration

/// ******************************************************
/// <summary>Sets the action to start an effect.</summary>
/// ******************************************************
bool vrpn_OpenHaptics::startEffect(vrpn_int32 effect_index)
{
	unsigned int i = 0;
	//Seek the effectID on the list
	if (i<effect.size()){

		while (i<effect.size() && effect[i].effect_index != effect_index)
			i++;
		if (i<effect.size() && effect[i].effect_index == effect_index){
			//Check if the effect is DesActivate
			if (!effect[i].effect_active){
				//Activate the effect
				effect[i].effect_active = true;
				effect[i].flagStateChanged = true;
				printf("\nStart Effect : %i", effect_index);
				//Check if is a vibration effect
				if (strcmp(effect[i].type, "vibrationMotor") == 0){
					phantomVariable.effectVibMotorIndex = i;
					startVibration();
				}
				if (strcmp(effect[effect_index].type, "vibrationContact") == 0){
					phantomVariable.effectVibContactIndex = i;
					startVibration();
				}				
			}
		}
	}		
	//printf("Start Effect phantomVariable.effectVibMotorIndex %d\n", phantomVariable.effectVibMotorIndex);

	return true;
}

/// ******************************************************
/// <summary>Sets the action to stop an effect.</summary>
/// ******************************************************
bool vrpn_OpenHaptics::stopEffect(vrpn_int32 effect_index)
{
	unsigned int i = 0;
	if (i<effect.size()){
		//Seek the effectID on the list
		while (i<effect.size() && effect[i].effect_index != effect_index)
			i++;
		if (i<effect.size() && effect[i].effect_index == effect_index){
			//Check if the effect is activate
			if (effect[i].effect_active){
				//DesActivate the effect
				effect[i].effect_active = false;
				effect[i].flagStateChanged = true;
				printf("\nStop Effect : %i", effect_index);
				//Check if is a vibration effect
				if (strcmp(effect[i].type, "vibrationMotor") == 0){
					stopVibration();
					phantomVariable.effectVibMotorIndex = -1;
				}
				if (strcmp(effect[effect_index].type, "vibrationContact") == 0){
					stopVibration();
					phantomVariable.effectVibContactIndex = -1;
				}
			}
		}		
	}
	
	//printf("Stop Effect phantomVariable.effectVibMotorIndex %d\n", phantomVariable.effectVibMotorIndex);
	return true;
}	

/// ***********************************************************************
/// <summary>Sets/Store the Touchable Face that will be rendered.</summary>
/// ***********************************************************************
bool vrpn_OpenHaptics::setTouchableFace(vrpn_int32 TFace)
{
	TouchableFace = TFace;
	//printf("setTouchable\n");
	if (TFace == 0)							//Front face
	  hlTouchableFace(HL_FRONT); 
	else if (TFace == 1)					//Back Face
	  hlTouchableFace(HL_BACK); 
	else									//Front and Back Face 
       hlTouchableFace(HL_FRONT_AND_BACK);	
	return true;
}

/// ***********************************************************************
/// <summary>Sets/Store the Workspace that will be rendered.</summary>
/// <param name="modelMatrix"> Modelmatrix, size has to be 16.</param>
/// <param name="matrix"> Projection Matrix (size has to be 16).</param>
/// ***********************************************************************
bool vrpn_OpenHaptics::setWorkspaceProjectionMatrix(vrpn_float64 *modelMatrix, vrpn_float64 *matrix)
{	
	//Set the Modelview Matrix
		//printf("Set modelview Matrix\n");	
		for(int i=0;i<16;i++)
		{
			wsParameters.modelviewMatrix[i] = modelMatrix[i];
		}		
		//Modification - 26.06.11
		//HL_MODELVIEW matrix is only applied when not using the OpenGL modelview (when
		//HL_USE_GL_MODELVIEW is disabled). When the OpenGL modelview is
		//used, you should use OpenGL functions to query the modelview matrix.
		//in that case we use the GL_ModelView matrix. We fill GL_modelview with the OpenGL client 
		//ModelView Matrix.		
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixd(wsParameters.modelviewMatrix);

	//Set the Workspace with the Projection Matrix
		
		//printf("\nWorkSpace set by Projection Matrix");
		for(int i=0; i<16;i++)
		{	
			wsParameters.projectionMatrix[i] = matrix[i];
			//printf("projection: %lf\n", wsParameters.projectionMatrix[i]);
		}

		hlMatrixMode(HL_TOUCHWORKSPACE);
		hlLoadIdentity();
		
		// Fit haptic workspace to view volume.
		hluFitWorkspace(wsParameters.projectionMatrix);	
	return true;
}

/// ***********************************************************************
/// <summary>Sets/Store the Workspace that will be rendered.</summary>
/// <param name="modelMatrix"> Modelmatrix, size has to be 16.</param>
/// <param name="matrix">  Bounding Box coordinates (size has to be 6).</param>
/// ***********************************************************************
bool vrpn_OpenHaptics::setWorkspaceBoundingBox(vrpn_float64 *modelMatrix, vrpn_float64 *matrix)
{
	//Set the Modelview Matrix
	//printf("Set modelview Matrix\n");	
	for (int i = 0; i<16; i++)
	{
		wsParameters.modelviewMatrix[i] = modelMatrix[i];
	}
	//Modification - 26.06.11
	//HL_MODELVIEW matrix is only applied when not using the OpenGL modelview (when
	//HL_USE_GL_MODELVIEW is disabled). When the OpenGL modelview is
	//used, you should use OpenGL functions to query the modelview matrix.
	//in that case we use the GL_ModelView matrix. We fill GL_modelview with the OpenGL client 
	//ModelView Matrix.		
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(wsParameters.modelviewMatrix);

	//printf("\nWorkSpace set by Bounding Box.");	
	for (int i = 0; i<3; i++)
	{
		wsParameters.minPoint[i] = matrix[i];
		wsParameters.maxPoint[i] = matrix[i + 3];
	}
	// Fit haptic workspace to view the boundingbox.
	hluFitWorkspaceBox(wsParameters.modelviewMatrix, wsParameters.minPoint, wsParameters.maxPoint);
	
	return true;
}

///----------------------------------------------------------------------------------------------
///-------------------------------------- Callbacks ---------------------------------------------
///----------------------------------------------------------------------------------------------

//***********************************************************************************************
//Event callbacks triggered when an object is being touched and give the identifier of the object
//***********************************************************************************************
void HLCALLBACK vrpn_OpenHaptics::ObjectIsTouched(HLenum event, HLuint objectId, HLenum thread,
	HLcache *cache, void *userdata)
{	
	vrpn_OpenHaptics* myObj = static_cast<vrpn_OpenHaptics*>(userdata);


	//Taking the openhaptics ID of the object that is touched. 
	//Later we have to convert it to our object number ID.
	//Have to send the object number, not the openhaptics id.
	//So we find that, is the index on the array.
	unsigned int i = 0;
	while ((i<myObj->object.size()) && (myObj->object[i].objId != objectId)){
		i++;
	};
	myObj->objTouchingNumber = i;
}

void HLCALLBACK vrpn_OpenHaptics::ObjectIsUnTouched(HLenum event, HLuint object, HLenum thread,
	HLcache *cache, void *userdata)
{
	
	vrpn_OpenHaptics* myObj = static_cast<vrpn_OpenHaptics*>(userdata);
	myObj->objTouchingNumber = -1;
}

/******************************************************************************
Event callback triggered when an the proxy in on motion
******************************************************************************/
void HLCALLBACK vrpn_OpenHaptics::ProxyOnMotion(HLenum event, HLuint objectId, HLenum thread,
	HLcache *cache, void *userdata){
	vrpn_OpenHaptics* myObj = static_cast<vrpn_OpenHaptics*>(userdata);
	
	myObj->phantomVariable.proxyOnMotionTracker = true;
	myObj->phantomVariable.proxyOnMotionForceDevice = true;
}//END Callback onMotion

//
////***********************************************************************************************
////HDCALLBACKS for vibration and tangencial force
////***********************************************************************************************
HDCallbackCode HDCALLBACK vrpn_OpenHaptics::hdComputeforceCB(void *pUserData){
	vrpn_OpenHaptics* myObj = static_cast<vrpn_OpenHaptics*>(pUserData);

	hdBeginFrame(hdGetCurrentDevice());

	HDdouble instRate;
	static HDdouble timer = 0;


	HDErrorInfo error;
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		hduPrintError(stderr, &error, "Error in hdComputeforceCB\n");
		return HD_CALLBACK_DONE;
	}


	HDdouble proxyTransform[16];
	hduMatrix proxyMatrix;

	//Get the Proxy transformation matrix
	hdGetDoublev(HD_CURRENT_TRANSFORM, proxyTransform);
	proxyMatrix = hduMatrix(proxyTransform);

	

	//**VIBRATION**//***********************************************************************************************************//
	//Total Vibration Force
	hduVector3Dd vibForce;

	//VIBRATION OF TOOL OF(MOTOR)
	HDdouble VibrationFreq; //= 40; /* Hz */
	HDdouble VibrationAmplitude;//= 0.8; /* N */

	HDdouble contactVibFreq;// = 60; /* Hz */
	HDdouble contactVibAmplitude;// = 0.5; /* N */

	//Save the variable in a local parameters. 
	//To avoid MultiThreads problems.
	int local_effectVibMotorIndex = myObj->phantomVariable.effectVibMotorIndex;

	//Check if Motor vibration is active
	if (local_effectVibMotorIndex != -1){
		if (myObj->effect[local_effectVibMotorIndex].effect_active){

			VibrationFreq = myObj->effect[local_effectVibMotorIndex].frequency; /* Hz */
			VibrationAmplitude = myObj->effect[local_effectVibMotorIndex].magnitude; /* N */

			/*VIBRATION OF TOOL ON X AXE(MOTOR)*/
			myObj->phantomVariable.VibrationDirectionX[0] = 1;
			myObj->phantomVariable.VibrationDirectionX[1] = 0;
			myObj->phantomVariable.VibrationDirectionX[2] = 0;
			//Switch from global y direction to local proxy direction
			proxyMatrix.multDirMatrix(myObj->phantomVariable.VibrationDirectionX, myObj->phantomVariable.XproxyVibDirection);

			/*VIBRATION OF TOOL ON Y AXE(MOTOR)*/
			myObj->phantomVariable.VibrationDirectionY[0] = 0;
			myObj->phantomVariable.VibrationDirectionY[1] = 1;
			myObj->phantomVariable.VibrationDirectionY[2] = 0;
			//Switch from global y direction to local proxy direction
			proxyMatrix.multDirMatrix(myObj->phantomVariable.VibrationDirectionY, myObj->phantomVariable.YproxyVibDirection);

		}//END if Motor vibration active
	}

	//END VIBRATION OF TOOL OF(MOTOR)

	//VIBRATION OF TOOL OF(CONTACT)
	//Save the variable in a local parameters. 
	//To Avoid MultiThreads problems.
	int local_effectVibContactIndex = myObj->phantomVariable.effectVibContactIndex;

	if (local_effectVibContactIndex != -1){
		if (myObj->effect[local_effectVibContactIndex].effect_active){

			contactVibFreq = myObj->effect[local_effectVibContactIndex].frequency; /* Hz */
			contactVibAmplitude = myObj->effect[local_effectVibContactIndex].magnitude; /* N */
			myObj->phantomVariable.ZproxyContactVibDirection.set(myObj->phantomVariable.normalProxy[0],
				myObj->phantomVariable.normalProxy[1],
				myObj->phantomVariable.normalProxy[2]);

			//No need to swich to global coords considering that Vibs are to apply
			//on Normal vector axis onto Phantom
			//Switch from global y direction to local proxy direction
			//proxyMatrix.multDirMatrix(contactVibDirection, ZproxyContactVibDirection);
		}
	}
	//END VIBRATION OF TOOL OF(CONTACT)
	//END VIBRATION

	
	//**COMPUTE VIBRATIONL FORCES************************************************************************************

	/* Use the reciprocal of the instantaneous rate as a timer. */
	hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	timer += 1.0 / instRate;

	//Vibration of tool Motor
	/* Apply a sinusoidal force in the direction of motion. */
	hduVecScale(myObj->phantomVariable.motorVibForceX, myObj->phantomVariable.XproxyVibDirection, VibrationAmplitude * cos(2 * M_PI*timer * VibrationFreq));
	hduVecScale(myObj->phantomVariable.motorVibForceY, myObj->phantomVariable.YproxyVibDirection, VibrationAmplitude * sin(2 * M_PI*timer * VibrationFreq));
	hduVecAdd(myObj->phantomVariable.motorVibForce, myObj->phantomVariable.motorVibForceX, myObj->phantomVariable.motorVibForceY);

	//Vibration of tool at contact
	/* Apply a sinusoidal force in the direction of motion. */
	if (myObj->phantomVariable.dop != 0)
		hduVecScale(myObj->phantomVariable.contactVibForce, myObj->phantomVariable.ZproxyContactVibDirection, contactVibAmplitude * sin(timer * contactVibFreq));

	//Combine all force models
	hduVecAdd(vibForce, myObj->phantomVariable.contactVibForce, myObj->phantomVariable.motorVibForce);

	//Set up Force model in the haptic context
	hdSetDoublev(HD_CURRENT_FORCE, vibForce);
	//printf(" -  force %lf \n", hduVecMagnitude(force));

	hdEndFrame(hdGetCurrentDevice());

	return HD_CALLBACK_CONTINUE;
}

#endif

