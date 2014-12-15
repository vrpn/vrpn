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
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#ifdef VRPN_USE_OVR
#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface, etc
#include "vrpn_SendTextMessageStreamProxy.h"  // for operator<<, etc

// Library/third-party includes
#include <OVR.h>
#include <boost/lexical_cast.hpp>

// Standard includes
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for char_traits, basic_string, etc
#include <cstddef>                      // for size_t
#include <cstdio>                       // for fprintf, NULL, stderr
#include <cstring>                      // for memset


vrpn_Tracker_OculusRift::vrpn_Tracker_OculusRift(const char* name, vrpn_Connection* conn, int hmd_index)
    : vrpn_Analog(name, conn)
    , vrpn_Tracker(name, conn)
{
    ovrBool initialized = ovr_Initialize();
    if (!initialized) {
        // TODO report error and do something else (set global state? try again
        // later?)
        //fprintf(stderr, "Error initializing Oculus Rift API.");
        send_text_message(vrpn_TEXT_ERROR) << "Error initializing Oculus Rift API.";
    }

    int num_hmds = ovrHmd_Detect();
    send_text_message(vrpn_TEXT_NORMAL) << "Detected " << num_hmds << " HMDs.";

    if (0 <= hmd_index && hmd_index < num_hmds) {
        send_text_message(vrpn_TEXT_NORMAL) << "Attached to HMD index " << hmd_index << ".";
        _hmd = ovrHmd_Create(0);
    } else {
        if (num_hmds > 0) {
            send_text_message(vrpn_TEXT_WARNING) << "Requested HMD index "
                << hmd_index << " is outside the valid range [0.."
                << (num_hmds > 1 ? num_hmds - 1 : 0)
                << "]. Creating debug HMD to use instead.";
        } else {
            send_text_message(vrpn_TEXT_WARNING)
                << "No HMDs are currently attached. Creating debug HMD.";
        }
        _hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
    }

    // Get more information about the HMD
    ovrHmdType hmd_type = _hmd->Type;
    //typedef enum
    //{
    //    ovrHmd_None             = 0,
    //    ovrHmd_DK1              = 3,
    //    ovrHmd_DKHD             = 4,
    //    ovrHmd_DK2              = 6,
    //    ovrHmd_Other             // Some HMD other then the one in the enumeration.
    //} ovrHmdType;

    // Send along some information about the HMD we're connected to
    send_text_message(vrpn_TEXT_NORMAL)
        << "Connected to " << _hmd->ProductName;
    if (strlen(_hmd->SerialNumber) > 0)
        send_text_message(vrpn_TEXT_NORMAL)
            << "Serial: "
            << _hmd->SerialNumber;
    if (_hmd->FirmwareMajor >= 0)
        send_text_message(vrpn_TEXT_NORMAL)
            << "Firmware version: "
            << _hmd->FirmwareMajor << "."
            << _hmd->FirmwareMinor;

    // Initialize tracking and sensor fusion
    const unsigned int supported_tracking_capabilities = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position; // the tracking capabilities VRPN will report
    const unsigned int required_tracking_capabilities = 0; // the tracking capabilities which must be supported by the HMD
    const ovrBool tracking_configured = ovrHmd_ConfigureTracking(_hmd, supported_tracking_capabilities, required_tracking_capabilities);
    // returns FALSE if the required tracking capabilities are not supported (e.g., camera isn't plugged in)

    // Set up sensor counts
    vrpn_Analog::num_channel = ANALOG_CHANNELS;
    vrpn_Tracker::num_sensors = POSE_CHANNELS;
}

void vrpn_Tracker_OculusRift::_get_tracking_state()
{
    // Poll tracking data
    const ovrTrackingState ts = ovrHmd_GetTrackingState(_hmd, ovr_GetTimeInSeconds());
    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
        const ovrPoseStatef head_state = ts.HeadPose;              // head pose + ang/lin velocity + ang/lin accel
        const ovrPosef camera_pose = ts.CameraPose;                // camera position and orientation
        const ovrPosef leveled_camera_pose = ts.LeveledCameraPose; // same as camera pose but with roll and pitch zeroed out

        const ovrSensorData sensor_data = ts.RawSensorData;
        const ovrVector3f accelerator = sensor_data.Accelerometer; // acceleration reading in m/s^2
        const ovrVector3f gyro = sensor_data.Gyro;                 // rotation rate in rad/s
        const ovrVector3f magnetometer = sensor_data.Magnetometer; // magnetic field in Gauss
        const float temperature = sensor_data.Temperature;         // degrees Celsius

        // Deconstruct head pose
        const ovrPosef head_pose = head_state.ThePose;
        const ovrVector3f angular_velocity = head_state.AngularVelocity;
        // etc.

        // Poses
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

        acc[0] = head_state.LinearAcceleration.x;
        acc[1] = head_state.LinearAcceleration.y;
        acc[2] = head_state.LinearAcceleration.z;

        //vrpn_float64 vel[3], vel_quat[4]; // Cur velocity and dQuat/vel_quat_dt
        //vrpn_float64 vel_quat_dt;         // delta time (in secs) for vel_quat
        //vrpn_float64 acc[3], acc_quat[4]; // Cur accel and d2Quat/acc_quat_dt2
        //vrpn_float64 acc_quat_dt;         // delta time (in secs) for acc_quat
        //struct timeval timestamp;         // Current timestamp

        char msgbuf[512];
        int len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY))
        {
            fprintf(stderr, "vrpn_Tracker_OculusRift: cannot write message: tossing\n");
        }
    } else {
        send_text_message(vrpn_TEXT_WARNING) << "Orientation and position tracking is not supported on this HMD.";
    }

}

vrpn_Tracker_OculusRift::~vrpn_Tracker_OculusRift()
{
    ovrHmd_Destroy(_hmd);
    ovr_Shutdown();
}

void vrpn_Tracker_OculusRift::mainloop()
{
    // Server update.  We only need to call this once for all the
    // base devices because it is in the unique base class.
    server_mainloop();

    _get_tracking_state();
}

#endif // VRPN_USE_OVR

