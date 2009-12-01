#include "vrpn_Freespace.h"
#ifdef VRPN_USE_FREESPACE
#include "quat.h"
#include <freespace/freespace_codecs.h>
#include <cstring>
static q_type gs_qIdent = Q_ID_QUAT;
// libfreespce provides acceleration values in (mm/(s^2))^-3 
// VRPN wants these in meters/(s^2)
static vrpn_float64 convertFreespaceAccel(int16_t mmpss) {
	return mmpss / ((vrpn_float64) 1000);
}
// freespace reports in milliradians per second, so need to convert
// to meters per second...
// The radius to use is a bit arbitrary, and would depend on 
// what exactly this device is 'attached' to.  Additionally, this may 
// be different for each axis.  In general, the radius shouldn't really
// matter since the device may be representing a larger/smaller object in
// the virtual worl.
static vrpn_float64 convertFreespaceVelocity(int16_t mrps, double radius) {
	return mrps * radius / ((vrpn_float64)1000);
}
static vrpn_float64 scaleFreespaceQuat(int16_t v) {
	return v * (vrpn_float64) 1 / (1 << 14);
}
#define MILLIS_TO_SECONDS(x) (x / ((vrpn_float64)1000000))


vrpn_Freespace::vrpn_Freespace(FreespaceDeviceId freespaceId,
        struct FreespaceDeviceInfo* deviceInfo,
        const char *name, vrpn_Connection *conn):
  vrpn_Tracker_Server(name, conn),
  vrpn_Button(name, conn),
  vrpn_Dial(name, conn),
  _freespaceDevice(freespaceId)											   
{
	memcpy(&_deviceInfo, deviceInfo, sizeof(_deviceInfo));
	// 5 buttons + a scroll wheel
	vrpn_Button::num_buttons = 5;
	vrpn_Dial::num_dials = 1;
	memset(&_lastBodyFrameTime, 0, sizeof(_lastBodyFrameTime));
}

vrpn_Freespace* vrpn_Freespace::create(const char *name, vrpn_Connection *conn, 
	int device_index,
        bool send_body_frames, bool send_user_frames)
{
  // initialize libfreespace
  // XXX Should make sure this only happens once ever, I bet.
  int rc = freespace_init();
  freespace_exit();
  rc = freespace_init();
  if (rc != FREESPACE_SUCCESS) {
    fprintf(stderr, "vrpn_Freespace::create: failed to init freespace lib. rc=%d\n", rc);
    return NULL;
  }
  const unsigned MAX_DEVICES = 100;
  FreespaceDeviceId devices[MAX_DEVICES];
  int numIds = 0;
  rc = freespace_getDeviceList(devices, MAX_DEVICES, &numIds);
  if ( (rc != FREESPACE_SUCCESS) || (numIds < (device_index+1)) ) {
    fprintf(stderr, "vrpn_Freespace::create: Didn't find enough devices.\n");
    return NULL;
  }
  FreespaceDeviceId freespaceId = devices[device_index];

  struct freespace_DataMotionControl d;

  rc = freespace_openDevice(devices[device_index]);
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
    printf("vrpn_Freespace::create: Error flushing device: %d\n", rc);
    return NULL;
  }
  memset(&d, 0, sizeof(d));
  if (send_body_frames) { d.enableBodyMotion = 1; }
  if (send_user_frames) { d.enableUserPosition = 1; }
  d.inhibitPowerManager = 1;
  d.enableMouseMovement = 0;
  d.disableFreespace = 0;
  uint8_t buffer[FREESPACE_MAX_INPUT_MESSAGE_SIZE];

  rc = freespace_encodeDataMotionControl(&d, buffer, sizeof(buffer));
  if (rc > 0) {
      rc = freespace_send(freespaceId, buffer, rc);
      if (rc != FREESPACE_SUCCESS) {
          fprintf(stderr, "vrpn_Freespace::create: Could not send message: %d.\n", rc);
          return NULL;
      }
  } else {
      fprintf(stderr, "vrpn_Freespace::create: Could not encode message.\n");
      return NULL;
  }
  return new vrpn_Freespace(freespaceId, &deviceInfo, name, conn);
}

