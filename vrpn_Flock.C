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
int vrpn_Tracker_Flock::checkError() {
  unsigned char rguch[4]={'B','G','O',10};

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (write(serial_fd, &rguch, 2 )!=2) {
    perror("\nvrpn_Tracker_Flock: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // clear in buffer
  vrpn_flush_input_buffer( serial_fd );

  // now get error code
  if (write(serial_fd, &(rguch[2]), 2 )!=2) {
    perror("\nvrpn_Tracker_Flock: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // read response
  int cRet;
  if ((cRet=read_available_characters(rguch, 2))!=2) {
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock: received only %d of 2 chars for err code", 
	    cRet);
    return -1;
  }

  if (rguch[0]) {
    printError( rguch[0], rguch[1] );
  }
  return rguch[0];
}

vrpn_Tracker_Flock::vrpn_Tracker_Flock(char *name, vrpn_Connection *c, 
				       int cSensors, char *port, long baud):
  vrpn_Tracker_Serial(name,c,port,baud), cSensors(cSensors), cResets(0) {
    fprintf(stderr, "\nvrpn_Tracker_Flock: starting up ...");
}


vrpn_Tracker_Flock::~vrpn_Tracker_Flock() {
  int cChars=2;
  unsigned char rgch[2]={'B','G'};

  fprintf(stderr,"\nvrpn_Tracker_Flock: shutting down ...");
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
}


#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

void vrpn_Tracker_Flock::reset()
{
   int resetLen;
   unsigned char reset[2*(cSensors+1)+10];

   // Get rid of the characters left over from before the reset
   vrpn_flush_input_buffer(serial_fd);
   vrpn_flush_output_buffer(serial_fd);

   // put back into polled mode (need to stop stream mode
   // before doing an auto-config)
   resetLen=0;
   reset[resetLen++]='B';

   // send the poll mode command (cmd and cmd_size are args)
   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing poll cmd to tracker");
     status = TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

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
   // least 300 ms before and after sending the autoconfig

   sleep(1);
   // send the reset command (cmd and cmd_size are args)
   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing auto-config to tracker");
     status = TRACKER_FAIL;
     return;
   }
   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );
   sleep(1);

   resetLen=0;

   // clear the input buffer (it will contain a single point 
   // record from the poll command above)
   vrpn_flush_input_buffer(serial_fd);

   // get the system status to check that it opened correctly
   // examine value cmd is 'O'
   reset[resetLen++]='O';
   
   // flock system status is 36
   reset[resetLen++]=36;

   // write the cmd and get response
   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing get sys config to tracker");
     status = TRACKER_FAIL;
     return;
   }

   // make sure the command is sent out
   vrpn_drain_output_buffer( serial_fd );

   // let the tracker respond
   sleep(1);

   static unsigned char response[14];
   int cRet;
   if ((cRet=read_available_characters(response, 14))!=14) {
     fprintf(stderr, 
	     "\nvrpn_Tracker_Flock: received only %d of 14 chars as status", 
	     cRet);
     status = TRACKER_FAIL;
     return;
   }
   
   // check the configuration ...
   int fOk=1;

   for (int i=0;i<=cSensors;i++) {
     fprintf(stderr, "\nvrpn_Tracker_Flock: unit %d", i);
     if (response[i] & 0x20) {
       fprintf(stderr," (a receiver)");
     } else {
       fprintf(stderr," (a transmitter)");
     }
     if (response[i] & 0x80) {
       fprintf(stderr," is accessible", i);
     } else {
       fprintf(stderr," is not accessible", i);
       fOk=0;
     }
     if (response[i] & 0x40) {
       fprintf(stderr," and is running", i);
     } else {
       fprintf(stderr," and is not running", i);
       fOk=0;
     }
   }
   
   fprintf(stderr, "\n");

   if (!fOk) {
     perror("\nvrpn_Tracker_Flock: problems resetting tracker.");
     status = TRACKER_FAIL;
     return;
   }

   // now set modes: pos/quat, group, stream
   resetLen=0;
   // group mode
   reset[resetLen++] = 'P';
   reset[resetLen++] = 35;
   reset[resetLen++] = 1;
   // pos/quat mode sent to each receiver (transmitter is unit 1)
   // 0xf0 + addr is the cmd to tell the master to forward a cmd
   for (int i=1;i<=cSensors;i++) {
     reset[resetLen++] = 0xf0 + i + 1;
     reset[resetLen++] = ']';
   }

   // stream mode
   reset[resetLen++] = '@';

   // point mode for now
   //   reset[resetLen++]='B';

   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock: failed writing set mode cmds to tracker");
     status = TRACKER_FAIL;
     return;
   }
   
   // make sure the commands are sent out
   vrpn_drain_output_buffer( serial_fd );

   fprintf(stderr,"\nvrpn_Tracker_Flock: running.\n");

   gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = TRACKER_SYNCING;	// We're trying for a new reading
   cResets=0;
}


