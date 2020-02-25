#include "vrpn_Freespace.h"
#ifdef VRPN_USE_FREESPACE
#include "quat.h"
#include <freespace/freespace_codecs.h>
#include <cstring>
static const q_type gs_qIdent = Q_ID_QUAT;

// We've not yet initialized the FreeSpace library.
bool vrpn_Freespace::_freespace_initialized = false;

// libfreespace provides linear acceleration values in (mm/(s^2))^-3 
// VRPN wants these in meters/(s^2)
static vrpn_float64 convertFreespaceLinearAccelation(int16_t mmpss) {
	return mmpss / ((vrpn_float64) 1000);
}
// freespace reports in milliradians per second, so need to convert
// to meters per second...
// The radius to use is a bit arbitrary, and would depend on 
// what exactly this device is 'attached' to.  Additionally, this may 
// be different for each axis.  In general, the radius shouldn't really
// matter since the device may be representing a larger/smaller object in
// the virtual world.
static vrpn_float64 convertFreespaceAngularVelocity(int16_t mrps, double radius) {
	return mrps * radius / ((vrpn_float64) 1000);
}
#define MILLIS_TO_SECONDS(x) (x / ((vrpn_float64) 1000000))


vrpn_Freespace::vrpn_Freespace(FreespaceDeviceId freespaceId,
        struct FreespaceDeviceInfo* deviceInfo,
        const char *name, vrpn_Connection *conn):
        vrpn_Tracker_Server(name, conn),
        vrpn_Button_Filter(name, conn),
        vrpn_Dial(name, conn)
{
	memcpy(&_deviceInfo, deviceInfo, sizeof(_deviceInfo));
	// 5 buttons + a scroll wheel
	vrpn_Button::num_buttons = 5;
	vrpn_Dial::num_dials = 1;
	memset(&_lastBodyFrameTime, 0, sizeof(_lastBodyFrameTime));
	_timestamp.tv_sec = 0;
}

void vrpn_Freespace::freespaceInit() {
	if (!_freespace_initialized) {
		_freespace_initialized = true;
		int rc = freespace_init();
		if (rc != FREESPACE_SUCCESS) {
			fprintf(stderr, "vrpn_Freespace::freespaceInit: failed to init freespace lib. rc=%d\n", rc);
			_freespace_initialized = false;
		}
	}
}

void vrpn_Freespace::deviceSetConfiguration(bool send_body_frames, bool send_user_frames) {
    _sendBodyFrames = send_body_frames;
    _sendUserFrames = send_user_frames;
}

void vrpn_Freespace::deviceConfigure() {
    int rc = 0;

    // Send the data mode request
    struct freespace_message message;
    memset(&message, 0, sizeof(message));
    if (_deviceInfo.hVer == 1) {
        message.messageType = FREESPACE_MESSAGE_DATAMOTIONCONTROL;
        struct freespace_DataMotionControl * req;
        req = &message.dataMotionControl;
        if (_sendBodyFrames) { req->enableBodyMotion = 1; }
        if (_sendUserFrames) { req->enableUserPosition = 1; }
        req->inhibitPowerManager = 1;
        req->enableMouseMovement = 0;
        req->disableFreespace = 0;
    } else {
        message.messageType = FREESPACE_MESSAGE_DATAMODEREQUEST;
        struct freespace_DataModeRequest * req;
        req = &message.dataModeRequest;
        if (_sendBodyFrames) { req->enableBodyMotion = 1; }
        if (_sendUserFrames) { req->enableUserPosition = 1; }
        req->inhibitPowerManager = 1;
        req->enableMouseMovement = 0;
        req->disableFreespace = 0;
    }

    rc = freespace_sendMessage(_freespaceDevice, &message);
    if (rc != FREESPACE_SUCCESS) {
        fprintf(stderr, "vrpn_Freespace::deviceConfigure: Could not send message: %d.\n", rc);
    }
}

void vrpn_Freespace::deviceUnconfigure() {
    int rc = 0;

    // Send the data mode request
    struct freespace_message message;
    memset(&message, 0, sizeof(message));
    if (_deviceInfo.hVer == 1) {
        message.messageType = FREESPACE_MESSAGE_DATAMOTIONCONTROL;
        struct freespace_DataMotionControl * req;
        req = &message.dataMotionControl;
        req->enableBodyMotion = 0;
        req->enableUserPosition = 0;
        req->inhibitPowerManager = 0;
        req->enableMouseMovement = 1;
        req->disableFreespace = 0;
    } else {
        message.messageType = FREESPACE_MESSAGE_DATAMODEREQUEST;
        struct freespace_DataModeRequest * req;
        req = &message.dataModeRequest;
        req->enableBodyMotion = 0;
        req->enableUserPosition = 0;
        req->inhibitPowerManager = 0;
        req->enableMouseMovement = 1;
        req->disableFreespace = 0;
    }

    rc = freespace_sendMessage(_freespaceDevice, &message);
    if (rc != FREESPACE_SUCCESS) {
        fprintf(stderr, "vrpn_Freespace::deviceUnconfigure: Could not send message: %d.\n", rc);
    }
}

