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
#include "vrpn_Flock.h"
#include "vrpn_Serial.h"

// output a status msg every status_msg_secs
#define STATUS_MSG
#define STATUS_MSG_SECS 600

void vrpn_Tracker_Flock::printError( unsigned char uchErrCode, 
			unsigned char uchExpandedErrCode ) {

  fprintf(stderr,"\n\rError Code is %u (decimal) ", uchErrCode);
  /*
    Display a message describing the Error
    */
  switch (uchErrCode) {
  case 0:
    fprintf(stderr,"...No Errors Have Occurred");
    break;
  case 1:
    fprintf(stderr,"...System RAM Test Error");
    break;
  case 2:
    fprintf(stderr,"...Non-Volatile Storage Write Failure");
    break;
  case 3:
    fprintf(stderr,"...System EEPROM Configuration Corrupt");
    break;
  case 4:
    fprintf(stderr,"...Transmitter EEPROM Configuration Corrupt");
    break;
  case 5:
    fprintf(stderr,"...Receiver EEPROM Configuration Corrupt");
    break;
  case 6:
    fprintf(stderr,"...Invalid RS232 Command");
    break;
  case 7:
    fprintf(stderr,"...Not an FBB Master");
    break;
  case 8:
    fprintf(stderr,"...No 6DFOBs are Active");
    break;
  case 9:
    fprintf(stderr,"...6DFOB has not been Initialized");
    break;
  case 10:
    fprintf(stderr,"...FBB Receive Error - Intra Bird Bus");
    break;
  case 11:
    fprintf(stderr,"...RS232 Overrun and/or Framing Error");
    break;
  case 12:
    fprintf(stderr,"...FBB Receive Error - FBB Host Bus");
    break;
  case 13:
    fprintf(stderr,
	    "...No FBB Command Response from Device at address %d (decimal)",
	    uchExpandedErrCode & 0x0f);
    break;
  case 14:
    fprintf(stderr,"...Invalid FBB Host Command");
    break;
  case 15:
    fprintf(stderr,"...FBB Run Time Error");
    break;
  case 16:
    fprintf(stderr,"...Invalid CPU Speed");
    break;
  case 17:
    fprintf(stderr,"...Slave No Data Error");
    break;
  case 18:
    fprintf(stderr,"...Illegal Baud Rate");
    break;
  case 19:
    fprintf(stderr,"...Slave Acknowledge Error");
    break;
  case 20:
    fprintf(stderr,"...CPU Overflow Error - call factory");
    break;
  case 21:
    fprintf(stderr,"...Array Bounds Error - call factory");
    break;
  case 22:
    fprintf(stderr,"...Unused Opcode Error - call factory");
    break;
  case 23:
    fprintf(stderr,"...Escape Opcode Error - call factory");
    break;
  case 24:
    fprintf(stderr,"...Reserved Int 9 - call factory");
    break;
  case 25:
    fprintf(stderr,"...Reserved Int 10 - call factory");
    break;
  case 26:
    fprintf(stderr,"...Reserved Int 11 - call factory");
    break;
  case 27:
    fprintf(stderr,"...Numeric CPU Error - call factory");
    break;
  case 28:
    fprintf(stderr,"...CRT Syncronization Error");
    break;
  case 29:
    fprintf(stderr,"...Transmitter Not Active Error");
    break;
  case 30:
    fprintf(stderr,"...ERC Extended Range Transmitter Not Attached Error");
    break;
  case 31:
    fprintf(stderr,"...CPU Time Overflow Error");
    break;
  case 32:
    fprintf(stderr,"...Receiver Saturated Error");
    break;
  case 33:
    fprintf(stderr,"...Slave Configuration Error");
    break;
  case 34:
    fprintf(stderr,"...ERC Watchdog Error");
    break;
  case 35:
    fprintf(stderr,"...ERC Overtemp Error");
    break;
  default:
    fprintf(stderr,"...UNKNOWN ERROR... check user manual");
    break;
  }
}

