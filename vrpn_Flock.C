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

#include <stdio.h>                      // for fprintf, stderr, perror, etc
#include <time.h>                       // for ctime, time, time_t

#include "quat.h"                       // for q_invert
#include "vrpn_Flock.h"
#include "vrpn_Serial.h"                // for vrpn_drain_output_buffer, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_Types.h"                 // for vrpn_float64

class VRPN_API vrpn_Connection;

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
    fprintf(stderr,"...CRT Synchronization Error");
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
				       int cSensors, const char *port, long baud,
				       int fStreamMode, int useERT, bool invertQuaternion, int active_hemisphere)
  : vrpn_Tracker_Serial(name,c,port,baud)
  , activeHemisphere(active_hemisphere)
  , cSensors(cSensors)
  , fStream(fStreamMode)
  , fGroupMode(1)
  , d_useERT(useERT)
  , d_invertQuaternion(invertQuaternion)
  , cResets(0)
  , cSyncs(0)
  , fFirstStatusReport(1)
{
    if (cSensors>VRPN_FLOCK_MAX_SENSORS) {
      fprintf(stderr, "\nvrpn_Tracker_Flock: too many sensors requested ... only %d allowed (%d specified)", VRPN_FLOCK_MAX_SENSORS, cSensors );
      cSensors = VRPN_FLOCK_MAX_SENSORS;
    }
    fprintf(stderr, "\nvrpn_Tracker_Flock: starting up (FOBHack)...");
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
    status = vrpn_TRACKER_FAIL;
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
     status = vrpn_TRACKER_FAIL;
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
    status = vrpn_TRACKER_FAIL;
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
   unsigned char reset[6*(VRPN_FLOCK_MAX_SENSORS+1)+10];

   // If the RTS/CTS pins are in the cable that connects the Flock
   // to the computer, we need to raise and drop the RTS/CTS line
   // to make the communications on the Flock reset.  We need to give
   // it time to reset.  If these wires are not installed (ie, if only
   // send, receive and ground are connected) then we don't need this.
   // To be more general, we put it in.  The following code snippet
   // comes from Kyle at Ascension.
   vrpn_set_rts( serial_fd );
   vrpn_SleepMsecs(1000);
   vrpn_clear_rts( serial_fd );
   vrpn_SleepMsecs(2000);

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
   fprintf(stderr,"  vrpn_Flock: Sending POLL mode command...\n");
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing poll cmd to tracker");
     status = vrpn_TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // wait for tracker to respond and flush buffers
   vrpn_SleepMsecs(5000);

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
   reset[resetLen++]= (unsigned char)(cSensors+d_useERT);

   // as per pg 59 of the jan 31, 1995 FOB manual, we need to pause at
   // least 300 ms before and after sending the autoconfig (paused above)

   // send the reset command (cmd and cmd_size are args)
   fprintf(stderr,"  vrpn_Flock: Sending RESET command...\n");
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing auto-config to tracker");
     status = vrpn_TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // wait for auto reconfig
   vrpn_SleepMsecs(10000);

   // now set modes: pos/quat, group, stream
   resetLen=0;

   // group mode
   reset[resetLen++] = 'P';
   reset[resetLen++] = 35;
   reset[resetLen++] = 1;
   // pos/quat mode sent to each receiver (transmitter is unit 1)
   // 0xf0 + addr is the cmd to tell the master to forward a cmd
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = (unsigned char)(0xf0 + i + d_useERT);
     reset[resetLen++] = ']';
   }

   //
   // Set the active hemisphere based on what's in the config file.

   unsigned char hem, sign;
   switch (activeHemisphere)
   {
       case HEMI_PLUSX :   hem = 0x00; sign = 0x00;  break;
       case HEMI_MINUSX:   hem = 0x00; sign = 0x01;  break;
       case HEMI_PLUSY :   hem = 0x06; sign = 0x00;  break;
       case HEMI_MINUSY:   hem = 0x06; sign = 0x01;  break;
       case HEMI_PLUSZ :   hem = 0x0c; sign = 0x00;  break;
       case HEMI_MINUSZ:   hem = 0x0c; sign = 0x01;  break;
   }
   // prepare the command
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = (unsigned char)(0xf0 + i + d_useERT);
     reset[resetLen++] = 'L';
     reset[resetLen++] = hem;
     reset[resetLen++] = sign;
   }

   // write it all out
   fprintf(stderr,"  vrpn_Flock: Setting parameters...\n");
   if (vrpn_write_characters(serial_fd, (const unsigned char *) reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");
     status = vrpn_TRACKER_FAIL;
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
     status = vrpn_TRACKER_FAIL;
     return;
   }

   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // let the tracker respond
   vrpn_SleepMsecs(500);

   fprintf(stderr,"  vrpn_Flock: Checking for response...\n");
   unsigned char response[14];
   int cRet;
   if ((cRet=vrpn_read_available_characters(serial_fd, response, 14))!=14) {
     fprintf(stderr, 
	     "\nvrpn_Tracker_Flock: received only %d of 14 chars as status", 
	     cRet);
     status = vrpn_TRACKER_FAIL;
     return;
   }
   
   // check the configuration ...
   int fOk=1;
   for (i=0;i<=cSensors-1+d_useERT;i++) {
     fprintf(stderr, "\nvrpn_Tracker_Flock: unit %d", i);
     if (response[i] & 0x20) {
       fprintf(stderr," (a receiver)");
     } else {
       fprintf(stderr," (a transmitter)");
// now we allow non transmitters at fisrt address !!!!
//       if (i != 0) {
//	   fprintf(stderr,"\nError: VRPN Flock driver can only accept transmitter as first unit\n");
//	   status = vrpn_TRACKER_FAIL;
//	   fOk=0;
//	   return;
//     }
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
     status = vrpn_TRACKER_FAIL;
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
       status = vrpn_TRACKER_FAIL;
       return;
     }
     
     // make sure the commands are sent out
     vrpn_drain_output_buffer( serial_fd );
   }

   fprintf(stderr,"\nvrpn_Tracker_Flock: done with reset ... running.\n");

   vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}


