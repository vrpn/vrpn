#include "vrpn_Tracker_ThalmicLabsMyo.h"

#ifdef  VRPN_INCLUDE_THALMICLABSMYO

#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "quat.h" 

/*
 * TODO : 
 * Use myo timestamp when possible
 *		Timestamps are 64 bit unsigned integers that correspond to a number of microseconds since some (unspecified) period in time. Timestamps are monotonically non-decreasing.
 * Stream raw emg data ?
*/

// --------------------------------------------------------------------------
// Constructor:
// name (i): device name
// c (i): vrp8n_Connection
// useLock (b): require unlock gesture before detecting any gesture
// armSide (ARMSIDE) : specify (or don't) the arm side for the myo

vrpn_Tracker_ThalmicLabsMyo::vrpn_Tracker_ThalmicLabsMyo(const char *name, vrpn_Connection *c, bool useLock, ARMSIDE armSide) :
	vrpn_Tracker(name, c),
	vrpn_Analog(name, c),
	vrpn_Button_Filter(name, c),
	_armSide(armSide),
	hub(NULL),
	_myo(NULL),
	_analogChanged(false)
{
	while (hub == NULL)
	{
		try
		{
			hub = new myo::Hub();
			hub->setLockingPolicy(useLock ? myo::Hub::lockingPolicyStandard : myo::Hub::lockingPolicyNone);
			hub->addListener(this);

			/// Set up sensor counts
			vrpn_Analog::num_channel = 9; // 0,1,2 : euler rotation - 3,4,5 : acceleration - 6,7,8 : gyroscope
			vrpn_Button::num_buttons = libmyo_num_poses + 1; // we add one for lock/unlock
			vrpn_Tracker::num_sensors = 1; // we have orientation and acceleration for the tracker

			vrpn_gettimeofday(&_timestamp, NULL);
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
}

vrpn_Tracker_ThalmicLabsMyo::~vrpn_Tracker_ThalmicLabsMyo()
{
  if (hub != NULL) {
    try {
      delete hub;
    } catch (...) {
      fprintf(stderr, "vrpn_Tracker_ThalmicLabsMyo::~vrpn_Tracker_ThalmicLabsMyo(): delete failed\n");
      return;
    }
  }
}

void vrpn_Tracker_ThalmicLabsMyo::mainloop() {

	if (hub == NULL)
		return;

	// server update.  We only need to call this once for all three
	// base devices because it is in the unique base class.
	server_mainloop();
	
	hub->runOnce(1);

	// only report once per loop for analog changes
	if (_analogChanged)
	{
		vrpn_Analog::report_changes(vrpn_CONNECTION_LOW_LATENCY);
		_analogChanged = false;
	}
}

void vrpn_Tracker_ThalmicLabsMyo::_report_lock() {
	vrpn_gettimeofday(&_timestamp, NULL);
	double dt = vrpn_TimevalDurationSeconds(_timestamp, vrpn_Button::timestamp);
	vrpn_Button::timestamp = _timestamp;
	buttons[0] = _locked;
	vrpn_Button::report_changes();
}

void vrpn_Tracker_ThalmicLabsMyo::onPair(myo::Myo* myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion)
{
	std::cout << "Paired with Myo." << std::endl;
	myo->vibrate(myo->vibrationShort);
}

void vrpn_Tracker_ThalmicLabsMyo::onConnect(myo::Myo* myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion)
{
	std::cout << "Myo Connected." << std::endl;
	myo->vibrate(myo->vibrationShort);
}

void vrpn_Tracker_ThalmicLabsMyo::onDisconnect(myo::Myo* myo, uint64_t timestamp)
{
	std::cout << "Myo disconnected." << std::endl;
	if (myo == _myo)
		_myo = NULL;
}

void vrpn_Tracker_ThalmicLabsMyo::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
{
	if (myo != _myo)
		return;
	vrpn_gettimeofday(&_timestamp, NULL);
	double dt = vrpn_TimevalDurationSeconds(_timestamp, vrpn_Button::timestamp);
	vrpn_Button::timestamp = _timestamp;
	

	//	std::cout << "Myo switched to pose " << pose.toString() << "." << std::endl;

	buttons[0] = _locked;
	// reset all buttons to 0. Maybe we should only do this if rest is on ?
	for (int i = 1; i < libmyo_num_poses + 1; ++i)
		buttons[i] = 0;
	buttons[pose.type() + 1] = 1;

	vrpn_Button::report_changes();
}

void vrpn_Tracker_ThalmicLabsMyo::onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
{
	std::cout << "Myo on arm : " << (arm == myo::armLeft ? "Left." : "Right.") << std::endl;
	if (_armSide == ARMSIDE::ANY || (_armSide == ARMSIDE::RIGHT && arm == myo::armRight) || (_armSide == ARMSIDE::LEFT && arm == myo::armLeft))
	{
		std::cout << "Myo : " << d_servicename << " accepted" << std::endl;
		myo->vibrate(myo->vibrationShort);
		_myo = myo;
	}
	else
	{
		std::cout << "Myo : " << d_servicename << " NOT accepted : wrong arm detected" << std::endl;
	}
	
}

void vrpn_Tracker_ThalmicLabsMyo::onArmUnsync(myo::Myo* myo, uint64_t timestamp)
{
	if (myo != _myo)
		return;
	std::cout << "Myo : " << d_servicename <<" removed from arm."<< std::endl;
	myo->vibrate(myo->vibrationLong);
	_myo = NULL;
}

void vrpn_Tracker_ThalmicLabsMyo::onUnlock(myo::Myo* myo, uint64_t timestamp)
{
	if (myo != _myo)
		return;
	_locked = false;
	_report_lock();
}

void vrpn_Tracker_ThalmicLabsMyo::onLock(myo::Myo* myo, uint64_t timestamp)
{
	if (myo != _myo)
		return;
	_locked = true;
	_report_lock();
}

void vrpn_Tracker_ThalmicLabsMyo::onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& rotation)
{
	if (myo != _myo)
		return;
	if (!d_connection) {
		return;
	}
	vrpn_gettimeofday(&_timestamp, NULL);
	double dt = vrpn_TimevalDurationSeconds(_timestamp, vrpn_Button::timestamp);
	vrpn_Tracker::timestamp = _timestamp;
	vrpn_Analog::timestamp = _timestamp;

	d_quat[0] = rotation.x();
	d_quat[1] = rotation.y();
	d_quat[2] = rotation.z();
	d_quat[3] = rotation.w();

	// do the same as analog, with euler angles (maybe offset from when OnArmSync?)
	q_vec_type euler;

	q_to_euler(euler, d_quat);
	channel[ANALOG_ROTATION_X] = euler[Q_ROLL];
	channel[ANALOG_ROTATION_Y] = euler[Q_PITCH];
	channel[ANALOG_ROTATION_Z] = euler[Q_YAW];


	char msgbuf[1000];
	int len = vrpn_Tracker::encode_to(msgbuf);
	if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
		fprintf(stderr, "Thalmic Lab's myo tracker: can't write message: tossing\n");
	}

	_analogChanged = true;
}

