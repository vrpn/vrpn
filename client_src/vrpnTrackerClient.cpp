/*****************************************************************************\
  vrpnTrackerClient.cpp
  --
  Description : illustrates how to write a tracker client and how to
                alter the default synchronized connection parameters

  To compile :  g++ -I/afs/cs.unc.edu/proj/hmd/beta/include -L/afs/cs.unc.edu/proj/hmd/beta/lib/sgi_irix -o vrpnTrackerClient vrpnTrackerClient.cpp -lvrpn_g++ -lsdi
  ----------------------------------------------------------------------------
  Author: weberh
  Created: Tue Dec 16 16:03:22 1997
  Revised: Tue Jan 20 01:13:47 1998 by weberh
\*****************************************************************************/

#include <vrpn_Tracker.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#endif

void printTrackerPosOri(void *userdata, const vrpn_TRACKERCB info ) {
  static int i=0;
  if (!(i%101)) {
    if (info.sensor==0) {
    cerr << "posOri: ";
    cerr << "Sensor: " << info.sensor;
    cerr << " Pos: (" << info.pos[0]/0.0254 << ", " << info.pos[1]/0.0254 << ", " 
	 << info.pos[2]/0.0254 << ")   Ori: (" << info.quat[3] << ", <" 
	 << info.quat[0] << ", " << info.quat[1] << ", " << info.quat[2] 
	 << ">)" << endl;
    }
  }

  i++;
}

void printTrackerFirstDeriv(void *userdata, const vrpn_TRACKERVELCB info ) {
  static int i=0;
  if (!(i%101)) {
    cerr << "dPosOri: ";
    cerr << "Sensor: " << info.sensor;
    cerr << " dPos: (" << info.vel[0] << ", " << info.vel[1] << ", " 
	 << info.vel[2] << ")   dOri: (" << info.vel_quat[3] << ", <" 
	 << info.vel_quat[0] << ", " << info.vel_quat[1] << ", " 
	 << info.vel_quat[2] << ">)   dT (for dOri): " << info.vel_quat_dt
	 << endl;
  }
  i++;
}

void printTrackerSecondDeriv(void *userdata, const vrpn_TRACKERACCCB info ) {
  static int i=0;
  if (!(i%101)) {
    cerr << "ddPosOri: ";
    cerr << "Sensor: " << info.sensor;
    cerr << " ddPos: (" << info.acc[0] << ", " << info.acc[1] << ", " 
	 << info.acc[2] << ")   ddOri: (" << info.acc_quat[3] << ", <" 
	 << info.acc_quat[0] << ", " << info.acc_quat[1] << ", " 
	 << info.acc_quat[2] << ">)   dT (for ddOri): " << info.acc_quat_dt
	 << endl;
  }
  i++;
}

main(int argc, char *argv[]) {
  vrpn_Tracker_Remote *pTracker;

  if (argc==2) {
    // just use defaults and open tracker, ie Tracker0@histidine, where
    // histidine is in your .sdi_devices file, and we assume the tracker
    // will be Tracker0 since that is the convention
    pTracker = new vrpn_Tracker_Remote(argv[1]);
  } else if (argc==3) {
    // sync the clocks at argv[2] frequency
    vrpn_get_connection_by_name( argv[1], atof(argv[2]));
    pTracker = new vrpn_Tracker_Remote(argv[1]);
  } else if (argc>=4) {
    // sync the clocks at argv[2] frequency, and use a sync window
    // of argv[3] readings
    vrpn_get_connection_by_name( argv[1], atof(argv[2]), atoi(argv[3]));
    pTracker = new vrpn_Tracker_Remote(argv[1]);
  } else {
    cerr << "Usage: " << argv[0] << " sdiTrackerName [syncFreq] [syncWindow]"
	 << " (you can't specify a sync window without a sync freq)." << endl;
    return -1;
  }    

  // Register the callback handlers for the diff types of tracker messages
  pTracker->register_change_handler(NULL, printTrackerPosOri);
  pTracker->register_change_handler(NULL, printTrackerFirstDeriv);
  pTracker->register_change_handler(NULL, printTrackerSecondDeriv);

  cerr.setf(ios::fixed);
  cerr.precision(5);
  
  // call the tracker main loop
  while (1) {
    pTracker->mainloop();
  }
  return 0;
}
