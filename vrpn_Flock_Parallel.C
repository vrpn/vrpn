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

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef linux
#include <termios.h>
#endif

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Flock_Parallel.h"
#include "vrpn_Serial.h"

// output a status msg every status_msg_secs
#define STATUS_MSG
#define STATUS_MSG_SECS 600

// we create a vrpn_Tracker_Flock in polling mode as the master
vrpn_Tracker_Flock_Parallel::vrpn_Tracker_Flock_Parallel(char *name, 
							 vrpn_Connection *c, 
							 int cSensors, 
							 char *port, 
							 long baud,
							 char *slavePortArray[]) :
  vrpn_Tracker_Flock(name,c,cSensors,port,baud,0)
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
							   position_m_id,
							   i );
    }      
  }
}

vrpn_Tracker_Flock_Parallel::~vrpn_Tracker_Flock_Parallel() {

  // have all slaves shut down (they just send a 'B' -- slaves 
  // can't run or sleep
  for (int i=0;i<cSensors;i++) {
    delete rgSlaves[i];
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

void vrpn_Tracker_Flock_Parallel::get_report(void)
{
  // this could either do nothing or call slave get_report(s)
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
    if (rgSlaves[i]->status==TRACKER_FAIL) {
      status=TRACKER_FAIL;
      break;
    }
    // now check for reset being needed (cont to check for failures)
    if (rgSlaves[i]->status==TRACKER_RESETTING) {
      status=TRACKER_RESETTING;
      continue;
    }
  }
  
  // the master never has reports ready -- it is always either in
  // fail, reset, or sync mode
  // The slaves send messages on the master's connection
  switch (status) {
  case TRACKER_SYNCING:
    // everything is a-ok
  break;
  
  case TRACKER_RESETTING:
    reset();
    break;
    
  case TRACKER_FAIL:
    checkError();
    if (cResets==4) {
      fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nWill attempt to reset in 15 seconds.\n");
      sleep(15);
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
      rgSlaves[i]->status = TRACKER_RESETTING;
    }
    status = TRACKER_RESETTING;
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
				   vrpn_int32 positionMsgID,
				   int iSensorID ) :
  vrpn_Tracker_Flock(name,c,1,port,baud,1), vrpnMasterID(masterID),
  vrpnPositionMsgID(positionMsgID)
{
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
    status = TRACKER_FAIL;
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
  status = TRACKER_FAIL;\
  return;\
} \
gettimeofday(&timestamp, NULL);\
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
    status = TRACKER_FAIL;
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
      status = TRACKER_FAIL;
     return;
    }
    
    // make sure the commands are sent out
    vrpn_drain_output_buffer( serial_fd );
  } else {
    poll();
  }

  fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: "
	  "done with reset ... running.\n", d_sensor);
  
  gettimeofday(&timestamp, NULL);	// Set watchdog now
  status = TRACKER_SYNCING;	// We're trying for a new reading
}

// max time between start of a report and the finish (or time to 
// wait for first report)

// Allow enough time for startup of many sensors -- 1 second per sensor
#define MAX_TIME_INTERVAL       (MAX_SENSORS*1000000)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

void vrpn_Tracker_Flock_Parallel_Slave::mainloop()
{
  // We don't call the generic server mainloop code, because the master unit
  // will have done that for us.

  switch (status) {
  case TRACKER_REPORT_READY:
    {
      // NOTE: the flock behavior is very finicky -- if you try to poll
      //       again before most of the last response has been read, then
      //       it will complain.  You need to wait and issue the next
      //       group poll only once you have read out the previous one entirely
      //       As a result, polling is very slow with multiple sensors.
      if (fStream==0) {
	if (d_sensor==(cSensors-1)) { 
	  poll();
	}
      }

#ifdef	VERBOSE
      static int count = 0;
      if (count++ == 10) {
	printf("\nvrpn_Tracker_Flock_Parallel_Slave %d: Got report", d_sensor); print_latest_report();
	count = 0;
      }
#endif            
      // successful read, so reset the reset count
      cResets = 0;

      // Send the message on the connection
      if (d_connection) {

#ifdef STATUS_MSG
	// data to calc report rate 
	struct timeval tvNow;

	// get curr time
	gettimeofday(&tvNow, NULL);

	if (fFirstStatusReport) {
	  // print a status message in cStatusInterval seconds
	  cStatusInterval=3;
	  tvLastStatusReport=tvNow;
	  cReports=0;
	  fFirstStatusReport=0;
	  fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: "
		  "status will be printed every %d seconds.",
		  d_sensor, STATUS_MSG_SECS);
	}

	cReports++;

	if (vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvLastStatusReport)) > 
	    cStatusInterval*1000){

	  double dRate = cReports / 
	    (vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, 
						tvLastStatusReport))/1000.0);
	  time_t tNow = time(NULL);
	  char *pch = ctime(&tNow);
	  pch[24]='\0';
	  fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: "
		  "reports being sent at %6.2lf hz "
		  "(%d sensors, so ~%6.2lf hz per sensor) ( %s )", 
		  d_sensor, dRate, cSensors, dRate/cSensors, pch);
	  tvLastStatusReport = tvNow;
	  cReports=0;
	  // display the rate every STATUS_MSG_SECS seconds
	  cStatusInterval=STATUS_MSG_SECS;
	}