int vrpn_Tracker_Flock::get_report(void)
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

   if (status == vrpn_TRACKER_SYNCING) {
     // Try to get a character.  If none, just return.
     if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
       return 0;
     }
     
     // If the high bit isn't set, we don't want it we
     // need to look at the next one, so just return
     if ( (buffer[0] & 0x80) == 0) {
       fprintf(stderr,"\nvrpn_Tracker_Flock: Syncing (high bit not set)");
       cSyncs++;
       if (cSyncs>RECORD_SIZE) {
	 // reset the tracker if more than a few syncs occur
	 status = vrpn_TRACKER_RESETTING;
       }
       return 0;
     }
     cSyncs=0;

     // Got the first character of a report -- go into PARTIAL mode
     // and say that we got one character at this time.
     bufcount = 1;
     vrpn_gettimeofday(&timestamp, NULL);
     status = vrpn_TRACKER_PARTIAL;
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
     status = vrpn_TRACKER_FAIL;
     return 0;
   }
   bufcount += ret;
   if (bufcount < RECORD_SIZE) {	// Not done -- go back for more
     return 0;
   }
   
   // confirm that we are still synched ...
   if ( (buffer[0] & 0x80) == 0) {
     fprintf(stderr,"\nvrpn_Tracker_Flock: lost sync, resyncing.");
     status = vrpn_TRACKER_SYNCING;
     return 0;
   }
   
   // Now decode the report
   
   // from the flock manual (page 28) and ascension's code ...

   // they use 14 bit ints
   short *rgs= (short *)buffer;
   short cs = (short)(RECORD_SIZE/2);

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
     uchLsb = (unsigned char)(buffer[irgs*2] & 0x7F);
     uchLsb <<= 1;
     uchMsb = buffer[irgs*2+1];
     rgs[irgs] = (short)( ((unsigned short) uchLsb) + (((unsigned short) uchMsb) << 8) );
     rgs[irgs] <<= 1;
   }

   // scale factor for position.
   // According to Jo Skermo, this depends on whether we're using the
   // extended-range transmitter or not.
   double int_to_inches;
   if (d_useERT) {
     int_to_inches = 144.0/32768.0;
   } else {
     int_to_inches = 36.0/32768.0;
   }

   int i;
   for (i=0;i<3;i++) {
     // scale and convert to meters
     pos[i] = (double)(rgs[i] * int_to_inches * 0.0254);
   }
   
   // they code quats as w,x,y,z, we need to give out x,y,z,w
   // The quats are already normalized
