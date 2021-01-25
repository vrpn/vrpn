#pragma once

#include <stddef.h>                     // for size_t

#include "vrpn_Configure.h"             // for VRPN_API, VRPN_USE_HID
#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_Types.h"                 // for vrpn_uint8

#include <Antilatency.Api.h>
#include <Antilatency.InterfaceContract.LibraryLoader.h>

class VRPN_API vrpn_Connection;

class VRPN_API vrpn_Tracker_Alt : public vrpn_Tracker
{
public:
    vrpn_Tracker_Alt(const char* name, vrpn_Connection* trackercon, std::string placementCode = "", std::string environmentCode = "", std::string libsPath = "", bool zv = false, bool zav = false);
	~vrpn_Tracker_Alt(){
		std::cout << "vrpn_Tracker_Alt::~vrpn_Tracker_Alt()";
	}
	virtual void mainloop();
    void setExtrapolationTime(float extrapolationTime);

protected:
	static Antilatency::StorageClient::ILibrary _storageClient;
	static Antilatency::Alt::Tracking::ILibrary _altTrackingLibrary;
	static Antilatency::DeviceNetwork::ILibrary _adnLibrary;
	static Antilatency::DeviceNetwork::INetwork _deviceNetwork;

	Antilatency::DeviceNetwork::NodeHandle GetTrackingNode();
	struct timeval _timestamp;
    float _extrapolationTime = 0.0f;
	bool _zv = false;
	bool _zav = false;
	std::string _tag;
    Antilatency::Alt::Tracking::ITrackingCotask _trackingCotask;
	Antilatency::Alt::Tracking::IEnvironment _environment;
	Antilatency::Math::floatP3Q _placement;
	Antilatency::DeviceNetwork::NodeHandle _node = Antilatency::DeviceNetwork::NodeHandle::Null;
	Antilatency::Alt::Tracking::ITrackingCotaskConstructor _cotaskConstructor;

	template<typename T, typename TRes>
	void quaternionFromAngularVelocity(TRes* result, const T& x, const T& y, const T& z){
		auto angle = sqrt(x*x+y*y+z*z);
		if (static_cast<double>(angle) > 0){
			auto halfAngle = angle * 0.5f;
			auto sinHalfAngle = sin(halfAngle);
			auto cosHalfAngle = cos(halfAngle);
			result[0] = x * (sinHalfAngle / angle);
			result[1] = y * (sinHalfAngle / angle);
			result[2] = z * (sinHalfAngle / angle);
			result[3] = cosHalfAngle;
		} else {
			result[0] = x * 0.5f;
			result[1] = y * 0.5f;
			result[2] = z * 0.5f;
			result[3] = 1.0f;
		}
	}
};