vrpn_Freespace* vrpn_Freespace::create(const char *name, vrpn_Connection *conn, 
	int device_index,
        bool send_body_frames, bool send_user_frames)
{
    // initialize libfreespace
    vrpn_Freespace::freespaceInit();

    const unsigned MAX_DEVICES = 100;
    FreespaceDeviceId devices[MAX_DEVICES];
    int numIds = 0;
    int rc;
    rc = freespace_getDeviceList(devices, MAX_DEVICES, &numIds);
    if ( (rc != FREESPACE_SUCCESS) || (numIds < (device_index+1)) ) {
        fprintf(stderr, "vrpn_Freespace::create: Didn't find enough devices: %d.\n", numIds);
        return NULL;
    }
    FreespaceDeviceId freespaceId = devices[device_index];

    rc = freespace_openDevice(freespaceId);
    if (rc != FREESPACE_SUCCESS) {
        fprintf(stderr, "vrpn_Freespace::create: Could not open device %d.\n", device_index);
        return NULL;
    }
    struct FreespaceDeviceInfo deviceInfo;
    rc = freespace_getDeviceInfo(freespaceId, &deviceInfo);
    if (rc != FREESPACE_SUCCESS) {
        return NULL;
    }
    rc = freespace_flush(freespaceId);
    if (rc != FREESPACE_SUCCESS) {
        fprintf(stderr, "vrpn_Freespace::create: Error flushing device: %d\n", rc);
        return NULL;
    }

    vrpn_Freespace* dev;
    try { dev = new vrpn_Freespace(freespaceId, &deviceInfo, name, conn); }
    catch (...) { return NULL; }
    dev->deviceSetConfiguration(send_body_frames, send_user_frames);
    printf("Added freespace device: %s : \n", name);
    return dev;
}

void vrpn_Freespace::mainloop(void) 
{
	server_mainloop();
    struct freespace_message s;

    /*
     * Send configuration information to the device repeatedly to force the
     * device into the correct mode.  This method ensures the correct mode 
     * even if the Freespace device was asleep during initialization or if
     * the tracker resets.
     */
    struct timeval timestamp;
    vrpn_gettimeofday(&timestamp, NULL);
    if (_timestamp.tv_sec != timestamp.tv_sec) {
        _timestamp.tv_sec = timestamp.tv_sec;
        deviceConfigure();
    }

	// Do not block, read as many messages as 
    while (FREESPACE_SUCCESS == freespace_readMessage(_freespaceDevice, &s, 0)) {
	    switch(s.messageType) {
		    case FREESPACE_MESSAGE_LINKSTATUS:
			    handleLinkStatus(s.linkStatus);
			    break;
		    case FREESPACE_MESSAGE_BODYFRAME:
			    handleBodyFrame(s.bodyFrame);
			    break;
		    case FREESPACE_MESSAGE_USERFRAME:
			    handleUserFrame(s.userFrame);
			    break;
            case FREESPACE_MESSAGE_ALWAYSONRESPONSE:
                break;
            case FREESPACE_MESSAGE_DATAMODERESPONSE:
                break;
		    default:
                send_text_message("Received an unhandled message from freespace device", timestamp, vrpn_TEXT_WARNING);
			    break;
	    }
    }
}
vrpn_Freespace::~vrpn_Freespace(void)
{
	// as part of cleanup, disable user and body frame messages, and
	// put back to just mouse movement.
	int rc = freespace_flush(_freespaceDevice);
    if (rc != FREESPACE_SUCCESS) {
        printf("freespaceInputThread: Error flushing device: %d\n", rc);
    }

    deviceUnconfigure();
	freespace_closeDevice(_freespaceDevice);
}

