/*****************************************************************************\
  vrpn_Flock_Parallel.h
  --
  Description : implements a class which runs an ascension flock of birds
                tracker in multiple rs232 mode.

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Thu Mar  5 19:38:55 1998
  Revised: Tue Mar 10 14:09:55 1998 by weberh
  $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Flock_Parallel.h,v $
  $Locker:  $
  $Revision: 1.4 $
\*****************************************************************************/
#ifndef _VRPN_FLOCK_PARALLEL_H_
#define _VRPN_FLOCK_PARALLEL_H_

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Flock.h"

// This is a class which provides a server for an ascension 
// Flock of Birds tracker.  The server will send out messages
// at whatever rate the flock of bird's stream mode feeds them.
// Default filtering is active.
// The timestamp is the time when the first character was read
// from the serial driver with "read".  No adjustment is currently
// made to this time stamp.

// Adelstein, et al, "Dynamic response of electromagnetic spatial
// displacement trackers", Presence 5(3) found that if all of the filters
// are turned off, and the time required to transport the data to the
// server is ignored, then the latency of the flock is about 2 ms for ori,
// 7.5 ms for pos for the 0-4hz range (ie, when the data is ready to leave
// the flock in stream mode, the pos is 7.5 ms old, and the ori is 2 ms
// old).  Later this data will be combined with the baud rate to provide a
// more accurate time stamp for certain flock modes.

// If this is running on a non-linux system, then the serial port driver
// is probably adding more latency -- see the vrpn README for more info.

// forward decl
class vrpn_Tracker_Flock_Parallel_Slave;

// this class acts as the master
// these are both derived from vrpn_Tracker_Flock to take advantage
// of some of the data and functionality, but they replace the major 
// functions
class vrpn_Tracker_Flock_Parallel: public vrpn_Tracker_Flock {
  
 public:
  
  // for parallel, you supply the master port and then the 
  // slave ports in a array with cSensors elements
  vrpn_Tracker_Flock_Parallel(char *name, vrpn_Connection *c, int cSensors,
			      char *port, long baud,
			      char *slavePortArray[] );
    
  virtual ~vrpn_Tracker_Flock_Parallel();
  virtual void mainloop(void);
    
 protected:
  virtual void get_report(void);
  virtual void reset();

  // slave ptrs
  vrpn_Tracker_Flock_Parallel_Slave *rgSlaves[MAX_SENSORS];
};

// the special args are the master's vrpn id and tracker pos msg id
// The slave masquerades as the master when sending out reports
class vrpn_Tracker_Flock_Parallel_Slave : public vrpn_Tracker_Flock {
public:
  vrpn_Tracker_Flock_Parallel_Slave( char *name,
				     vrpn_Connection *c,
				     char *port,
				     long baud,
				     long vrpnMasterID,
				     long vrpnPositionMsgID,
				     int iSensorID );
  virtual ~vrpn_Tracker_Flock_Parallel_Slave();
  virtual void mainloop();

 protected:
  virtual void reset();
  long vrpnMasterID;
  long vrpnPositionMsgID;

  friend class vrpn_Tracker_Flock_Parallel;
};


#endif // _VRPN_FLOCK_PARALLEL_H_


/*****************************************************************************\
  $Log: vrpn_Flock_Parallel.h,v $
  Revision 1.4  1998/11/05 22:45:49  taylorr
  This version strips out the serial-port code into vrpn_Serial.C.

  It also makes it so all the library files compile under NT.

  It also fixes an insidious initialization bug in the forwarder code.

  Revision 1.3  1998/06/01 20:12:10  kumsu
  changed to ANSI to compile with aCC for hp

  Revision 1.2  1998/05/05 21:09:56  taylorr
  This version works better with the aCC compiler on PixelFlow.

  Revision 1.1  1998/03/10 19:35:46  weberh
  The original parallel flock structure was a pain, so I redid it from
  scratch. The result is a new vrpn_Tracker_Flock_Parallel class and
  a corresponding slave class. Both leverage off of the base flock class
  heavily and avoid repeating code.  The master does all coordination and
  resetting, and the slaves just read reports.  In this way i am able to
  get 100hz from all of the sensors.

  I modified the makefile appropriately.

\*****************************************************************************/


