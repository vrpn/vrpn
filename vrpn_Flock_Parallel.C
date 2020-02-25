/*****************************************************************************\
  vrpn_Flock_Parallel.C
  --
  Description : A master and slave class are defined in this file.
                The master class connects to a flock controller and
		manages the slave receivers.

  ----------------------------------------------------------------------------
  Author: weberh
  Created: Thu Mar  5 19:38:55 1998
  Revised: Fri Mar 19 15:05:56 1999 by weberh
\*****************************************************************************/

// The structure of this code came from vrpn_3Space.[Ch]
// Most of the flock specific code comes from snippets in:
// ~hmd/src/libs/tracker/apps/ascension/FLOCK232/C
// That directory includes a program called cbird which allows you
// to send almost any command to the flock.
// If you are having trouble with the flock, compile and run that program.
// Things to remember:
//   Drain or flush i/o buffers as appropriate
//   If the flock is in stream mode ('@'), put it into point mode ('B')
//     before trying to send other commands
//   Even if you are running in group mode, you need to preface data
//     specification commands with the RS232TOFBB command.
//   (weberh 1/11/98)

// If you want to try polling instead of stream mode, just set define POLL
// #define POLL

#include <stdio.h>                      // for fprintf, stderr, NULL, etc

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Flock_Parallel.h"
#include "vrpn_Serial.h"                // for vrpn_drain_output_buffer, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc

class VRPN_API vrpn_Connection;

// output a status msg every status_msg_secs
#define STATUS_MSG
#define STATUS_MSG_SECS 600

// we create a vrpn_Tracker_Flock in polling mode as the master
vrpn_Tracker_Flock_Parallel::vrpn_Tracker_Flock_Parallel(char *name, 
							 vrpn_Connection *c, 
							 int cSensors, 
							 char *port, 
							 long baud,
							 char *slavePortArray[],
							 bool invertQuaternion) :
  vrpn_Tracker_Flock(name,c,cSensors,port,baud,0, 1, invertQuaternion)
{
  if (cSensors<=0) {
    fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: must ask for pos num "
	    "of sensors");
    cSensors = 0;
    return;
  }
  fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: starting up ...");

  if (!slavePortArray) {
    fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: null slave port array.");
    return;
  }

  for (int i=0;i<cSensors;i++) {
    char rgch[15];
    fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: initing slave %d ...", i);
    if (!slavePortArray[i]) {
      fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel:slave %d: null port array"
	      " entry.", i);
      return;
    } else {
      // create a traker name
      sprintf(rgch, "flockSlave%d", i);
      rgSlaves[i] = new vrpn_Tracker_Flock_Parallel_Slave( rgch,
							   d_connection,
							   slavePortArray[i],
							   baud,
							   d_sender_id,
							   i );
    }      
  }
}

vrpn_Tracker_Flock_Parallel::~vrpn_Tracker_Flock_Parallel() {

  // have all slaves shut down (they just send a 'B' -- slaves 
  // can't run or sleep
  for (int i=0;i<cSensors;i++) {
    try {
      delete rgSlaves[i];
    } catch (...) {
      fprintf(stderr, "vrpn_Tracker_Flock_Parallel::~vrpn_Tracker_Flock_Parallel(): delete failed\n");
      return;
    }
  }

  // now regular flock destructor will be called (just as we need)
}

// reset is a little different because we don't want it to go into
// stream mode.
void vrpn_Tracker_Flock_Parallel::reset()
{
   // call the base class reset (won't stream -- this is a polling 
   // vrpn_Tracker_Flock)
   vrpn_Tracker_Flock::reset();

   // Now, call slave reset(s)
   for (int i=0;i<cSensors;i++) {
     rgSlaves[i]->reset();
   }
}

int vrpn_Tracker_Flock_Parallel::get_report(void)
{
    // this could either do nothing or call slave get_report(s)
    return 0;
}

void vrpn_Tracker_Flock_Parallel::mainloop()
{
  int i;

  // Call the generic server code, since we are a server.
  server_mainloop();

  // call slave mainloops
  for (i=0;i<cSensors;i++) {
    rgSlaves[i]->mainloop();
  }

  // check slave status (master fails if any slave does
  // and the master resets the slaves)
  for (i=0;i<cSensors;i++) {
    // first check for failure
    if (rgSlaves[i]->status==vrpn_TRACKER_FAIL) {
      status=vrpn_TRACKER_FAIL;
      break;
    }
    // now check for reset being needed (cont to check for failures)
    if (rgSlaves[i]->status==vrpn_TRACKER_RESETTING) {
      status=vrpn_TRACKER_RESETTING;
      continue;
    }
  }
  
  // the master never has reports ready -- it is always either in
  // fail, reset, or sync mode
  // The slaves send messages on the master's connection
  switch (status) {
  case vrpn_TRACKER_SYNCING:
    // everything is a-ok
  break;
  
  case vrpn_TRACKER_RESETTING:
    reset();
    break;
    
  case vrpn_TRACKER_FAIL:
    checkError();
    if (cResets==4) {
      fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nWill attempt to reset in 15 seconds.\n");
      vrpn_SleepMsecs(1000.0*15);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Parallel: tracker failed, trying to reset ...");

    // reset master serial i/o
    vrpn_close_commport(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);

    // reset slave serial i/o
    for (i=0;i<cSensors;i++) {
      vrpn_close_commport(rgSlaves[i]->serial_fd);
      rgSlaves[i]->serial_fd = vrpn_open_commport(rgSlaves[i]->portname, 
						  rgSlaves[i]->baudrate);
      rgSlaves[i]->status = vrpn_TRACKER_RESETTING;
    }
    status = vrpn_TRACKER_RESETTING;
    break;
  }
}

/*******************************************************************************
  HERE IS THE VRPN SLAVE CODE
*******************************************************************************/


// stream mode flock, one sensor, same connection as master
// it will send faked messages from the master
vrpn_Tracker_Flock_Parallel_Slave::
vrpn_Tracker_Flock_Parallel_Slave( char *name, vrpn_Connection *c, 
				   char *port, long baud,
				   vrpn_int32 masterID,
				   int iSensorID ) :
  vrpn_Tracker_Flock(name,c,1,port,baud,1)
{
  d_sender_id = masterID;	// Spoofing the master
  d_sensor=iSensorID;
  fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: starting up ...",
	  d_sensor);
  // get_report should operate in non-group mode
  fGroupMode = 0;
}

