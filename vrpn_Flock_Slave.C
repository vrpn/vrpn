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
#include "vrpn_Flock_Slave.h"

// output a status msg every status_msg_secs
#define STATUS_MSG
#define STATUS_MSG_SECS 600

void vrpn_Tracker_Flock_Slave::printError( unsigned char uchErrCode, 
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
    fprintf(stderr,"...No FBB Command Response from Device at address %d",
	    uchExpandedErrCode & 0x0f);
    fprintf(stderr,"\n\rExpanded Error Code Address is %u (decimal)",
	    uchExpandedErrCode & 0x0f);
    //    fprintf(stderr,"\n\rExpanded Error Code Command is %u (decimal)\n\r",
    //	    (unsigned char) ((exterrnum & ~fbbaddrbits) >> cmdbitshft));
    fprintf(stderr,"\n\rExpanded Error Code Command is %u (decimal)\n\r",
    	    (unsigned char) ((uchExpandedErrCode & ~0x0f) >> 4));
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
int vrpn_Tracker_Flock_Slave::checkError() {
  unsigned char rguch[4]={'B','G','O',10};

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (write(serial_fd, &rguch, 2 )!=2) {
    perror("\nvrpn_Tracker_Flock_Slave: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // clear in buffer
  vrpn_flush_input_buffer( serial_fd );

  // now get error code
  if (write(serial_fd, &(rguch[2]), 2 )!=2) {
    perror("\nvrpn_Tracker_Flock_Slave: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // read response
  int cRet;
  if ((cRet=read_available_characters(rguch, 2))!=2) {
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Slave: received only %d of 2 chars for err code", 
	    cRet);
    return -1;
  }

  if (rguch[0]) {
    printError( rguch[0], rguch[1] );
  }
  return rguch[0];
}

vrpn_Tracker_Flock_Slave::vrpn_Tracker_Flock_Slave(
			    char *name, vrpn_Connection *c, 
			    int sensorID, char *port, long baud):
  vrpn_Tracker_Serial(name,c,port,baud), cResets(0) {
  sensor = sensorID;  
  fprintf(stderr, "\nvrpn_Tracker_Flock_Slave: starting up ...\n");
  status = TRACKER_RESETTING;
  timestamp.tv_sec = -1;
  cSensors = 1;
  cSeconds = 3;
  fFirst =1; cReports = 0;
}


vrpn_Tracker_Flock_Slave::~vrpn_Tracker_Flock_Slave() {
  /*
  int cChars=1;
  unsigned char rgch[2]={'G'};

  fprintf(stderr,"\nvrpn_Tracker_FlockSlave %d: shutting down ...", sensor);
  // clear output buffer
  vrpn_flush_output_buffer( serial_fd );

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (write(serial_fd, &rgch, cChars )!=cChars) {
    perror("\nvrpn_Tracker_Flock: failed writing sleep cmd to tracker");
    status = TRACKER_FAIL;
    return;
  }
  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );
  fprintf(stderr, " done.\n");
  */
}


#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
  if (t2.tv_sec == -1) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}

void vrpn_Tracker_Flock_Slave::reset()
{
  // do nothing here. Some delay may be helpful??
  // sleep(1);
   // we are waitng for the output only

   status = TRACKER_SYNCING;	// We're trying for a new reading
   //fprintf(stderr, "reset\n");
   write(serial_fd, "B", 1);
   timestamp.tv_sec = -1;
   cResets=0;
}


void vrpn_Tracker_Flock_Slave::get_report(void)
{
   int ret;
   static int cSyncs=0;
   //fprintf(stderr, "I am here %d\n", sensor);
   // The reports are *14* bytes long each (Pos/Quat format without
   // group address), with a phasing bit set in the first byte 
   // of each sensor.
   // VRPN sends every tracker report for every sensor.
#define RECORD_SIZE 15-1 

   // We need to search for the phasing bit because if the input
   // buffer overflows then it will be emptied and overwritten.

   // If we're synching, read a byte at a time until we find
   // one with the high bit set.

   if (status == TRACKER_SYNCING) {
     
     // Try to get a character.  If none, just return.
     if (read_available_characters(buffer, 1) != 1) {
       return;
     }
     
     // If the high bit isn't set, we don't want it we
     // need to look at the next one, so just return
     if ( (buffer[0] & 0x80) == 0) {
       fprintf(stderr,"\nvrpn_Tracker_Flock_Slave: Syncing (high bit not set)");
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
     //fprintf(stderr, "Reading some thing %d\n", sensor);
     write(serial_fd, "B", 1); // poll back next record;
   }
     
   // Read as many bytes of this record as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the tracker hasn't simply
   // stopped sending characters).
   ret = read_available_characters(&buffer[bufcount], RECORD_SIZE-bufcount);
   if (ret == -1) {
     fprintf(stderr,"\nvrpn_Tracker_Flock_Slave: Error reading");
     status = TRACKER_FAIL;
     return;
   }
   bufcount += ret;
   if (bufcount < RECORD_SIZE) {	// Not done -- go back for more
     return;
   }
   
   // confirm that we are still synched ...
   if ( (buffer[0] & 0x80) == 0) {
     fprintf(stderr,"\nvrpn_Tracker_Flock_Slave: lost sync, resyncing.");
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
     quat[i-4] = (double)(rgs[i] * WTF);
   }
   quat[3] = (double)(rgs[3] * WTF);

   // all set for this sensor, so cycle
   status = TRACKER_REPORT_READY;
   bufcount = 0;
   if (fFirst == 1) {
     gettimeofday(&tvNow, NULL);
     tvLastPrint = tvNow;
     fFirst = 2;
   }

#ifdef VERBOSE
      print();
#endif
}


void vrpn_Tracker_Flock_Slave::mainloop()
{
#define VERBOSE
  switch (status) {
  case TRACKER_REPORT_READY:
    {
      
      // Send the message on the connection
      if (connection) {
	
	static char	msgbuf[1000];
	int	len = encode_to(msgbuf);

	// data to calc report rate 
	
	gettimeofday(&tvNow, NULL);
	
	cReports++;

#ifdef STATUS_MSG
	if (vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvLastPrint)) > cSeconds*1000){

	  if (fFirst==2) {
	    fprintf(stderr, "\nFlock: status will be printed every %d seconds",
		    STATUS_MSG_SECS);
	    fFirst = 0;
	  }

	  double dRate = cReports / 
	    (vrpn_TimevalMsecs(vrpn_TimevalDiff(tvNow, tvLastPrint))/1000.0);
	  time_t tNow = time(NULL);
	  char *pch = ctime(&tNow);
	  pch[24]='\0';
	  cSensors = 1; // one Flock Slave server 1 sensors;
	  fprintf(stderr, "\nFlock: reports being sent at %6.2lf hz (%d sensors, so ~%6.2lf hz per sensor) ( %s )", 
		  dRate, cSensors, dRate/cSensors, pch);
	  tvLastPrint = tvNow;
	  cReports=0;
	  // display the rate every X seconds
	  cSeconds=STATUS_MSG_SECS;
	}
#endif

#if 0
	fprintf(stderr,
		"\np/q (%d): ( %lf, %lf, %lf ) < %lf ( %lf, %lf, %lf ) >", 
		sensor, pos[0], pos[1], pos[2], 
		quat[3], quat[0], quat[1], quat[2] );
#endif

	if (connection->pack_message(len, timestamp,
				     position_m_id, my_id, msgbuf,
				     vrpn_CONNECTION_LOW_LATENCY)) {
	  fprintf(stderr,
		  "\nvrpn_Tracker_Flock_Slave: cannot write message ...  tossing");
	}
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock_Slave: No valid connection");
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
	fprintf(stderr,"\nvrpn_Tracker_Flock_Slave: failed to read ... current_time=%ld:%ld, timestamp=%ld:%ld",
		current_time.tv_sec, current_time.tv_usec, 
		timestamp.tv_sec, timestamp.tv_usec);
	status = TRACKER_FAIL;
      }
    }
  break;
  
  case TRACKER_RESETTING:
    reset();
    break;
    
  case TRACKER_FAIL:
    if (cResets==4) {
      char ch;
      checkError();
      fprintf(stderr, "\nvrpn_Tracker_Flock_Slave: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nPress return when the flock is ready to be re-started. ");
      fscanf(stdin, "%c", &ch);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Slave: tracker %d failed, trying to reset \n", sensor);
    close(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);
    status = TRACKER_RESETTING;
    cResets++;
    break;
  }
}