void vrpn_Tracker_ThalmicLabsMyo::onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel)
{
	if (myo != _myo)
		return;
	if (!d_connection) {
		return;
	}
	vrpn_gettimeofday(&_timestamp, NULL);
	double dt = vrpn_TimevalDurationSeconds(_timestamp, vrpn_Button::timestamp);
	vrpn_Tracker::timestamp = _timestamp;
	vrpn_Analog::timestamp = _timestamp;
	
	acc[0] = accel[0];
	acc[1] = accel[1];
	acc[2] = accel[2];

	// same for analog
	channel[ANALOG_ACCEL_X] = accel[0];
	channel[ANALOG_ACCEL_Y] = accel[1];
	channel[ANALOG_ACCEL_Z] = accel[2];

	char msgbuf[1000];
	int len = encode_acc_to(msgbuf);
	if (d_connection->pack_message(len, _timestamp, accel_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
		fprintf(stderr, "Thalmic Lab's myo tracker: can't write message: tossing\n");
	}
	_analogChanged = true;
}

void vrpn_Tracker_ThalmicLabsMyo::onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro)
{
	if (myo != _myo)
		return;
	vrpn_gettimeofday(&_timestamp, NULL);
	double dt = vrpn_TimevalDurationSeconds(_timestamp, vrpn_Button::timestamp);

	vrpn_Analog::timestamp = _timestamp;

	channel[ANALOG_GYRO_X] = gyro[0];
	channel[ANALOG_GYRO_Y] = gyro[1];
	channel[ANALOG_GYRO_Z] = gyro[2];

	_analogChanged = true;
}

#endif