vrpn_Tracker_Flock_Parallel_Slave::~vrpn_Tracker_Flock_Parallel_Slave() {

  // Some compilers (CenterLine CC on sunos!?) still don't support 
  // automatic aggregate initialization

  int cLen=0;
  //unsigned char rgch[2]={'B','G'};
  unsigned char rgch [2];
  rgch[cLen++] = 'B';

  // slaves can't do a sleep
  //  rgch[cLen++] = 'G';

  fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: shutting down ...", d_sensor);
  // clear output buffer
  vrpn_flush_output_buffer( serial_fd );

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (vrpn_write_characters(serial_fd, (const unsigned char *) rgch, cLen )!=cLen) {
    fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: "
	    "failed writing sleep cmd to tracker", d_sensor);
    status = vrpn_TRACKER_FAIL;
    return;
  }
  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );
  fprintf(stderr, " done.\n");
}

#define poll() { \
char chPoint = 'B';\
fprintf(stderr,"."); \
if (vrpn_write_characters(serial_fd, (const unsigned char *) &chPoint, 1 )!=1) {\
  fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: failed writing set mode cmds to tracker", d_sensor);\
  status = vrpn_TRACKER_FAIL;\
  return;\
} \
vrpn_gettimeofday(&timestamp, NULL);\
}   


// slave resets are ONLY called by the master
void vrpn_Tracker_Flock_Parallel_Slave::reset()
{
  // slaves just flush on a reset and then stream (or poll)
  // master does all the real resetting

  // set vars for error handling
  // set them right away so they are set properly in the
  // event that we fail during the reset.
  cResets++;
  cSyncs=0;
  fFirstStatusReport=1;

  // Get rid of the characters left over from before the reset
  // (make sure they are processed)
  vrpn_drain_output_buffer(serial_fd);

  // put back into polled mode (need to stop stream mode
  // before doing an auto-config)
  int resetLen=0;
  unsigned char reset[3];
  reset[resetLen++]='B';

  // send the poll mode command (cmd and cmd_size are args)
  if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
    fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: "
	    "failed writing poll cmd to tracker", d_sensor);
    status = vrpn_TRACKER_FAIL;
    return;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  // wait for tracker to respond and flush buffers
  vrpn_SleepMsecs(500);

  // clear the input buffer (it will contain a single point 
  // record from the poll command above and garbage from before reset)
  vrpn_flush_input_buffer(serial_fd);

  // now start it running
  resetLen = 0;

  // either stream or let poll take place later
  if (fStream==1) {
    // stream mode
    reset[resetLen++] = '@';

    if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
      fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: "
	      "failed writing set mode cmds to tracker", d_sensor);
      status = vrpn_TRACKER_FAIL;
     return;
    }
    
    // make sure the commands are sent out
    vrpn_drain_output_buffer( serial_fd );
  } else {
    poll();
  }

  fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: "
	  "done with reset ... running.\n", d_sensor);
  
  vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
  status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}

// max time between start of a report and the finish (or time to 
// wait for first report)

// Allow enough time for startup of many sensors -- 1 second per sensor
#define MAX_TIME_INTERVAL       (VRPN_FLOCK_MAX_SENSORS*1000000)

void vrpn_Tracker_Flock_Parallel_Slave::mainloop()
{
  // We don't call the generic server mainloop code, because the master unit
  // will have done that for us.

  switch (status) {
  case vrpn_TRACKER_SYNCING:
  case vrpn_TRACKER_PARTIAL:
    {
	// It turns out to be important to get the report before checking
	// to see if it has been too long since the last report.  This is
	// because there is the possibility that some other device running
	// in the same server may have taken a long time on its last pass
	// through mainloop().  Trackers that are resetting do this.  When
	// this happens, you can get an infinite loop -- where one tracker
	// resets and causes the other to timeout, and then it returns the
	// favor.  By checking for the report here, we reset the timestamp
	// if there is a report ready (ie, if THIS device is still operating).
	while (get_report()) {
	    send_report();
	}
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if ( vrpn_TimevalDuration(current_time,timestamp) > MAX_TIME_INTERVAL) {
		fprintf(stderr,"Tracker failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",
				current_time.tv_sec, static_cast<long>(current_time.tv_usec),
				timestamp.tv_sec, static_cast<long>(timestamp.tv_usec));
		send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
		status = vrpn_TRACKER_FAIL;
	}
    }
  break;
  // master resets us
  case vrpn_TRACKER_RESETTING:
    break;
  case vrpn_TRACKER_FAIL:
    // here we just fail and let the master figure out that we have
    // failed and need to be reset
    fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: tracker "
	    "failed, trying to reset ...", d_sensor);
    break;
  }
}
