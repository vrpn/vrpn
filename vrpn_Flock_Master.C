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


#include "vrpn_Flock_Master.h"

// output a status msg every status_msg_secs
#define STATUS_MSG
#define STATUS_MSG_SECS 600

void vrpn_Tracker_Flock_Master::printError( unsigned char uchErrCode, 
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
int vrpn_Tracker_Flock_Master::checkError() {
  unsigned char rguch[4]={'B','G','O',10};

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (write(serial_fd, &rguch, 2 )!=2) {
    perror("\nvrpn_Tracker_Flock_Master: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // clear in buffer
  vrpn_flush_input_buffer( serial_fd );

  // now get error code
  if (write(serial_fd, &(rguch[2]), 2 )!=2) {
    perror("\nvrpn_Tracker_Flock_Master: failed writing cmds to tracker");
    return -1;
  }

  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );

  sleep(1);

  // read response
  int cRet;
  if ((cRet=read_available_characters(rguch, 2))!=2) {
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Master: received only %d of 2 chars for err code", 
	    cRet);
    return -1;
  }

  if (rguch[0]) {
    printError( rguch[0], rguch[1] );
  }
  return rguch[0];
}

vrpn_Tracker_Flock_Master::vrpn_Tracker_Flock_Master(char *name, vrpn_Connection *c, 
				       int cSensors, char *port, long baud):
  vrpn_Tracker_Serial(name,c,port,baud), cSensors(cSensors), cResets(0) {
    if (cSensors>MAX_SENSORS) {
      fprintf(stderr, "\nvrpn_Tracker_Flock_Master: too many sensors requested ... only %d allowed (%d specified)", MAX_SENSORS, cSensors );
      cSensors = MAX_SENSORS;
    }
    fprintf(stderr, "\nvrpn_Tracker_Flock_Master: starting up ...");
}


vrpn_Tracker_Flock_Master::~vrpn_Tracker_Flock_Master() {
  int cChars=1;
  unsigned char rgch[2]={'G'};

  fprintf(stderr,"\nvrpn_Tracker_Flock_Master: shutting down ...");
  // clear output buffer
  vrpn_flush_output_buffer( serial_fd );

  // put the flock to sleep (B to get out of stream mode, G to sleep)
  if (write(serial_fd, &rgch, cChars )!=cChars) {
    perror("\nvrpn_Tracker_Flock_Master: failed writing sleep cmd to tracker");
    status = TRACKER_FAIL;
    return;
  }
  // make sure the command is sent out
  vrpn_drain_output_buffer( serial_fd );
  fprintf(stderr, " done.\n");
}


#define MAX_TIME_INTERVAL       (5*1000000) // max time between reports (usec)
unsigned long	vrpn_Tracker_Flock_Master::duration(struct timeval t1, struct timeval t2)
{
  if (cResets != -1 ) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}

void vrpn_Tracker_Flock_Master::reset()
{
   int resetLen;
   unsigned char reset[6*(MAX_SENSORS+1)+10];

   // Get rid of the characters left over from before the reset
   vrpn_flush_input_buffer(serial_fd);
   vrpn_flush_output_buffer(serial_fd);

   // put back into polled mode (need to stop stream mode
   // before doing an auto-config)
   resetLen=0;
   reset[resetLen++]='B';

   // send the poll mode command (cmd and cmd_size are args)
   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock_Master: failed writing poll cmd to tracker");
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
     perror("\nvrpn_Tracker_Flock_Master: failed writing auto-config to tracker");
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
     perror("\nvrpn_Tracker_Flock_Master: failed writing get sys config to tracker");
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
	     "\nvrpn_Tracker_Flock_Master: received only %d of 14 chars as status", 
	     cRet);
     status = TRACKER_FAIL;
     return;
   }
   
   // check the configuration ...
   int fOk=1;
   int i;

   for (i=0;i<=cSensors;i++) {
     fprintf(stderr, "\nvrpn_Tracker_Flock_Master: unit %d", i);
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
     perror("\nvrpn_Tracker_Flock_Master: problems resetting tracker.");
     status = TRACKER_FAIL;
     return;
   }

   // now set modes: pos/quat, group, stream
   resetLen=0;

   /* disable group mode
   // group mode
   reset[resetLen++] = 'P';
   reset[resetLen++] = 35;
   reset[resetLen++] = 1;
   */

   // pos/quat mode sent to each receiver (transmitter is unit 1)
   // 0xf0 + addr is the cmd to tell the master to forward a cmd
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = 0xf0 + i + 1;
     reset[resetLen++] = ']';
   }

   // *NO* stream mode
   //reset[resetLen++] = '@';

   //point mode for now
   //reset[resetLen++]='B';

   // set to lower hemisphere, send cmd to each receiver
   for (i=1;i<=cSensors;i++) {
     reset[resetLen++] = 0xf0 + i + 1;
     reset[resetLen++] = 'L';
     reset[resetLen++] = 0xc;
     reset[resetLen++] = 0;
   }
   //reset[resetLen++]= 'F';

   if (write(serial_fd, reset, resetLen )!=resetLen) {
     perror("\nvrpn_Tracker_Flock_Master: failed writing set mode cmds to tracker");
     status = TRACKER_FAIL;
     return;
   }
   
   // make sure the commands are sent out
   vrpn_drain_output_buffer( serial_fd );

   fprintf(stderr,"\nvrpn_Tracker_Flock_Master: running.\n");

   gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = TRACKER_SYNCING;	// We're trying for a new reading
   cResets=-1; 
}


