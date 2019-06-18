/** @file
	@brief Header

	@date 2014

	@author
	Kevin M. Godby
	<kevin@godby.org>
*/

#ifndef VRPN_TRACKER_OCULUS_RIFT_H_
#define VRPN_TRACKER_OCULUS_RIFT_H_

// Internal Includes
#include "quat.h"                       // for q_vec_type
#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Tracker.h"               // for vrpn_Tracker

// Library/third-party includes
// - none

// Standard includes
#include <string>

#ifdef VRPN_USE_OVR

/** \name Forward declarations */
//@{

typedef struct ovrHmdDesc_ ovrHmdDesc;
typedef const ovrHmdDesc* ovrHmd;

typedef struct ovrTrackingState_ ovrTrackingState;

//@}

/** @brief Device supporting the Oculus Rift DK1 and DK2.
 *
 * The Oculus Rift hardware contains a number of MEMS sensors including a
 * gyroscope, acceleromter, and magnetometer. Starting with DK2, there is also
 * an external camera to track the HMD position.
 */
class VRPN_API vrpn_Tracker_OculusRift : public vrpn_Analog, public vrpn_Tracker {
public:
    vrpn_Tracker_OculusRift(const char* name, vrpn_Connection* conn, int hmd_index = 0);
    virtual ~vrpn_Tracker_OculusRift();

    virtual void mainloop();

private:
    enum {
        ANALOG_CHANNELS = 10,  // accelerometer[3], gyro[3], magnetometer[3], temperature
        POSE_CHANNELS = 3      // head, camera, leveled camera
    };

    enum RiftStatus {
        RIFT_UNINITIALIZED,         //< API has not yet been initialized
        RIFT_WAITING_TO_CONNECT,    //< API has been initialized successfully, not yet receiving data from the Rift
        RIFT_REPORTING              //< receiving data from the Rift
    };

    void _init();
    void _connect();
    void _update();
    void _report_hmd_pose(const ovrTrackingState&);
    void _report_camera_pose(const ovrTrackingState&);
    void _report_sensor_values(const ovrTrackingState&);

    int _hmdIndex;              //< index of HMD to connect to (0-indexed)
    RiftStatus _status;         //< status of Oculus Rift driver
    ovrHmd _hmd;                //< HMD device handle
    struct timeval _startTime;  //< time at which OVR API was initialized
};

#else
class VRPN_API vrpn_Tracker_OculusRift;
#endif // VRPN_USE_OVR

#endif // VRPN_TRACKER_OCULUS_RIFT_H_

