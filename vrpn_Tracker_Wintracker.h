/*
 * vrpn_Tracker_Wintracker.h
 *
 *  Created on: Dec 11, 2012
 *      Author: Emiliano Pastorelli - Institute of Cybernetics, Tallinn (Estonia)
 */

#ifndef VRPN_TRACKER_WINTRACKER_H_
#define VRPN_TRACKER_WINTRACKER_H_

#include "vrpn_Configure.h"
#include "vrpn_HumanInterface.h"
#include "vrpn_Tracker.h"


#if defined(VRPN_USE_HID)
#include <string>

class VRPN_API vrpn_Tracker_Wintracker: public vrpn_Tracker, vrpn_HidInterface {

	public:
		vrpn_Tracker_Wintracker(const char * name, vrpn_Connection * trackercon, const char s0,  const char s1,  const char s2, const char ext, const char hemisphere);

		virtual void mainloop();

		virtual void on_data_received(size_t bytes, vrpn_uint8 *buffer);

protected:
  	  std::string _name;
      vrpn_Connection *_con;

      bool _should_report;
      struct timeval _timestamp;
};
#endif

#endif /* VRPN_TRACKER_WINTRACKER_H_ */