// check for flock error, return err number if there is an error
// zero if no error
int vrpn_Tracker_Flock::checkError() {

  // Some compilers (CenterLine CC on sunos!?) still don't support 
  // automatic aggregate initialization

  //unsigned char rguch[4]={'B','G','O',10};

  int cLen=0;
  unsigned char rguch[2];
  rguch[cLen++] = 'B';
  rguch[cLen++] = 'G';

  // put the flock to sleep (B to get out of stream mode, G to sleep)

  if (vrpn_write_characters(serial_fd, (const unsigned char *) &rguch, cLen )!=cLen) {
    perror("\nvrpn_Tracker_Flock: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  vrpn_SleepMsecs(500);

  // clear in buffer
  vrpn_flush_input_buffer( serial_fd );

  // now get error code and clear error status
  // we want error code 16, not 10 -- we want the expanded error code
  // prepare error status query (expanded error codes)
  cLen=0;
  rguch[cLen++] = 'O';
  rguch[cLen++] = 16;
  if (vrpn_write_characters(serial_fd, (const unsigned char *)rguch, cLen )!=cLen) {
    perror("\nvrpn_Tracker_Flock: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  vrpn_SleepMsecs(500);

  // read response (2 char response to error query 16),
  // 1 char response to 10
  int cRet;
  if ((cRet=vrpn_read_available_characters(serial_fd, rguch, 2))!=2) {
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock: received only %d of 2 chars for err code", 
	    cRet);
    return -1;
  }

  // if first byte is 0, there is no error
  //  if (rguch[0]) {
  printError( rguch[0], rguch[1] );
  //  }
  return rguch[0];
}

vrpn_Tracker_Flock::vrpn_Tracker_Flock(char *name, vrpn_Connection *c, 
				       int cSensors, char *port, long baud,
				       int fStreamMode ) :
  vrpn_Tracker_Serial(name,c,port,baud), cSensors(cSensors), cResets(0),
  fStream(fStreamMode), fGroupMode(1), cSyncs(0), fFirstStatusReport(1) {
    if (cSensors>MAX_SENSORS) {
      fprintf(stderr, "\nvrpn_Tracker_Flock: too many sensors requested ... only %d allowed (%d specified)", MAX_SENSORS, cSensors );
      cSensors = MAX_SENSORS;
    }
    fprintf(stderr, "\nvrpn_Tracker_Flock: starting up ...");
}


double vrpn_Tracker_Flock::getMeasurementRate() {
  // the cbird code shows how to read these
  int resetLen = 0;
  unsigned char reset[5];
  unsigned char response[5];

  // get crystal freq and measurement rate
  reset[resetLen++]='O';
  reset[resetLen++]=2;
  reset[resetLen++]='O';
  reset[resetLen++]=6;

  if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
    perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");
    status = TRACKER_FAIL;
    return 1;
  }
  
  // make sure the commands are sent out
  vrpn_drain_output_buffer( serial_fd );
  
  // let the tracker respond
  vrpn_SleepMsecs(500);
  
  int cRetF;
   if ((cRetF=vrpn_read_available_characters(serial_fd, response, 4))!=4) {
     fprintf(stderr, 
	     "\nvrpn_Tracker_Flock: received only %d of 4 chars as freq", 
	     cRetF);
     status = TRACKER_FAIL;
     return 1;
   }
   
   fprintf(stderr, "\nvrpn_Tracker_Flock: crystal freq is %d Mhz", 
	   (unsigned int)response[0]);
   unsigned int iCount = response[2] + (((unsigned int)response[3]) << 8);
   return (1000.0/((4*(iCount*(8.0/response[0])/1000.0)) + 0.3));
}

vrpn_Tracker_Flock::~vrpn_Tracker_Flock() {

  // Some compilers (CenterLine CC on sunos!?) still don't support 
  // automatic aggregate initialization

  int cLen=0;
  //unsigned char rgch[2]={'B','G'};
  unsigned char rgch [2];
  rgch[cLen++] = 'B';
  rgch[cLen++] = 'G';

  fprintf(stderr,"\nvrpn_Tracker_Flock: shutting down ...");
  // clear output buffer
  vrpn_flush_output_buffer( serial_fd );

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (vrpn_write_characters(serial_fd, (const unsigned char *) rgch, cLen )!=cLen) {
    perror("\nvrpn_Tracker_Flock: failed writing sleep cmd to tracker");
    status = TRACKER_FAIL;
    return;
  }
  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );
  fprintf(stderr, " done.\n");
}

void vrpn_Tracker_Flock::reset()
{
   int i;
   int resetLen;
   unsigned char reset[6*(MAX_SENSORS+1)+10];

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
   resetLen=0;
   reset[resetLen++]='B';

   // send the poll mode command (cmd and cmd_size are args)
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing poll cmd to tracker");
     status = TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // wait for tracker to respond and flush buffers
   vrpn_SleepMsecs(500);

   // Send the tracker a string that should reset it.

   // we will try to do an auto-reconfigure of all units of the flock
   // then set the flock into pos/quat stream mode (this requires group
   // mode, so we set that as well).

   resetLen=0;

   // "change value" cmd
   reset[resetLen++]='P';

   // flock of birds auto-configure code
   reset[resetLen++]=50;

   // number of units (xmitter + receivers)
   // eventually this will be an arg of some sort
   reset[resetLen++]= cSensors+1;

   // as per pg 59 of the jan 31, 1995 FOB manual, we need to pause at
   // least 300 ms before and after sending the autoconfig (paused above)

   // send the reset command (cmd and cmd_size are args)
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing auto-config to tracker");
     status = TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // wait for auto reconfig
   vrpn_SleepMsecs(500);

   // now set modes: pos/quat, group, stream
   resetLen=0;

   // group mode
   reset[resetLen++] = 'P';
   reset[resetLen++] = 35;
   reset[resetLen++] = 1;
   // pos/quat mode sent to each receiver (transmitter is unit 1)
   // 0xf0 + addr is the cmd to tell the master to forward a cmd
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = 0xf0 + i + 1;
     reset[resetLen++] = ']';
   }

   // set to lower hemisphere, send cmd to each receiver
   // as above, first part is rs232 to fbb, 'L' is hemisphere
   // 0xC is the 'axis' (lower), and 0x0 is the 'sign' (lower)
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = 0xf0 + i + 1;
     reset[resetLen++] = 'L';
     reset[resetLen++] = 0xc;
     reset[resetLen++] = 0;
   }

   // write it all out
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");
     status = TRACKER_FAIL;
     return;
   }
   
   // make sure the commands are sent out
   vrpn_drain_output_buffer( serial_fd );

   // clear the input buffer (it will contain a single point 
   // record from the poll command above and garbage from before reset)
   vrpn_flush_input_buffer(serial_fd);

   resetLen=0;
   // get the system status to check that it opened correctly
   // examine value cmd is 'O'
   reset[resetLen++]='O';
   
   // flock system status is 36
   reset[resetLen++]=36;

   // write the cmd and get response
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing get sys config to tracker");
     status = TRACKER_FAIL;
     return;
   }

   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // let the tracker respond
   vrpn_SleepMsecs(500);

   unsigned char response[14];
   int cRet;
   if ((cRet=vrpn_read_available_characters(serial_fd, response, 14))!=14) {
     fprintf(stderr, 
	     "\nvrpn_Tracker_Flock: received only %d of 14 chars as status", 
	     cRet);
     status = TRACKER_FAIL;
     return;
   }
   
   // check the configuration ...
   int fOk=1;

   for (i=0;i<=cSensors;i++) {
     fprintf(stderr, "\nvrpn_Tracker_Flock: unit %d", i);
     if (response[i] & 0x20) {
       fprintf(stderr," (a receiver)");
     } else {
       fprintf(stderr," (a transmitter)");
     }
     if (response[i] & 0x80) {
       fprintf(stderr," is accessible");
     } else {
       fprintf(stderr," is not accessible");
       fOk=0;
     }
     if (response[i] & 0x40) {
       fprintf(stderr," and is running");
     } else {
       fprintf(stderr," and is not running");
       fOk=0;
     }
   }
   
   fprintf(stderr, "\n");

   if (!fOk) {
     perror("\nvrpn_Tracker_Flock: problems resetting tracker.");
     status = TRACKER_FAIL;
     return;
   }

