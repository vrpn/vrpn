/** @file
	@brief Header

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDED_vrpn_Tracker_RazerHydra_h_GUID_8c30e762_d7e7_40c5_9308_b9bc118959fd
#define INCLUDED_vrpn_Tracker_RazerHydra_h_GUID_8c30e762_d7e7_40c5_9308_b9bc118959fd

// Internal Includes
#include "quat.h"                       // for q_vec_type
#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include "vrpn_Configure.h"             // for VRPN_API, VRPN_USE_HID
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_Types.h"                 // for vrpn_uint8

// Library/third-party includes
// - none

// Standard includes
// - none

class VRPN_API vrpn_Connection;

#ifdef VRPN_USE_HID

/** @brief Device supporting the Razer Hydra game controller as a tracker,
	analog device, and button device, using the USB HID protocol directly

	The left wand (the one with LB and LT on its "end" buttons - look from above)
	is sensor 0, and the right wand (with RB and RT on it) is sensor 1.
	The "front" of the base is the side opposite the cables: there's a small
	logo on it. You can have the base in any orientation you want, but the info
	that follows assumes you have the base sitting on a desk, with the front toward you.
	If you have the base in a different coordinate frame in the world, please make
	the appropriate mental transformations yourself. :)

	When starting the VRPN server, make sure that the left wand is somewhere to
	the left of the base, and the right wand somewhere right of the base -
	they do not need to be placed on the base or any more complicated homing/calibration
	procedure. This is for the hemisphere tracking: it needs to have an "initial state"
	that is roughly known, so it uses the sign of the X coordinate position.

	(If you can't do this for whatever reason, modification of the driver code for an
	alternate calibration procedure is possible.)

	If using the Hydra on Windows, the server will work with or without the official
	Razer Hydra drivers installed. If you are only using the device with VRPN, don't
	install the official drivers. However, if you do have them installed, make sure that
	the "Hydra Configurator" and the Hydra system tray icon are closed to avoid unexpected
	failure (their software can switch the device out of the mode that VRPN uses).

	Works great on Linux (regardless of endianness) - no drivers needed, thanks to USB HID.

	The base coordinate system is right-handed with the axes:
	* X - out the right of the base
	* Y - out the front of the base
	* Z - down

	The wand coordinates are also right-handed, with the tracked point somewhere near
	the cable entry to the controller. When held with the joystick vertical, the axes
	are:
	* X - to the right
	* Y - out the front of the controller (trigger buttons)
	* Z - Up, along the joystick

	Buttons are as follows, with the right controller's button channels starting
	at 8 instead of 0:
	* 0 - "middle" button below joystick
	* 1-4 - numbered buttons
	* 5 - "bumper" button (above trigger)
	* 6 - joystick button (if you push straight down on the joystick)

	Analog channels are as follows, with the right controller starting at 3
	instead of 0:
	* 0 - joystick left/right: centered at 0, right is positive, in [-1, 1]
	* 1 - joystick up/down: centered at 0, up is positive, in [-1, 1]
	* 2 - analog trigger, in range 0 (not pressed) to 1 (fully pressed).
*/

class VRPN_API vrpn_Tracker_RazerHydra: public vrpn_Analog, public vrpn_Button_Filter, public vrpn_Tracker {
	public:
		vrpn_Tracker_RazerHydra(const char * name, vrpn_Connection * trackercon);
		~vrpn_Tracker_RazerHydra();

		virtual void mainloop();

		virtual bool reconnect();

	private:
		enum HydraStatus {
			HYDRA_WAITING_FOR_CONNECT,
			HYDRA_LISTENING_AFTER_CONNECT,
			HYDRA_LISTENING_AFTER_SET_FEATURE,
			HYDRA_REPORTING
		};
		enum {
			ANALOG_CHANNELS = 6,
			BUTTON_CHANNELS = 16,
			POSE_CHANNELS = 2
		};


		void _waiting_for_connect();
		void _listening_after_connect();
		void _listening_after_set_feature();

		void _enter_motion_controller_mode();

		void _report_for_sensor(int sensorNum, vrpn_uint8 * data, double dt);

		HydraStatus status;
		bool _wasInGamepadMode;
		int _attempt;
		struct timeval _timestamp;
		struct timeval _connected;
		struct timeval _set_feature;

		bool _calibration_done[POSE_CHANNELS];
		int _mirror[POSE_CHANNELS];
		q_vec_type _old_position[POSE_CHANNELS];

		struct FilterData;

		FilterData * _f;

		// This device has both a control and a data interface.
		// On the mac, we may need to swap these because we can't tell which
		// is which when we open them.
		class MyInterface;

		MyInterface * _ctrl;
		MyInterface * _data;
};

#endif

#endif // INCLUDED_vrpn_Tracker_RazerHydra_h_GUID_8c30e762_d7e7_40c5_9308_b9bc118959fd
