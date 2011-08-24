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
#include "vrpn_Configure.h"
#include "vrpn_HumanInterface.h"
#include "vrpn_Tracker.h"

// Library/third-party includes
// - none

// Standard includes
#ifdef VRPN_USE_HID
#include <string>



class VRPN_API vrpn_Tracker_RazerHydra: public vrpn_Tracker, vrpn_HidInterface {
	public:
		vrpn_Tracker_RazerHydra(const char * name, vrpn_Connection * trackercon);

		virtual void mainloop();
		virtual void reconnect();

		virtual void on_data_received(size_t bytes, vrpn_uint8 *buffer);

	protected:
		std::string _name;
		vrpn_Connection *_con;

		bool _should_report;
		struct timeval _timestamp;
};

#endif

#endif // INCLUDED_vrpn_Tracker_RazerHydra_h_GUID_8c30e762_d7e7_40c5_9308_b9bc118959fd