#define GET_FREQ
#ifdef GET_FREQ
  fprintf(stderr, "\nvrpn_Tracker_Flock: sensor measurement rate is %lf hz.",
	  getMeasurementRate());
#endif

   // now start it running
   resetLen = 0;

   if (fStream==1) {
     // stream mode
     reset[resetLen++] = '@';
     if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
       perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");
       status = TRACKER_FAIL;
       return;
     }
     
     // make sure the commands are sent out
     vrpn_drain_output_buffer( serial_fd );
   }

   fprintf(stderr,"\nvrpn_Tracker_Flock: done with reset ... running.\n");

   gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = TRACKER_SYNCING;	// We're trying for a new reading
}


void vrpn_Tracker_Flock::get_report(void)
{
   int ret;
   unsigned RECORD_SIZE = 15;

   // The reports are 15 bytes long each (Pos/Quat format plus
   // group address), with a phasing bit set in the first byte 
   // of each sensor.
   // If not in group mode, then reports are just 14 bytes
   // VRPN sends every tracker report for every sensor.

   if (!fGroupMode) {
     RECORD_SIZE = 14;
   }
     
   // We need to search for the phasing bit because if the input
   // buffer overflows then it will be emptied and overwritten.

   // If we're synching, read a byte at a time until we find
   // one with the high bit set.

   if (status == TRACKER_SYNCING) {
     // Try to get a character.  If none, just return.
     if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
       return;
     }
     
     // If the high bit isn't set, we don't want it we
     // need to look at the next one, so just return
     if ( (buffer[0] & 0x80) == 0) {
       fprintf(stderr,"\nvrpn_Tracker_Flock: Syncing (high bit not set)");
       cSyncs++;
       if (cSyncs>10) {
	 // reset the tracker if more than a few syncs occur
	 status = TRACKER_RESETTING;
       }
       return;
     }
     cSyncs=0;

     // Got the first character of a report -- go into PARTIAL mode
     // and say that we got one character at this time.
     bufcount = 1;
     gettimeofday(&timestamp, NULL);
     status = TRACKER_PARTIAL;
   }
     
   // Read as many bytes of this record as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the tracker hasn't simply
   // stopped sending characters).
   ret = vrpn_read_available_characters(serial_fd, &buffer[bufcount],
		RECORD_SIZE-bufcount);
   if (ret == -1) {
     fprintf(stderr,"\nvrpn_Tracker_Flock: Error reading");
     status = TRACKER_FAIL;
     return;
   }
   bufcount += ret;
   if (bufcount < RECORD_SIZE) {	// Not done -- go back for more
     return;
   }
   
   // confirm that we are still synched ...
   if ( (buffer[0] & 0x80) == 0) {
     fprintf(stderr,"\nvrpn_Tracker_Flock: lost sync, resyncing.");
     status = TRACKER_SYNCING;
     return;
   }
   
   // Now decode the report
   
   // from the flock manual (page 28) and ascension's code ...

   // they use 14 bit ints
   short *rgs= (short *)buffer;
   short cs = RECORD_SIZE/2;

   // Go though the flock data and make into two's complemented
   // 16 bit integers by:
   //  1) getting rid of the lsb phasing bit
   //  2) shifting the lsbyte left one bit
   //  3) recombining msb/lsb into words
   //  4) shifting each word left one more bit
   // These are then scaled appropriately.

   unsigned char uchLsb;
   unsigned char uchMsb;

   for (int irgs=0;irgs<cs;irgs++) {
     // The data is dealt with as bytes so that the host byte ordering
     // will not affect the operation
     uchLsb = buffer[irgs*2] & 0x7F;
     uchLsb <<= 1;
     uchMsb = buffer[irgs*2+1];
     rgs[irgs] = ((unsigned short) uchLsb) + (((unsigned short) uchMsb) << 8);
     rgs[irgs] <<= 1;
   }

   // scale factor for position 
