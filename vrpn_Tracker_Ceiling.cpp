#include "vrpn_Tracker_Ceiling.h"
#include "vrpn_Shared.h"		 // defines gettimeofday for WIN32

#include "tracker.h"        // Ceiling tracker library (which uses CIB library)
#include <iostream.h>

/*
 * This vrpn class for the ceiling tracker outputs values at a rate
 * according to the configuration file (usually the tracker is running
 * much faster than the outputs).  -PBW 6/6/97
 */

// this var is used to store the offset between the time frame the
// RealTime class operates in and UTC (gettimeofday time -- secs, usecs
// since 1970)
static timeval tvReal2GetTimeOffset;
/*
 * The last line Track(allow...) is initializing the ceiling tracker,
 * this constructor will take a while to load.  The paramters
 * are argc, argv right now, and this is used to tell the tracker
 * whether it can use more than one thread as was specified in the
 * configuration file.
 */
char *ONETHREADARGS[2] = { "tracker", "-T" };
char *NORMARGS[] = { "tracker" };
static ARGSIZE = sizeof(NORMARGS)/sizeof(NORMARGS[0]);

vrpn_Tracker_Ceiling::vrpn_Tracker_Ceiling
(char *name, vrpn_Connection *c, int sensors, float Hz,
 int allowthreads, float pred) :
  vrpn_Tracker(name, c), update_rate(Hz), num_sensors(sensors), predict(pred),
  Track(allowthreads ? ARGSIZE : 2, allowthreads ? NORMARGS : ONETHREADARGS)
{
  //this never changes for now
  sensor = 0;
  
  // we try to hit this interval each mainloop
  interval_time = 1.0/update_rate;  
  
  // diagnostic message about pacing
  fprintf(stderr, "vprn_Tracker_Ceiling: %.2fHz - %.2fms cycles\n", 
	  update_rate, interval_time*1000);
  
  if (predict != 0) 
    fprintf(stderr, "vrpn_Tracker_Ceiling: %.2fms predictions\n", predict);
  
  predict /= 1000;  // must be in seconds
  
  logfile = NULL;
  logts = new TrackerState[1];  // if not logging just need 1
  logindex = logmaxindex = 0;
  reptcount = trkcount = 0;

  struct timeval tv, tvRT;
  double dNow1, dNow2;
  // call once to be sure it is inited
  gettimeofday(&tv, NULL);
  
  dNow1 = RealTime::Now();
  gettimeofday(&tv, NULL);
  dNow2 = RealTime::Now();
  
  cerr.precision(5);
  cerr.setf(ios::fixed);
  
  // offset between gettimeofday and 
  tvRT.tv_sec = (long) ((dNow1 + dNow2)/2.0);
  tvRT.tv_usec = (long) (1e6*(((dNow1 + dNow2)/2.0) - tvRT.tv_sec));
  tvReal2GetTimeOffset = vrpn_TimevalDiff( tv, tvRT );
}


void vrpn_Tracker_Ceiling::setupLogfile(char *name, int timeout)
{
  logfile = fopen(name, "w");
  
  if (!logfile) {
    fprintf(stderr, "vrpn_Tracker_Ceiling: Could not open log file "
	    "%s for writing\n", name);
    fprintf(stderr, "vrpn_Tracker_Ceiling: No log file being recorded\n");
    return;
  }
  
  fprintf(stderr, "vrpn_Tracker_Ceiling: Opened logfile %s - "
	  "%ds at %.1fHz\n", name, timeout, update_rate);
  
  // 1.02 is padding in case update rate varies
  int newsize = int(timeout * update_rate * 1.02); 
  logmaxindex = newsize - 1;
  
  fprintf(stderr, "vrpn_Tracker_Ceiling: allocating %.2fMB for %d TrackerStates\n",
	  ((double)newsize * sizeof(TrackerState))/1048576, newsize);
  
  delete [] logts;
  logts = new TrackerState[newsize];
  
  if (!logts) {
    fprintf(stderr, "vrpn_Tracker_Ceiling: new FAILED for %d TrackerStates\n",
	    newsize);
    fclose(logfile);
    exit(1);
  }
  
  logendtime = int(RealTime::Now()) + timeout;
}


void vrpn_Tracker_Ceiling::writeLogfile(void)
{
  fprintf(stderr, "vrpn_Tracker_Ceiling: writing logfile...\n");
  for (int i = 0; i < logindex; i++) {
    const TrackerState &ts = logts[i];
    fprintf(logfile, "%.5f %.6f %.6f %.6f %.4f %.4f %.4f ",
	    ts.Obs.time,
	    ts.xyz[0], ts.xyz[1], ts.xyz[2], 
	    ts.ypr[0], ts.ypr[1], ts.ypr[2]);
    fprintf(logfile, "%.4g %.4g %.4g %.4g %.4g %.4g\n",
	    ts.lv[0], ts.lv[1], ts.lv[2],
	    ts.av[0], ts.av[1], ts.av[2]);
    if (i > 0 && (i % 10000) == 0) {
      fprintf(stderr, "vrpn_Tracker_Ceiling: wrote %d lines...\n", i);
    }
  }
  fclose(logfile);
  fprintf(stderr, "vrpn_Tracker_Ceiling: Done writing. Wrote %d lines.\n", i);
  exit(1);
}


/*
 * This mainloop will send out a packet only every interval_time.  
 * It uses the static "endtime" to determine when to send the next packet.
 *
 * Timestamps: derived from packet, and transformed into UTC (gettimeofday style)
 * units in sendTrackerReport.
 *
 * Prediction: use KF to predict foward if needed.  Still use packet
 * timestamp for now.
 *
 * Quaternions: swap order from WXYZ which we use to quatlibs XYZW
 *
 * TrackerStates are overwritten in the logts array, the index is 
 * incremented in mainloop for each value reported.
 */