#define WTF (float)(1.0/32768.0)                    /* float to word integer */
   for (i=4;i<7;i++) {
     //     quat[i-4] = (double)(((short *)buffer)[i] * WTF);
     d_quat[i-4] = (double)(rgs[i] * WTF);
   }
   d_quat[3] = (double)(rgs[3] * WTF);

   // Because the Flock was used at UNC by having the transmitter
   // above the user, we were always operating in the wrong hemisphere.
   // According to the Flock manual, the Flock should report things
   // in the same way other trackers do.  The original code inverted
   // the Quaternion value before returning it, probably because of this
   // hemisphere problem.  Sebastien Maraux pointed out the confusion.
   // To Enable either mode to work, the code now has an optional parameter
   // that will invert the quaternion, but it is not the default anymore.
   // Others must have been using the Flock in the same manner, because
   // they didn't see a problem with this code.
   
   if (d_invertQuaternion) { q_invert(d_quat, d_quat); }

   if (fGroupMode) {
     // sensor addr are 0 indexed in vrpn, but start at 2 in the flock 
     // (1 is the transmitter)
//     d_sensor = buffer[RECORD_SIZE-1]-2;
     d_sensor = buffer[RECORD_SIZE-1]-1-d_useERT;
   }      

   // all set for this sensor, so cycle
   status = vrpn_TRACKER_SYNCING;
   bufcount = 0;

#ifdef VERBOSE
      print_latest_report();
#endif
    return 1;
}

#define poll() { \
char chPoint = 'B';\
fprintf(stderr,"."); \
if (vrpn_write_characters(serial_fd, (const unsigned char *) &chPoint, 1 )!=1) {\
  perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");\
  status = vrpn_TRACKER_FAIL;\
  return;\
} \
vrpn_gettimeofday(&timestamp, NULL);\
}   

static const char* vrpn_ctime_r(time_t* cur_time, char* buffer, size_t bufsize)
{
#if _WIN32
  if (0 != ctime_s(buffer, bufsize, cur_time)) {
    return NULL;
  }
  return buffer;
#else
  return ctime_r(cur_time, buffer);
#endif
}

void	vrpn_Tracker_Flock::send_report(void) {
    vrpn_Tracker_Serial::send_report();

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

    // successful read, so reset the reset count
    cResets = 0;

#ifdef STATUS_MSG
	// data to calc report rate 
	struct timeval tvNow;

	// get curr time
	vrpn_gettimeofday(&tvNow, NULL);

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
    char pch[28];
    memset(pch, 0, sizeof(pch));
	  vrpn_ctime_r(&tNow, pch, sizeof(pch));
	  fprintf(stderr, "\nFlock: reports being sent at %6.2lf hz "
		  "(%d sensors, so ~%6.2lf hz per sensor) ( %s )", 
		  dRate, cSensors, dRate/cSensors, pch);
	  tvLastStatusReport = tvNow;
	  cReports=0;
	  // display the rate every STATUS_MSG_SECS seconds
	  cStatusInterval=STATUS_MSG_SECS;
	}
#endif
}