#define POSK144 (float)(144.0/32768.0)    /* integer to inches ER Controller */

   int i;
   for (i=0;i<3;i++) {
     // scale and convert to meters
     pos[i] = (double)(rgs[i] * POSK144 * 0.0254);
   }
   
   // they code quats as w,x,y,z, we need to give out x,y,z,w
   // The quats are already normalized
#define WTF (float)(1.0/32768.0)                    /* float to word integer */
   for (i=4;i<7;i++) {
     //     quat[i-4] = (double)(((short *)buffer)[i] * WTF);
     d_quat[i-4] = (double)(rgs[i] * WTF);
   }
   d_quat[3] = (double)(rgs[3] * WTF);

   // KEY: the flock is unusual -- most trackers report back the
   //      transform xfSourceFromSensor, which you can apply
   //      to points in sensor space to get the coords of those
   //      points in src space.  The flock is strange, it reports
   //      the pos of the sensor in the src space (fine), and
   //      then it reports how to rotate the sensor axes to
   //      coincide with the src axes -- the inverse of the quat
   //      needed for xfSrcFromSensor.  So that the flock is 
   //      consistent with all other trackers, we invert the quat
   //      here (since we assume quat is normalized, we can form 
   //      the inverse by rotating about the opp axis)
   
   d_quat[0] = -d_quat[0];
   d_quat[1] = -d_quat[1];
   d_quat[2] = -d_quat[2];

   if (fGroupMode) {
     // sensor addr are 0 indexed in vrpn, but start at 2 in the flock 
     // (1 is the transmitter)
     d_sensor = buffer[RECORD_SIZE-1]-2;
   }      

   // all set for this sensor, so cycle
   status = TRACKER_REPORT_READY;
   bufcount = 0;