#endif

#if 0
	fprintf(stderr,
		"\np/q (%d): ( %lf, %lf, %lf ) < %lf ( %lf, %lf, %lf ) >", 
		d_sensor, pos[0], pos[1], pos[2], 
		d_quat[3], d_quat[0], d_quat[1], d_quat[2] );
#endif

	// pack and deliver tracker report
	static char msgbuf[1000];
	int	    len = encode_to(msgbuf);
	if (d_connection->pack_message(len, timestamp,
				     vrpnPositionMsgID, vrpnMasterID, msgbuf,
				     vrpn_CONNECTION_LOW_LATENCY)) {
	  fprintf(stderr,
		  "\nvrpn_Tracker_Flock_Parallel_Slave %d: cannot write message ...  tossing", d_sensor);
	}
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: No valid connection", d_sensor);
      }
      
      // Ready for another report
      status = TRACKER_SYNCING;
    }
  break;
  
  case TRACKER_SYNCING:
  case TRACKER_PARTIAL:
    {
      struct timeval current_time;

      gettimeofday(&current_time, NULL);
      if ( duration(current_time,timestamp) < MAX_TIME_INTERVAL) {
	get_report();
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock_Parallel_Slave %d: failed to read ... current_time=%ld:%ld, timestamp=%ld:%ld",
		d_sensor,
		current_time.tv_sec, current_time.tv_usec, 
		timestamp.tv_sec, timestamp.tv_usec);
	status = TRACKER_FAIL;
      }
    }
  break;
  // master resets us
  case TRACKER_RESETTING:
#if 0
    reset();
    if (fStream==0) {
      poll();
    }
#endif
    break;
  case TRACKER_FAIL:
    // here we just fail and let the master figure out that we have
    // failed and need to be reset
    fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: tracker "
	    "failed, trying to reset ...", d_sensor);
#if 0
    checkError();
    if (cResets==4) {
      fprintf(stderr, "\nvrpn_Tracker_Flock_Parallel_Slave %d: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nWill attempt to reset in 15 seconds.\n", d_sensor);
      sleep(15);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Parallel_Slave %d: tracker failed, trying to reset ...", d_sensor);
    vrpn_close_commport(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);
    status = TRACKER_RESETTING;
#endif
    break;
  }
}



