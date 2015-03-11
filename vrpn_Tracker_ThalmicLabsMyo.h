// vrpn_Tracker_ThalmicLabsMyo.h 
// 
// Thalmic Labs Myo's (https://www.thalmic.com/en/myo/) VRPN server
//
// (10/12/2014) developed by Florian Nouviale

#ifndef __TRACKER_THALMICLABSMYO_H
#define __TRACKER_THALMICLABSMYO_H

#include "vrpn_Configure.h"             // for VRPN_API

#ifdef VRPN_INCLUDE_THALMICLABSMYO

#include "vrpn_Tracker.h"                // for vrpn_Analog
#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filterl

#include <myo/myo.hpp>

// --------------------------------------------------------------------------
// VRPN class:

class VRPN_API vrpn_Tracker_ThalmicLabsMyo : public vrpn_Tracker, public vrpn_Analog, public vrpn_Button_Filter, public myo::DeviceListener
{

public:

	enum ARMSIDE
	{
		ANY = 0,
		LEFT = 1,
		RIGHT = 2
	};

	vrpn_Tracker_ThalmicLabsMyo(const char *name,
		vrpn_Connection *c,
		bool useLock,
		ARMSIDE armSide);

	~vrpn_Tracker_ThalmicLabsMyo();

	virtual void mainloop();


	virtual void onPair(myo::Myo* myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion);

	virtual void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose);

	virtual void onConnect(myo::Myo* myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion);

	virtual void onDisconnect(myo::Myo* myo, uint64_t timestamp);

	virtual void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection);

	virtual void onArmUnsync(myo::Myo* myo, uint64_t timestamp);

	virtual void onUnlock(myo::Myo* myo, uint64_t timestamp);

	virtual void onLock(myo::Myo* myo, uint64_t timestamp);

	/// Called when a paired Myo has provided new orientation data.
	virtual void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& rotation);

	/// Called when a paired Myo has provided new accelerometer data in units of g.
	virtual void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel);

	/// Called when a paired Myo has provided new gyroscope data in units of deg/s.
	virtual void onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro);

private:

	struct timeval _timestamp;

	void _report_lock();

	enum {
		ANALOG_ROTATION_X = 0,
		ANALOG_ROTATION_Y = 1,
		ANALOG_ROTATION_Z = 2,
		ANALOG_ACCEL_X = 3,
		ANALOG_ACCEL_Y = 4,
		ANALOG_ACCEL_Z = 5,
		ANALOG_GYRO_X = 6,
		ANALOG_GYRO_Y = 7,
		ANALOG_GYRO_Z = 8
	};

	myo::Hub* hub;

	myo::Myo* _myo;

	ARMSIDE _armSide;

	bool _locked;

	bool _analogChanged;
};

#endif  //VRPN_INCLUDE_THALMICLABSMYO
#endif	//__TRACKER_THALMICLABSMYO_H