void vrpn_Freespace::mainloop(void) 
{
	server_mainloop();
#define BUFFER_SIZE 1024
	uint8_t buffer[BUFFER_SIZE];
	int bytesRead = 0;
	int rc;
	// only block for 100 milliseconds....
	rc = freespace_read(_freespaceDevice, buffer, BUFFER_SIZE, 100, &bytesRead);
	if (rc != FREESPACE_SUCCESS) {
		// no read, nothing left to do.
		return;
	}
	struct freespace_message s;
	if (freespace_decode_message(buffer, bytesRead, &s) == FREESPACE_SUCCESS) {
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
			default:
				fprintf(stderr, "got an unhandled message from freespace device %d\n", s.messageType);
				break;
		}
	}
}
vrpn_Freespace::~vrpn_Freespace(void)
{
	// as part of cleanup, disable user and body frame messages, and
	// put back to just mouse movement.
    
	struct freespace_DataMotionControl d;
	int rc = freespace_flush(_freespaceDevice);
    if (rc != FREESPACE_SUCCESS) {
        printf("freespaceInputThread: Error flushing device: %d\n", rc);
    }
	// reset the device to just send mouse events.
    memset(&d, 0, sizeof(d));
    d.enableBodyMotion = 0;
    d.enableUserPosition = 0;
    d.inhibitPowerManager = 0;
    d.enableMouseMovement = 1;
    d.disableFreespace = 0;
    uint8_t buffer[FREESPACE_MAX_INPUT_MESSAGE_SIZE];

    rc = freespace_encodeDataMotionControl(&d, buffer, sizeof(buffer));
    if (rc > 0) {
		rc = freespace_send(_freespaceDevice, buffer, rc);
        if (rc != FREESPACE_SUCCESS) {
            printf("freespaceInputThread: Could not send message: %d.\n", rc);
        }
    } else {
        printf("freespaceInputThread: Could not encode message.\n");
    }
	freespace_closeDevice(_freespaceDevice);
}

void vrpn_Freespace::handleBodyFrame(const struct freespace_BodyFrame& body) {
			vrpn_gettimeofday(&(vrpn_Tracker::timestamp), NULL);
		vrpn_float64 now  = vrpn_Tracker::timestamp.tv_sec + MILLIS_TO_SECONDS(vrpn_Tracker::timestamp.tv_usec);
		vrpn_float64 delta = now - _lastBodyFrameTime;
		_lastBodyFrameTime = now;

		vrpn_Tracker::acc[0] = convertFreespaceAccel(body.linearAccelX);
		vrpn_Tracker::acc[1] = convertFreespaceAccel(body.linearAccelY);
		vrpn_Tracker::acc[2] = convertFreespaceAccel(body.linearAccelZ);
		q_copy(vrpn_Tracker::acc_quat, gs_qIdent);
		vrpn_Tracker::acc_quat_dt = delta;
                vrpn_Tracker_Server::report_pose_acceleration(0, vrpn_Tracker::timestamp,
                  vrpn_Tracker::acc, vrpn_Tracker::acc_quat, delta);

		vrpn_Tracker::vel[0] = convertFreespaceVelocity(body.angularVelX, 1);
		vrpn_Tracker::vel[1] = convertFreespaceVelocity(body.angularVelY, 1);
		vrpn_Tracker::vel[2] = convertFreespaceVelocity(body.angularVelZ, 1);
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
		vrpn_Tracker::pos[0] = user.linearPosX;
		vrpn_Tracker::pos[1] = user.linearPosY;
		vrpn_Tracker::pos[2] = user.linearPosZ;
		
		vrpn_Tracker::d_quat[Q_W] = scaleFreespaceQuat(user.angularPosA);
		vrpn_Tracker::d_quat[Q_X] = scaleFreespaceQuat(user.angularPosB);
		vrpn_Tracker::d_quat[Q_Y] = scaleFreespaceQuat(user.angularPosC);
		vrpn_Tracker::d_quat[Q_Z] = scaleFreespaceQuat(user.angularPosD);

                vrpn_Tracker_Server::report_pose(0, vrpn_Tracker::timestamp, vrpn_Tracker::pos,
                  vrpn_Tracker::d_quat);

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


