#include "vrpn_Tracker_Alt.h"

#include <Windows.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <quat/quat.h>
#include <chrono>

Antilatency::DeviceNetwork::INetwork vrpn_Tracker_Alt::_deviceNetwork;
Antilatency::Alt::Tracking::ILibrary vrpn_Tracker_Alt::_altTrackingLibrary;
Antilatency::DeviceNetwork::ILibrary vrpn_Tracker_Alt::_adnLibrary;
Antilatency::StorageClient::ILibrary vrpn_Tracker_Alt::_storageClient;

Antilatency::DeviceNetwork::NodeHandle vrpn_Tracker_Alt::GetTrackingNode(){
    auto result = Antilatency::DeviceNetwork::NodeHandle::Null;

    auto nodes = _cotaskConstructor.findSupportedNodes(_deviceNetwork);
	//try{
		if (!nodes.empty()) {
			for (auto node : nodes) {
				if (_deviceNetwork.nodeGetStatus(node) == Antilatency::DeviceNetwork::NodeStatus::Idle){
					auto socket = _deviceNetwork.nodeGetParent(node);
					if(socket != Antilatency::DeviceNetwork::NodeHandle::Null && _deviceNetwork.nodeGetStringProperty(socket, "Tag") == _tag){
						result = node;
						break;
					}
				}
			}
		} 
	//} catch(...) {
	//
	//}

    return result;
}

vrpn_Tracker_Alt::vrpn_Tracker_Alt(const char* name, vrpn_Connection* trackercon, std::string placementCode, std::string environmentCode, std::string libsPath, bool zv, bool zav)
    : vrpn_Tracker(name, trackercon), _tag(name), _zv(zv), _zav(zav)
{
    if (_adnLibrary == nullptr) {
		std::string path = libsPath + "AntilatencyDeviceNetwork";
		_adnLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::DeviceNetwork::ILibrary>(path.c_str());
        if (_adnLibrary == nullptr) {
			std::cout << "Failed to load AntilatencyDeviceNetwork library" << std::endl;
			throw;
		}
		_adnLibrary.setLogLevel(Antilatency::DeviceNetwork::LogLevel::Info);
		std::cout << "ADN version: " << _adnLibrary.getVersion() << std::endl;
    }

	if (_altTrackingLibrary == nullptr) {
		std::string path = libsPath + "AntilatencyAltTracking";
		_altTrackingLibrary = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::Alt::Tracking::ILibrary>(path.c_str());
        if (_altTrackingLibrary == nullptr) {
			std::cout << "Failed to load AntilatencyAltTracking library" << std::endl;
			throw;
		}
    }

    if (_storageClient == nullptr) {
		std::string path = libsPath + "AntilatencyStorageClient";
		_storageClient = Antilatency::InterfaceContract::getLibraryInterface<Antilatency::StorageClient::ILibrary>(path.c_str());
        if (_storageClient == nullptr) {
			std::cout << "Failed to load AntilatencyStorageClient library" << std::endl;
			throw;
		}
    }

	if(_deviceNetwork == nullptr){
		//Alt socket USB device ID
		Antilatency::DeviceNetwork::UsbDeviceType antilatencyUsbDeviceType;
		antilatencyUsbDeviceType.pid = 0x0000;
		antilatencyUsbDeviceType.vid = Antilatency::DeviceNetwork::UsbVendorId::Antilatency;

		//Alt socket USB device ID (deprecated)
		Antilatency::DeviceNetwork::UsbDeviceType antilatencyUsbDeviceTypeLegacy;
		antilatencyUsbDeviceTypeLegacy.pid = 0x0000;
		antilatencyUsbDeviceTypeLegacy.vid = Antilatency::DeviceNetwork::UsbVendorId::AntilatencyLegacy;

		_deviceNetwork = _adnLibrary.createNetwork(std::vector<Antilatency::DeviceNetwork::UsbDeviceType>{antilatencyUsbDeviceType, antilatencyUsbDeviceTypeLegacy});
		if(_deviceNetwork == nullptr){
			std::cout << "Failed to create Antilatency::DeviceNetwork" << std::endl;
			throw;
		}
	}

	// environment
	if(environmentCode == ""){
		environmentCode = _storageClient.getLocalStorage().read("environment", "default");
	}
	std::cout << "environment code: " << environmentCode << std::endl;
	_environment = _altTrackingLibrary.createEnvironment(environmentCode);

	// placement
	if(placementCode == ""){
		placementCode = _storageClient.getLocalStorage().read("placement", "default");
	}
    _placement = _altTrackingLibrary.createPlacement(placementCode);
	std::cout << "placement code: " << placementCode << std::endl;
	_cotaskConstructor = _altTrackingLibrary.createTrackingCotaskConstructor();
}