void vrpn_Freespace::handleBodyFrame(const struct freespace_BodyFrame& body) {
			vrpn_gettimeofday(&(vrpn_Tracker::timestamp), NULL);
	vrpn_float64 now  = vrpn_Tracker::timestamp.tv_sec + MILLIS_TO_SECONDS(vrpn_Tracker::timestamp.tv_usec);
	vrpn_float64 delta = now - _lastBodyFrameTime;
	_lastBodyFrameTime = now;

    vrpn_Tracker::acc[0] = convertFreespaceLinearAccelation(body.linearAccelX);
	vrpn_Tracker::acc[1] = convertFreespaceLinearAccelation(body.linearAccelY);
	vrpn_Tracker::acc[2] = convertFreespaceLinearAccelation(body.linearAccelZ);
	q_copy(vrpn_Tracker::acc_quat, gs_qIdent);
	vrpn_Tracker::acc_quat_dt = delta;
    vrpn_Tracker_Server::report_pose_acceleration(0, vrpn_Tracker::timestamp,
        vrpn_Tracker::acc, vrpn_Tracker::acc_quat, delta);

	vrpn_Tracker::vel[0] = convertFreespaceAngularVelocity(body.angularVelX, 1);
	vrpn_Tracker::vel[1] = convertFreespaceAngularVelocity(body.angularVelY, 1);
	vrpn_Tracker::vel[2] = convertFreespaceAngularVelocity(body.angularVelZ, 1);
	q_copy(vrpn_Tracker::vel_quat, gs_qIdent);
	vrpn_Tracker::vel_quat_dt = delta;
    vrpn_Tracker_Server::report_pose_velocity(0, vrpn_Tracker::timestamp,
        vrpn_Tracker::vel, vrpn_Tracker::vel_quat, vrpn_Tracker::vel_quat_dt);

    vrpn_Button::buttons[0] = body.button1;
	vrpn_Button::buttons[1] = body.button2;
	vrpn_Button::buttons[2] = body.button3;
	vrpn_Button::buttons[3] = body.button4;
	vrpn_Button::buttons[4] = body.button5;
	
	vrpn_Button::timestamp = vrpn_Tracker::timestamp;
	vrpn_Button::report_changes();

	// this is totally arbitrary
	// I'm not sure how exactly the 'deltaWheel' should be interpreted, but it seems
	// like it would be the number of 'clicks' while turning the scroll wheel...
	// vrpn was this as a 'fraction of a revolution' so I would assume that a return of 1 means
	// the dial was spun 360 degrees.  I'm going to pretend it takes 16 clicks to make a full
	// revolution....'
	// this values should probably set in a configuration file.
	vrpn_Dial::dials[0] = body.deltaWheel / (vrpn_float64) 16;
	vrpn_Dial::timestamp = vrpn_Tracker::timestamp;
	vrpn_Dial::report_changes();
}

void vrpn_Freespace::handleUserFrame(const struct freespace_UserFrame& user) {
			vrpn_gettimeofday(&(vrpn_Tracker::timestamp), NULL);

    // Translate the Freespace linear position into VRPN linear position.
	vrpn_Tracker::pos[0] = user.linearPosY;
	vrpn_Tracker::pos[1] = user.linearPosZ;
	vrpn_Tracker::pos[2] = user.linearPosX;
		
    // Transfer the Freespace coordinate system angular position quanternion
    // into the VRPN coordinate system.  
	vrpn_Tracker::d_quat[Q_W] = user.angularPosA;
	vrpn_Tracker::d_quat[Q_X] = user.angularPosC;
	vrpn_Tracker::d_quat[Q_Y] = -user.angularPosD;
	vrpn_Tracker::d_quat[Q_Z] = user.angularPosB;
    q_normalize(vrpn_Tracker::d_quat, vrpn_Tracker::d_quat);

    vrpn_Tracker_Server::report_pose(0, vrpn_Tracker::timestamp, vrpn_Tracker::pos, vrpn_Tracker::d_quat);

	vrpn_Button::buttons[0] = user.button1;
	vrpn_Button::buttons[1] = user.button2;
	vrpn_Button::buttons[2] = user.button3;
	vrpn_Button::buttons[3] = user.button4;
	vrpn_Button::buttons[4] = user.button5;
	
	vrpn_Button::timestamp = vrpn_Tracker::timestamp;
	vrpn_Button::report_changes();

	// this is totally arbitrary
	// I'm not sure how exactly the 'deltaWheel' should be interpreted, but it seems
	// like it would be the number of 'clicks' while turning the scroll wheel...
	// vrpn was this as a 'fraction of a revolution' so I would assume that a return of 1 means
	// the dial was spun 360 degrees.  I'm going to pretend it takes 16 clicks to make a full
	// revolution....'
	// this values should probably set in a configuration file.
	vrpn_Dial::dials[0] = user.deltaWheel / (vrpn_float64) 16;
	vrpn_Dial::timestamp = vrpn_Tracker::timestamp;
	vrpn_Dial::report_changes();
}
void vrpn_Freespace::handleLinkStatus(const struct freespace_LinkStatus& l) {
	// Could be used to send messages when the loop is powered off/unavailable.
}
#endif //VRPN_USE_FREESPACE
