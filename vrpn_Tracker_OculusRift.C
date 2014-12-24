/**
	@file
	@brief Implementation

	@date 2014

	@author
	Kevin M. Godby
	<kevin@godby.org>
*/

// Internal Includes
#include "vrpn_Tracker_OculusRift.h"
#include "quat.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#ifdef VRPN_USE_OVR
#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface, etc
#include "vrpn_SendTextMessageStreamProxy.h"  // for operator<<, etc

// Library/third-party includes
#include <OVR.h>

// Standard includes
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for char_traits, basic_string, etc
#include <cstddef>                      // for size_t
#include <cstdio>                       // for fprintf, NULL, stderr
#include <cstring>                      // for memset
#include <cctype>                       // for tolower
#include <cstdlib>                      // for exit, EXIT_FAILURE

vrpn_Tracker_OculusRift::vrpn_Tracker_OculusRift(const char* name, vrpn_Connection* conn, int hmd_index)
    : vrpn_Analog(name, conn)
    , vrpn_Tracker(name, conn)
    , _hmdIndex(hmd_index)
    , _status(RIFT_UNINITIALIZED)
    , _hmd(NULL)
{
    // Set up sensor counts
    vrpn_Analog::num_channel = ANALOG_CHANNELS;
    vrpn_Tracker::num_sensors = POSE_CHANNELS;

    // Initialize the Oculus Rift API
    _init();
}

void vrpn_Tracker_OculusRift::_init()
{
    if (_status != RIFT_UNINITIALIZED) {
        fprintf(stderr, "Oculus Rift has already been initialized.\n");
        send_text_message(vrpn_TEXT_ERROR) << "Oculus Rift has already been initialized.";
        return;
    }

    // Capture the system time nearly simultaneously with the initialization of
    // the OVR API. This local timestamp will correspond to t = 0 for the OVR
    // API.
    vrpn_gettimeofday(&_startTime, NULL);
    ovrBool initialized = ovr_Initialize();
    if (!initialized) {
        fprintf(stderr, "Error initializing Oculus Rift API.");
        send_text_message(vrpn_TEXT_ERROR) << "Error initializing Oculus Rift API.";
        return;
    }

    _status = RIFT_WAITING_TO_CONNECT;
}

void vrpn_Tracker_OculusRift::_connect()
{
    int num_hmds = ovrHmd_Detect();
    send_text_message(vrpn_TEXT_NORMAL) << "Detected " << num_hmds << " HMDs.";

    if (0 <= _hmdIndex && _hmdIndex < num_hmds) {
        send_text_message(vrpn_TEXT_NORMAL) << "Attached to HMD index " << _hmdIndex << ".";
        _hmd = ovrHmd_Create(_hmdIndex);
    } else {
        if (num_hmds > 0) {
            send_text_message(vrpn_TEXT_ERROR) << "Requested HMD index "
                << _hmdIndex << " is outside the valid range [0.."
                << (num_hmds > 1 ? num_hmds - 1 : 0) << "].";
        } else {
            send_text_message(vrpn_TEXT_ERROR) << "No HMDs were detected.";
        }
        _hmd = NULL;
        return;
    }

    // Initialize tracking and sensor fusion
    const unsigned int supported_tracking_capabilities = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position; // the tracking capabilities VRPN will report
    const unsigned int required_tracking_capabilities = 0; // the tracking capabilities which must be supported by the HMD
    const ovrBool tracking_configured = ovrHmd_ConfigureTracking(_hmd, supported_tracking_capabilities, required_tracking_capabilities);
    // returns FALSE if the required tracking capabilities are not supported (e.g., camera isn't plugged in)

    _status = RIFT_REPORTING;
}

void vrpn_Tracker_OculusRift::_update()
{
    // Poll tracking data
    const ovrTrackingState ts = ovrHmd_GetTrackingState(_hmd, ovr_GetTimeInSeconds());

    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
        _report_hmd_pose(ts);
    }

    if (ts.StatusFlags & (ovrStatus_CameraPoseTracked)) {
        _report_camera_pose(ts);
    }

    _report_sensor_values(ts);
}

void vrpn_Tracker_OculusRift::_report_hmd_pose(const ovrTrackingState& ts)
{
    // Sensor 0: HMD pose
    //
    // Predicted head pose (and derivatives).
    d_sensor = 0;

    const ovrPoseStatef head_state = ts.HeadPose;
    const ovrPosef head_pose = head_state.ThePose;

    pos[0] = head_pose.Position.x;
    pos[1] = head_pose.Position.y;
    pos[2] = head_pose.Position.z;

    d_quat[Q_W] = head_pose.Orientation.w;
    d_quat[Q_X] = head_pose.Orientation.x;
    d_quat[Q_Y] = head_pose.Orientation.y;
    d_quat[Q_Z] = head_pose.Orientation.z;

    vel[0] = head_state.LinearVelocity.x;
    vel[1] = head_state.LinearVelocity.y;
    vel[2] = head_state.LinearVelocity.z;

    q_type quat;
    q_from_euler(quat, head_state.AngularVelocity.y, head_state.AngularVelocity.x, head_state.AngularVelocity.z);
    vel_quat[Q_W] = quat[Q_W];
    vel_quat[Q_X] = quat[Q_X];
    vel_quat[Q_Y] = quat[Q_Y];
    vel_quat[Q_Z] = quat[Q_Z];

    acc[0] = head_state.LinearAcceleration.x;
    acc[1] = head_state.LinearAcceleration.y;
    acc[2] = head_state.LinearAcceleration.z;

    q_from_euler(quat, head_state.AngularAcceleration.y, head_state.AngularAcceleration.x, head_state.AngularAcceleration.z);
    acc_quat[Q_W] = quat[Q_W];
    acc_quat[Q_X] = quat[Q_X];
    acc_quat[Q_Y] = quat[Q_Y];
    acc_quat[Q_Z] = quat[Q_Z];

    vrpn_Tracker::timestamp = vrpn_TimevalSum(_startTime, head_state.TimeInSeconds);

    char msgbuf[512];
    int len = vrpn_Tracker::encode_to(msgbuf);
    if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Tracker_OculusRift: cannot write message: tossing\n");
    }
}