void vrpn_Tracker_Ceiling::mainloop(void)
{
  static double endtime = 0;
  static double last = 0;
  static double lasthzprint = RealTime::Now();

  // just so it is not put on the stack each time
  static double now;
  static int i=0;

  // This call did a spin wait, so the connection did not get serviced.
  // This destroyed the accuracy of the clock-synchronized connection.
  // Now the wait is wrapped around the vrpn connection loop.
  //  TrackerLoop(endtime);
  
  Track.UpdateState(logts[logindex]);
  ++trkcount;  // vrpn_Tracker_Ceiling member for stats

  last = now;
  now = RealTime::Now();

  // exit if endtime is less than iteration time away
  if (now >= endtime - (now-last)) {

    // set up next target end time
    endtime = now + interval_time;

    // send off reports if there is a connection
    if (connection) {		
      sendTrackerReport();
    }
    
    // log records to a file
    if (logfile) {
      logindex++;
      if (logindex == logmaxindex || logendtime < endtime)
	writeLogfile();
    }
    
    // report the update rate for every 720 reports sent
    if (++reptcount == 720) {
      double now = RealTime::Now();
      double elapsed = now - lasthzprint;
      fprintf(stderr, "Tracker: %.0fHz VRPN: %.2fHz\n", 
	      trkcount/elapsed, reptcount/elapsed);
      reptcount = trkcount = 0;
      lasthzprint = now;
    }
  }
}

// actually send off a report 

void vrpn_Tracker_Ceiling::sendTrackerReport(void) {
  char msgbuf[1024];	
  int len;

  // get the current tracker state
  TrackerState &ts = logts[logindex];
  
  // take timestamp from packet we are sending out
  timestamp.tv_sec  = long(ts.Obs.time);
  timestamp.tv_usec = long(1e6 * (ts.Obs.time - timestamp.tv_sec));
  
  // convert it to UTC time frame
  timestamp = vrpn_TimevalSum( tvReal2GetTimeOffset, timestamp );
  
  // use KF to predict ahead if requested.  NOTE: timestamp
  // is currently still from last real packet, should it be
  // predicted time?
  if (predict != 0) {
    double now = RealTime::Now();
    Track.hiball_filter->position(&ts.xyz[0], now + predict);
    Track.hiball_filter->orientation(&ts.quat[0], now + predict);
  }
  
  // Send pos and ori
  
  // copy position as-is
  pos[0] =  ts.xyz[0];
  pos[1] =  ts.xyz[1];
  pos[2] =  ts.xyz[2];
  // we copy the elements of the quat so as to convert it from the
  // W X Y Z format which the tracker code (and libgb) use to the
  // X Y Z W format which VRPN and quatlib specify.
  // The quat was normalized by hiballfilter
  quat[0] = ts.quat[1];
  quat[1] = ts.quat[2];
  quat[2] = ts.quat[3];
  quat[3] = ts.quat[0];
  
  len = encode_to(msgbuf);
  if (connection->pack_message(len, timestamp, position_m_id, 
			       my_id, msgbuf, 
			       vrpn_CONNECTION_LOW_LATENCY)) {
    fprintf(stderr,"Ceiling tracker: can't write message: tossing\n");
  }
  
  // send first deriv
  
  vel[0] =  ts.lv[0];
  vel[1] =  ts.lv[1];
  vel[2] =  ts.lv[2];
  
  // we actually send a delta quat, and one easy way to calc this is
  // to predict forward cdDeltaT seconds, then take the diff.
  // TO_DO: later dDeltaT shoudl be adjusted based on the rotation rate
  //        and/or the tracker update frequency
  // users should qslerp with this.
  double qTemp[4];
  // for now just use 100 ms
  const double cdDeltaT=0.100;
  
  // note, in tracker.cpp, Track.hiball_filter->last_time and
  // ts.Obs.time are set equal (by a call to time())
  // we want ori in cdDelta seconds, so ...
  Track.hiball_filter->orientation(&qTemp[0], 
				   Track.hiball_filter->last_time +
				   cdDeltaT );
  
  vel_quat[0] = qTemp[1];
  vel_quat[1] = qTemp[2];
  vel_quat[2] = qTemp[3];
  vel_quat[3] = qTemp[0];
  
  // pack in the delta time for the quat vel
  vel_quat_dt = cdDeltaT;
  
  len = encode_vel_to(msgbuf);
  if (connection->pack_message(len, timestamp, velocity_m_id, 
			       my_id, msgbuf, 
			       vrpn_CONNECTION_LOW_LATENCY)) {
    fprintf(stderr,"Ceiling tracker: can't write message: tossing\n");
  }
  
  // send second deriv -- none for now, but init them for the 
  // conglomerate packet
  
  acc[0] =  0;
  acc[1] =  0;
  acc[2] =  0;
  acc_quat[0] = 0;
  acc_quat[1] = 0;
  acc_quat[2] = 0;
  acc_quat[3] = 0;
  
  // pack in the delta time for the quat acc
  acc_quat_dt = cdDeltaT;
  
  // don't bother with message for now since we have no values
#if 0
  len = encode_acc_to(msgbuf);
  if (connection->pack_message(len, timestamp, accel_m_id, 
			       my_id, msgbuf, 
			       vrpn_CONNECTION_LOW_LATENCY)) {
    fprintf(stderr,"Ceiling tracker: can't write message: tossing\n");
  }
#endif
}