#ifdef VERBOSE
      print_latest_report();
#endif
}

#define poll() { \
char chPoint = 'B';\
fprintf(stderr,"."); \
if (vrpn_write_characters(serial_fd, (const unsigned char *) &chPoint, 1 )!=1) {\
  perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");\
  status = TRACKER_FAIL;\
  return;\
} \
gettimeofday(&timestamp, NULL);\
}   

// max time between start of a report and the finish (or time to 
// wait for first report)
#define MAX_TIME_INTERVAL       (2000000)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

void vrpn_Tracker_Flock::mainloop()
{
  // Call the generic server mainloop, because we are a server
  server_mainloop();

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
	printf("\nvrpn_Tracker_Flock: Got report"); print_latest_report();
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
	  fprintf(stderr, "\nFlock: status will be printed every %d seconds.",
		  STATUS_MSG_SECS);
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
	  fprintf(stderr, "\nFlock: reports being sent at %6.2lf hz "
		  "(%d sensors, so ~%6.2lf hz per sensor) ( %s )", 
		  dRate, cSensors, dRate/cSensors, pch);
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
				     position_m_id, d_sender_id, msgbuf,
				     vrpn_CONNECTION_LOW_LATENCY)) {
	  fprintf(stderr,
		  "\nvrpn_Tracker_Flock: cannot write message ...  tossing");
	}
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock: No valid connection");
      }
      
#if 0
// polling for debugging purposes
      if (d_sensor==(cSensors-1)) {
	// poll again
	char ch;
	
	fprintf(stderr, "\nPoll again? ");
	fscanf(stdin, "%c", &ch);
	if (ch=='y') {
	  ch='B';
	  
	  // send the poll mode command (cmd and cmd_size are args)
	  if (vrpn_write_characters(serial_fd, (unsigned char*)&ch, 1 )!=1) {
	    perror("\nvrpn_Tracker_Flock: failed writing poll cmd to tracker");
	    status = TRACKER_FAIL;
	    return;
	  }
	  // make sure the command is sent out
	  vrpn_drain_output_buffer( serial_fd );
	}
      }
#endif
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
	fprintf(stderr,"\nvrpn_Tracker_Flock: failed to read ... current_time=%ld:%ld, timestamp=%ld:%ld",
		current_time.tv_sec, current_time.tv_usec, 
		timestamp.tv_sec, timestamp.tv_usec);
	status = TRACKER_FAIL;
      }
    }
  break;
  
  case TRACKER_RESETTING:
    reset();
    if (fStream==0) {
      poll();
    }
    break;
    
  case TRACKER_FAIL:
    checkError();
    if (cResets==4) {
      fprintf(stderr, "\nvrpn_Tracker_Flock: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nWill attempt to reset in 15 seconds.\n");
      sleep(15);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock: tracker failed, trying to reset ...");
    vrpn_close_commport(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);
    status = TRACKER_RESETTING;
    break;
  }
}