void vrpn_Tracker_Flock_Master::get_report(void)
{
  // master is the controller , it doesn't get any report
  // all reports are sent through seperate serial port
  // poll back a status report every now and then;
  static int ctimes = 0;
  unsigned char cReport[24];
  int thisread;
  const int reportLen = 14;
  if (status == TRACKER_PARTIAL) {
    thisread =read_available_characters(buffer+bufcount, reportLen-bufcount);
    //fprintf(stderr, "this time read %d %d %d **", thisread,reportLen, bufcount);

    //thisread =this->read_available_characters(buffer, 14);
    //fprintf(stderr, "this time read %d %d %d \n", thisread,reportLen, bufcount);
    if (thisread < 0) { status = TRACKER_FAIL; return;}
    bufcount += thisread;
    if (bufcount < reportLen) return;
    //printf("Report Status=  %x %x %x %x\n", 
    // buffer[0], buffer[1], buffer[2], buffer[3]);
    if (!(buffer[0] & (0x80|0x40) && buffer[1] & (0x80|0x40)  &&
	  buffer[2] & (0x80|0x40) && buffer[3] & (0x80|0x40)))
	{ status = TRACKER_FAIL; return; }

    status = TRACKER_SYNCING;
    cResets = 0;
    return; 
  }
  else {
    cResets = -1;
    int start =0;
    int read = 0;
    //cReport[start++]= 0xf0 +1;
    //cReport[start++]= 'G', 
    cReport[start++]= 'O', cReport[start++]= 36;
    //cReport[start++]= 'F';
    //fprintf(stderr, " get status report\n");

    // write the cmd and get response
   if (write(serial_fd, cReport, start )!=start) {
     perror("\nvrpn_Tracker_Flock_Master: failed writing get sys config to tracker");
     status = TRACKER_FAIL;
     return;
   }
    // make sure the command is sent out
   //vrpn_drain_output_buffer( serial_fd );

    gettimeofday(&timestamp, NULL);
    status = TRACKER_PARTIAL;
    bufcount =0;
    //sleep(1);
    //fprintf(stderr, " get status report 1.5\n");
    // Try to get a character.  If none, just return.
    bufcount = read_available_characters(buffer, 14); 
    
    //fprintf(stderr, " get status report 2 %d\n", bufcount);
    
  } 
  return;
}


void vrpn_Tracker_Flock_Master::mainloop()
{
  switch (status) {
  case TRACKER_REPORT_READY:
      status = TRACKER_SYNCING;
  break;
  
  case TRACKER_SYNCING:
  case TRACKER_PARTIAL:
    {
      struct timeval current_time;

      gettimeofday(&current_time, NULL);
      if ( duration(current_time,timestamp) < MAX_TIME_INTERVAL) {
	get_report();
      } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock_Master: failed to read ... current_time=%ld:%ld, timestamp=%ld:%ld",
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
      fprintf(stderr, "\nvrpn_Tracker_Flock_Master: problems resetting ... check that: a) all cables are attached, b) all units have FLY/STANDBY switches in FLY mode, and c) no receiver is laying too close to the transmitter.  When done checking, power cycle the flock.\nWill attempt to reset in 30 seconds.\n");
      sleep(30);
      // "press return when the flock is ready to be re-started. ");
      //      fscanf(stdin, "%c", &ch);
      cResets=0;
    }
    fprintf(stderr, 
	    "\nvrpn_Tracker_Flock_Master: tracker failed, trying to reset ...");
    close(serial_fd);
    serial_fd = vrpn_open_commport(portname, baudrate);
    status = TRACKER_RESETTING;
    cResets++;
    break;
  }
}