/*****************************************************************************\
  $Log: vrpn_Flock_Parallel.C,v $
  Revision 1.11  2000/11/13 21:15:40  taylorr
  2000-11-13  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * vrpn_Flock_Parallel.C (mainloop) : Made the code close and then
                  reopen the serial ports (like it used to do before I turned
                  it off because of a buggy Linux kernel).

  Revision 1.10  2000/11/06 19:38:27  taylorr
  2000-11-03  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * vrpn_Shared.C (gettimeofday) : Turned off hi-perf clock for Win98
                  (vrpn_AdjustFrequency) : Removed buggy clock-check code

  2000-11-01  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * vrpn_FileConnection.C : Fixed parsing of file://.
          * vrpn_Connection.C : Removed more of the WINDOWS_GETHOSTBYNAME hack

  Revision 1.9  2000/08/28 16:24:52  taylorr
  2000-08-28  Russell M. Taylor II  <taylorr@cs.unc.edu>

          * Makefile : Added Brown patches to compile under AIX.
                  Cleaned up RANLIB and some dependencies.

          * vrpn_Shared.h : Added INVALID_SOCKET defs for Win32.

          * vrpn_Types.h : Added AIX definitions.

  Revision 1.1.1.2  2000/07/10 13:12:55  ddj
  VRPN 5.00 release sources

  Revision 1.8  2000/07/03 16:39:46  taylorr
  This takes us to vrpn 5.0, which includes several improvements over
  the previous versions, but means that everyone now needs to upgrade
  the servers and clients, since they won't be interoperable with older
  versions.

  Below is a list of the bug fixes and features, with the major ones
  indicated by an asterisk:

     bug	Removed the mainloop() calls from within the ForceDevice methods to set
  		parameters. This will allow them to be used from within callback
  		handlers.
     bug	Modified "delete msgbuf" commands to become "delete [] msgbuf", which
  		matches the fact that they were created with "new char [len]".
    *ftr	The timeout parameter has been removed from the mainloop() functions of
  		the object classes (such as Analog) that had it.  Waiting is
  		more properly done outside this function (using
  		vrpn_SleepMsecs()).
    *ftr	Make it so that vrpn_Text messages don't get sent as the
  		maximum-length text message
  		no matter how short the actual message is.  THIS FEATURE MAKES
  		THE CODE NON-INTEROPERABLE WITH VERSION 4 OF VRPN.  THIS
  		FEATURE WILL BREAK ANY CODE THAT USED vrpn_Text TO PASS
  		STRINGS THAT CONTAINED THE NULL CHARACTER.
    *bug	Fixed the packing/unpacking of vrpn_Bool so that it uses proper-length
  		byte ordering.  THIS FEATURE MAY MAKE THE CODE
  		NON-INTEROPERABLE WITH VERSION 4 OF VRPN.
     bug	vrpn_Text send_message() should not call mainloop() inside it.
  		Causes segfaults if messages are send inside a callback handler.
    *ftr	vrpn_BaseClass object created, from which all object types should
  		derive.  Most system object types now derive from this (clock,
  		analog, button, dial, forcedevice, sound, tracker, text).
  		This pulls sender_id registration and several other
  		functions together so that they are only implemented once.
  		Also allows extension of function (needed for the following
  		features) by extending only the vrpn_BaseClass object functions.
    *ftr	Text-handling code pulled into the vrpn_BaseClass object, so that all
  		objects can send text messages -- allows moving towards this way
  		of printing error messages from within objects.
    *ftr	vrpn_System_TextPrinter object created to print warnings and errors
  		from any created object (by default).  User can change the
  		ostream that it uses, as well as adjust its level of verbosity.
  		This should allow all objects to print errors/warnings to the
  		user console by emitting VRPN text messages.  This addresses
  		a long-standing problem with VRPN where you couldn't tell if
  		there was a problem with the
  		server because its error messages go to some console on another
  		machine.  Now they will go to both places for objects that are
  		derived from vrpn_BaseClass.
    *ftr	Handling the Ping/Pong messages that tell the client if the server is
  		alive The client warns of no hearbeat in 3 seconds, error if
  		no heartbeat in 10 second.  Warn/error 1/second.  Gets printed
  		by vrpn_System_TextPrinter.

  Revision 1.7  2000/03/30 19:23:19  hudson
  Big changes:
    VRPN Connection now supports multiple NICs.  An additional optional argument
  to every constructor call (incl. vrpn_get_connection_by_name) can be the
  IP address or DNS name for the NIC you want to use.

  Little changes:
    Changed the names of fields in vrpn_Tracker, vrpn_Sounds, vrpn_ForceDevice
  because they wre causing warnings on Solaris compilers and we're trying not
  to tolerate those any more.  Had to touch just about every single tracker
  implementation, since they read and write those fields directly.

  Revision 1.6  1999/07/19 15:33:03  helser
  Oops. There was an nm_int16 in the vrpn_Shared.h head file when it
  shouldn't be there. I also fixed a bunch of trivial compiler warnings
  generated by SGIs.

  Revision 1.5  1999/07/08 20:31:39  jclark
  Updated serial code to work on NT machines.

  Jason Clark 7/8/99

  Revision 1.4  1999/03/19 23:05:43  weberh
  modified the client and server classes so that they work with
  tom's changes to allow blocking vrpn mainloop calls (ie, mainloop
  signature changed).

  Revision 1.3  1999/02/24 15:58:33  taylorr
  BIG CHANGES.
  I modified the code so that it can compile on 64-bit SGI machines.

  To do so, I did what I should have done in the first place and defined
  architecture-independent types (vrpn_in32, vrpn_int16, vrpn_float32,
  vrpn_float64 and so on).  These are defined per architecture in the
  vrpn_Shared.h file.

  FROM NOW ON, folks should use these rather than the non-specific types
  (int, long, float, double) that may vary between platforms.

  Revision 1.2  1998/11/05 22:45:48  taylorr
  This version strips out the serial-port code into vrpn_Serial.C.

  It also makes it so all the library files compile under NT.

  It also fixes an insidious initialization bug in the forwarder code.

  Revision 1.1  1998/03/10 19:35:45  weberh
  The original parallel flock structure was a pain, so I redid it from
  scratch. The result is a new vrpn_Tracker_Flock_Parallel class and
  a corresponding slave class. Both leverage off of the base flock class
  heavily and avoid repeating code.  The master does all coordination and
  resetting, and the slaves just read reports.  In this way i am able to
  get 100hz from all of the sensors.

  I modified the makefile appropriately.

\*****************************************************************************/