void vrpn_Tracker_OculusRift::_report_camera_pose(const ovrTrackingState& ts)
{
    // Sensor 1: Camera pose
    //
    // Current pose of the external camera (if present).  This pose includes
    // camera tilt (roll and pitch). For a leveled coordinate system use
    // LeveledCameraPose.
    d_sensor = 1;

    const ovrPosef camera_pose = ts.CameraPose;
    pos[0] = camera_pose.Position.x;
    pos[1] = camera_pose.Position.y;
    pos[2] = camera_pose.Position.z;

    d_quat[Q_W] = camera_pose.Orientation.w;
    d_quat[Q_X] = camera_pose.Orientation.x;
    d_quat[Q_Y] = camera_pose.Orientation.y;
    d_quat[Q_Z] = camera_pose.Orientation.z;

    char msgbuf[512];
    int len = vrpn_Tracker::encode_to(msgbuf);
    if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Tracker_OculusRift: cannot write message: tossing\n");
    }

    // Sensor 2: Leveled camera pose
    //
    // Camera frame aligned with gravity.  This value includes position and
    // yaw of the camera, but not roll and pitch.  It can be used as a
    // reference point to render real-world objects in the correct location.
    d_sensor = 2;

    const ovrPosef leveled_camera_pose = ts.LeveledCameraPose;
    pos[0] = leveled_camera_pose.Position.x;
    pos[1] = leveled_camera_pose.Position.y;
    pos[2] = leveled_camera_pose.Position.z;

    d_quat[Q_W] = leveled_camera_pose.Orientation.w;
    d_quat[Q_X] = leveled_camera_pose.Orientation.x;
    d_quat[Q_Y] = leveled_camera_pose.Orientation.y;
    d_quat[Q_Z] = leveled_camera_pose.Orientation.z;

    len = vrpn_Tracker::encode_to(msgbuf);
    if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Tracker_OculusRift: cannot write message: tossing\n");
    }
}

void vrpn_Tracker_OculusRift::_report_sensor_values(const ovrTrackingState& ts)
{
    // VRPN analogs, multiple channels.
    // * accelerometer reading in m/s^2
    // * gyro rotation rate in rad/s.
    // * magnetometer in Gauss
    // * temperature of sensor in degrees Celsius

    const ovrSensorData sensor_data = ts.RawSensorData;

    // Acceleration reading (x, y, z) in m/s^2
    const ovrVector3f accelerator = sensor_data.Accelerometer;
    channel[0] = accelerator.x;
    channel[1] = accelerator.y;
    channel[2] = accelerator.z;

    // Rotation rate (x, y, z) in rad/s
    const ovrVector3f gyro = sensor_data.Gyro;
    channel[3] = gyro.x;
    channel[4] = gyro.y;
    channel[5] = gyro.z;

    // Magnetic field (x, y, z) in Gauss
    const ovrVector3f magnetometer = sensor_data.Magnetometer;
    channel[6] = magnetometer.x;
    channel[7] = magnetometer.y;
    channel[8] = magnetometer.z;

    // Temperature of sensor in degrees Celsius
    const float temperature = sensor_data.Temperature;
    vrpn_Analog::channel[9] = temperature;

    vrpn_Analog::timestamp = vrpn_TimevalSum(_startTime, sensor_data.TimeInSeconds);
    vrpn_Analog::report(vrpn_CONNECTION_LOW_LATENCY);
}

vrpn_Tracker_OculusRift::~vrpn_Tracker_OculusRift()
{
    if (_hmd)
        ovrHmd_Destroy(_hmd);
    ovr_Shutdown();
    _status = RIFT_UNINITIALIZED;
}

void vrpn_Tracker_OculusRift::mainloop()
{
    // Server update.  We only need to call this once for all the
    // base devices because it is in the unique base class.
    server_mainloop();

    switch (_status) {
    case RIFT_UNINITIALIZED:
        _init();
        break;
    case RIFT_WAITING_TO_CONNECT:
        _connect();
        break;
    case RIFT_REPORTING:
        _update();
        break;
    default:
        fprintf(stderr, "vrpn_Tracker_OculusRift in unknown state %d. Resetting.", _status);
        _status = RIFT_UNINITIALIZED;
        break;
    };
}

#endif // VRPN_USE_OVR