void vrpn_Tracker_Alt::mainloop()
{
	if(_node == Antilatency::DeviceNetwork::NodeHandle::Null || _trackingCotask == nullptr || _trackingCotask.isTaskFinished()){
		_node = GetTrackingNode();
		if(_node == Antilatency::DeviceNetwork::NodeHandle::Null){
			return;
		}
		_trackingCotask = _cotaskConstructor.startTask(_deviceNetwork, _node, _environment);
	}
	
    auto state = _trackingCotask.getState(Antilatency::Alt::Tracking::Constants::DefaultAngularVelocityAvgTime);
    auto extrapolatedState = _trackingCotask.getExtrapolatedState(_placement, _extrapolationTime);

	vrpn_gettimeofday(&_timestamp, NULL);
	auto dt = _timestamp.tv_sec - vrpn_Tracker::timestamp.tv_sec + 0.000001*(_timestamp.tv_usec - vrpn_Tracker::timestamp.tv_usec);
    vrpn_Tracker::timestamp = _timestamp;

	vrpn_Tracker::pos[0] = extrapolatedState.pose.position.x;
	vrpn_Tracker::pos[1] = extrapolatedState.pose.position.y;
	vrpn_Tracker::pos[2] = extrapolatedState.pose.position.z;
	//for angular velocity check
	//vrpn_float64 previousRotation [4];
	//memcpy(previousRotation, vrpn_Tracker::d_quat, 4*sizeof(vrpn_float64));

	vrpn_Tracker::d_quat[0] = extrapolatedState.pose.rotation.x;
	vrpn_Tracker::d_quat[1] = extrapolatedState.pose.rotation.y;
	vrpn_Tracker::d_quat[2] = extrapolatedState.pose.rotation.z;
	vrpn_Tracker::d_quat[3] = extrapolatedState.pose.rotation.w;

	//set velocity
	if(_zv){
		extrapolatedState.velocity = {0,0,0};
	}
	vrpn_Tracker::vel[0] = extrapolatedState.velocity.x;
	vrpn_Tracker::vel[1] = extrapolatedState.velocity.y;
	vrpn_Tracker::vel[2] = extrapolatedState.velocity.z;

	if(_zav){
		memset(vrpn_Tracker::vel, 0, 4*sizeof(*vrpn_Tracker::vel));
	} else {
		vrpn_float64 worldAngularVelocity[4];
		vrpn_float64 localAngVeloV[3];
		vrpn_float64 worldAngVeloV[3];
		localAngVeloV[0] = extrapolatedState.localAngularVelocity.x;
		localAngVeloV[1] = extrapolatedState.localAngularVelocity.y;
		localAngVeloV[2] = extrapolatedState.localAngularVelocity.z;
		q_xform(worldAngVeloV, vrpn_Tracker::d_quat, localAngVeloV); 
		quaternionFromAngularVelocity(vrpn_Tracker::vel_quat, worldAngVeloV[0], worldAngVeloV[1], worldAngVeloV[2]);
	}

	// check angular velocity
	/*{
		double x, y, z, angle;
		q_to_axis_angle(&x, &y, &z, &angle, vrpn_Tracker::vel_quat);
		vrpn_float64 invPreviousRotation [4];
		q_invert(invPreviousRotation, previousRotation);
		vrpn_float64 dr[4];
		q_mult(dr, vrpn_Tracker::d_quat, invPreviousRotation);
		double x_, y_, z_, angle_;
		q_to_axis_angle(&x_, &y_, &z_, &angle_, dr);
		angle_ /= dt;
		std::cout << "velocity     : {" << x << ", " << y << ", " << z << "}, angle: " << angle << std::endl;
		std::cout << "reconstructed: {" << x_ << ", " << y_ << ", " << z_ << "}, angle: " << angle_ << std::endl;
	}*/
	
	
	char msgbuf[1000];
	int len = vrpn_Tracker::encode_to(msgbuf);
	if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
		std::cerr << "vrpn_Tracker_Alt: can't write message" << std::endl;
	}
	//std::cout << "main loop. position: {" << pos[0] << ", " << pos[1] << ", " << pos[2] << "}; stage: " << static_cast<int32_t>(state.stability.stage) << std::endl;
	server_mainloop();
}

void vrpn_Tracker_Alt::setExtrapolationTime(float extrapolationTime) {
    _extrapolationTime = extrapolationTime;
}