void vrpn_Tracker_Flock::get_report(void)
{
   int ret;
   static int cSyncs=0;

   // The reports are 15 bytes long each (Pos/Quat format plus
   // group address), with a phasing bit set in the first byte 
   // of each sensor.
   // VRPN sends every tracker report for every sensor.
#define RECORD_SIZE 15

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
   ret = read_available_characters(&buffer[bufcount], RECORD_SIZE-bufcount);
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

   for (int i=0;i<3;i++) {
     // scale and convert to meters
     pos[i] = (double)(rgs[i] * POSK144 * 0.0254);
   }
   
   // they code quats as w,x,y,z, we need to give out x,y,z,w
   // The quats are already normalized
#define WTF (float)(1.0/32768.0)                    /* float to word integer */
   for (int i=4;i<7;i++) {
     //     quat[i-4] = (double)(((short *)buffer)[i] * WTF);
     quat[i-4] = (double)(rgs[i] * WTF);
   }
   quat[3] = (double)(rgs[3] * WTF);

   // sensor addr are 0 indexed in vrpn, but start at 2 in the flock 
   // (1 is the transmitter)
   sensor = buffer[RECORD_SIZE-1]-2;

   // all set for this sensor, so cycle
   status = TRACKER_REPORT_READY;
   bufcount = 0;

#ifdef VERBOSE
      print();
#endif
}


void vrpn_Tracker_Flock::mainloop()
{
  switch (status) {
  case TRACKER_REPORT_READY:
    {
#ifdef	VERBOSE
      static int count = 0;
      if (count++ == 120) {
	printf("\nvrpn_Tracker_Flock: Got report"); print();
	count = 0;
      }
#endif            
      
      // Send the message on the connection
      if (connection) {
	static int cSeconds = 3;
	static char	msgbuf[1000];
	int	len = encode_to(msgbuf);

	// data to calc report rate 
	static struct timeval tvNow;
	gettimeofday(&tvNow, NULL);
	static struct timeval tvLastPrint=tvNow;
	static int cReports=0;
	
	cReports++;

	if (timevalMsecs(timevalDiff(tvNow, tvLastPrint)) > cSeconds*1000) {
	  double dRate = cReports / 
	    (timevalMsecs(timevalDiff(tvNow, tvLastPrint))/1000.0);
	  fprintf(stderr, "\nFlock: reports being sent at %6.2lf hz (%d sensors, so ~%6.2lf hz per sensor)\n", 
		  dRate, cSensors, dRate/cSensors);
	  tvLastPrint = tvNow;
	  cReports=0;
	  // display the rate every 30 seconds
	  cSeconds=30;
	}

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
		  "\nvrpn_Tracker_Flock: cannot write message ...  tossing");
	}
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock: No valid connection");
      }
      
#if 0
// polling for debugging purposes
      if (sensor==(cSensors-1)) {
	// poll again
	char ch;
	
	fprintf(stderr, "\nPoll again? ");
	fscanf(stdin, "%c", &ch);
	if (ch=='y') {
	  ch='B';
	  
	  // send the poll mode command (cmd and cmd_size are args)
	  if (write(serial_fd, &ch, 1 )!=1) {
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
    break;
    
  case TRACKER_FAIL:
    if (cResets==4) {
      char ch;
      checkError();
      fprintf(stderr, "\nvrpn_Tracker_Flock: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nPress return when the flock is ready to be re-started. ");
      fscanf(stdin, "%c", &ch);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock: tracker failed, trying to reset ...");
    close(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);
    status = TRACKER_RESETTING;
    cResets++;
    break;
  }
}


