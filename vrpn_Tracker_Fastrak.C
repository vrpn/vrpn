/*                               -*- Mode: C -*- 
 * 
 * This library is free software; you can redistribute it and/or          
 * modify it under the terms of the GNU Library General Public            
 * License as published by the Free Software Foundation.                  
 * This library is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      
 * Library General Public License for more details.                       
 * If you do not have a copy of the GNU Library General Public            
 * License, write to The Free Software Foundation, Inc.,                  
 * 675 Mass Ave, Cambridge, MA 02139, USA.                                
 *                                                                        
 * For more information on this program, contact Blair MacIntyre          
 * (bm@cs.columbia.edu) or Steven Feiner (feiner@cs.columbia.edu)         
 * at the Computer Science Dept., Columbia University,                    
 * 500 W 120th St, Room 450, New York, NY, 10027.                         
 *                                                                        
 * Copyright (C) Blair MacIntyre 1995, Columbia University 1995           
 * 
 * Author          : Ruigang Yang
 * Created On      : Thu Jan 15 17:30:37 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Tue Apr  7 13:21:36 1998
 * Update Count    : 414
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Tracker_Fastrak.C,v $
 * $Date: 1998/11/05 22:45:55 $
 * $Author: taylorr $
 * $Revision: 1.12 $
 * 
 * $Log: vrpn_Tracker_Fastrak.C,v $
 * Revision 1.12  1998/11/05 22:45:55  taylorr
 * This version strips out the serial-port code into vrpn_Serial.C.
 *
 * It also makes it so all the library files compile under NT.
 *
 * It also fixes an insidious initialization bug in the forwarder code.
 *
 * Revision 1.11  1998/06/05 19:30:47  taylorr
 * Slightly cleaner Fastrak driver.  It should work on SGIs as well as Linux.
 *
 * Revision 1.10  1998/06/05 17:13:15  taylorr
 * This version works on machines where float cannot be accessed on
 * unaligned memory boundaries.
 *
 * Revision 1.9  1998/06/04 16:05:53  taylorr
 * This version should compile at remote locations.
 * This version no longer requires the SDI library to run.  It does all
 * 	lookups by UDP callback based on host name for remote connections.
 * Some bug fixes.
 *
 * Revision 1.8  1998/04/07 17:27:15  ryang
 * change sensor id number starting from zeor
 *
 * Revision 1.7  1998/02/24 22:24:19  ryang
 * comment the printf debug function
 *
 * Revision 1.6  1998/02/20 20:27:00  hudson
 * Version 02.10:
 *   Makefile:  split library into server & client versions
 *   Connection:  added sender "VRPN Control", "VRPN Connection Got/Dropped"
 *     messages, vrpn_ANY_TYPE;  set vrpn_MAX_TYPES to 2000.  Bugfix for sender
 *     and type names.
 *   Tracker:  added Ruigang Yang's vrpn_Tracker_Canned
 *
 * Revision 1.5  1998/02/19 21:00:47  ryang
 * drivers for DynaSight
 *
 * Revision 1.4  1998/02/11 20:35:40  ryang
 * canned class
 *
 * Revision 1.3  1998/01/26 21:21:55  weberh
 * compiles on sgi in addition to linux
 *
 * Revision 1.2  1998/01/24 19:12:13  ryang
 * read one report at a time, no matter how many sensors are on.
 *
 * Revision 1.1  1998/01/22 21:08:51  ryang
 * add vrpn_Tracker_Fastrak.C to data base
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

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
#include "vrpn_Tracker_Fastrak.h"
#include "vrpn_Serial.h"

#ifndef VRPN_CLIENT_ONLY
#if defined(sgi) || defined(linux)

#define READ_HISTOGRAM

#define MAX_LENGTH              (512)

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)

#define TRACKER_SYNCING         (2)
#define TRACKER_REPORT_READY    (1)
#define TRACKER_PARTIAL         (0)
#define TRACKER_RESETTING       (-1)
#define TRACKER_FAIL            (-2)

// This constant turns the tracker binary values in the range -32768 to
// 32768 to meters.
#define T_3_DATA_MAX            (32768.0)
#define T_3_INCH_RANGE          (65.48)
#define T_3_CM_RANGE            (T_3_INCH_RANGE * 2.54)
#define T_3_METER_RANGE         (T_3_CM_RANGE / 100.0)
#define T_3_BINARY_TO_METERS    (T_3_METER_RANGE / T_3_DATA_MAX)

#undef VERBOSE


#if defined(sun) || defined(sgi) || defined(hpux) || defined(__hpux)
#   define T_F_REVERSE_BYTES(dest, src, size, num) \
                                t_reverse_bytes(dest, src, size, num)
#else
#   define T_F_REVERSE_BYTES(dest, src, size, num) \
		memcpy(dest, src, size*num)
#endif


/*****************************************************************************
 *
   t_reverse_bytes - reverse the byte order of words in a buffer
 
    input:
        - pointer to dest buffer
        -   ''    '' src    ''
        - size of words in buffer (can be odd)
        - number of words in buffer
    
    output:
        - src buffer's bytes are reversed in words in dest buffer
 
    notes:
        - also works for swapping bytes, but slightly less efficient
            and I didn't feel like changing all of the other drivers.
        
        - the dest and src buffers must be distinct!  (done this way
            for speed, although it could be modified)
 *
 *****************************************************************************/

static void
t_reverse_bytes(unsigned char destBuffer[], unsigned char srcBuffer[], 
		int wordSize, int numWords)
{
    int             i, j;
    unsigned char   *srcPtr, *destPtr;


srcPtr = srcBuffer;
destPtr = destBuffer;
for ( i = 0; i < numWords; i++ )
    {
    /* swap bytes at from ends of word and work toward the middle   */
    for ( j = 0; j < wordSize; j++ )
        destPtr[j] = srcPtr[wordSize-j-1];

    srcPtr += wordSize;
    destPtr += wordSize;
    }

}       /* t_reverse_bytes */

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
  if (t2.tv_sec == -1) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}

// Read from the input buffer on the specified handle until all of the
//  characters are read.  Return 0 on success, -1 on failure.
static int	vrpn_flushInputBuffer(int comm)
{
   tcflush(comm, TCIFLUSH);
   return 0;
}

void vrpn_Tracker_Fastrak::reset()
{
   static int numResets = 0;	// How many resets have we tried?
   int i,resetLen,ret;
   char reset[10];

   // Send the tracker a string that should reset it.  The first time we
   // try this, just do the normal ^Y reset.  Later, try to reset
   // to the factory defaults.  Then toggle the extended mode.
   // Then put in a carriage return to try and break it out of
   // a query mode if it is in one.  These additions are cumulative: by the
   // end, we're doing them all.
   resetLen = 0;
   numResets++;		  	// We're trying another reset
   if (numResets > 1) {	// Try to get it out of a query loop if its in one
   	reset[resetLen++] = (char) (13); // Return key -> get ready
   }
   if (numResets > 7) {
	reset[resetLen++] = 'Y'; // Put tracker into tracking (not point) mode
   }
   if (numResets > 3) {	// Get a little more aggressive
   	if (numResets > 4) { // Even more aggressive
      	reset[resetLen++] = 't'; // Toggle extended mode (in case it is on)
   }
   reset[resetLen++] = 'W'; // Reset to factory defaults
   reset[resetLen++] = (char) (11); // Ctrl + k --> Burn settings into EPROM
   }
   reset[resetLen++] = (char) (25); // Ctrl + Y -> reset the tracker
   fprintf(stderr, "Resetting the Fastrak (attempt #%d)",numResets);
   for (i = 0; i < resetLen; i++) {
	if (write(serial_fd, &reset[i], 1) == 1) {
		fprintf(stderr,".");
		sleep(1);  // Wait 2 seconds each character
   	} else {
		perror("Fastrak: Failed writing to tracker");
		status = TRACKER_FAIL;
		return;
	}
   }
   sleep(10);	// Sleep to let the reset happen
   fprintf(stderr,"\n");

   // Get rid of the characters left over from before the reset
   vrpn_flushInputBuffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   sleep(2);
   unsigned char scrap[80];
   if ( (ret = vrpn_read_available_characters(serial_fd, scrap, 80)) != 0) {
     fprintf(stderr,"  Fastrak warning: got >=%d characters after reset:\n",ret);
     for (i = 0; i < ret; i++) {
      	if (isprint(scrap[i])) {
         	fprintf(stderr,"%c",scrap[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",scrap[i]);
         }
     }
     fprintf(stderr, "\n");
     vrpn_flushInputBuffer(serial_fd);		// Flush what's left
   }

   // Asking for tracker status
   if (write(serial_fd, "S", 1) == 1) {
      sleep(1); // Sleep for a second to let it respond
   } else {
	perror("  Fastrak write failed");
	status = TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[56];
   if ( (ret = vrpn_read_available_characters(serial_fd, statusmsg, 55)) != 55){
  	fprintf(stderr, "  Got %d of 55 characters for status\n",ret);
   }
   if ( (statusmsg[0]!='2') || (statusmsg[54]!=(char)(10)) ) {
     int i;
     statusmsg[55] = '\0';	// Null-terminate the string
     fprintf(stderr, "  Tracker: status is (");
     for (i = 0; i < 55; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr, ")\n  Bad status report from tracker, retrying reset\n");
     return;
   } else {
     fprintf(stderr, "  Fastrak gives status!\n");
     //printChar((char *)statusmsg, 55);
     numResets = 0; 	// Success, use simple reset next time
   }


   /* set all stations except first station 
(since we can't turn it off) */
				
      stationArray[0] = T_F_ON;
      stationArray[1] = T_F_ON;
      stationArray[2] = T_F_ON;
      stationArray[3] = T_F_ON;
    numUnits = 4;
    mode = T_F_M_POLLING;

    if ( set_all_units() == T_ERROR ) {
      fprintf(stderr,"can't set all units\n");
      return;
    }



    /* set binary mode  */
    ms_sleep(50);
    bin_ascii(T_F_M_BINARY);

    /* set up the data format stuff in the tracker table;  setting it in
     *  the fastrak itself will be done when enable is called for that unit
     */
    dataFormat = T_DEFAULT_DATA_FORMAT;

    /* use quaternions for all formats, then convert later in read
       routines	*/

    reportLength = T_F_XYZ_QUAT_LENGTH;

    /* totalReportLength is updated by the enable/disable units routines */
    totalReportLength = reportLength * numUnits;

    /* set up filtering; don't worry about return code from this fn since
     *   it's not tragic if filter setting fails (but error message is printed
     *   by t_f_filter)
     */
    filter();

    // ok now turn on Station 0;


    //enable(0);
    //enable(1);

    fprintf(stderr, "  (at the end of Fastrak reset routine)\n");
    for (i=0; i< 4; i++) 
      if (stationArray[i] == T_F_ON) 
	printf("Station [%d] is ON\n", i);
      else printf("Station [%d] is OFF\n",i);

    printf("totalReportLength = %d, numUnits = %d\n", 
	   totalReportLength, numUnits);

    mode = T_F_M_CONTINUOUS;
    cont_mode();
    //sleep(5);
    //gettimeofday(&timestamp, NULL);	// Set watchdog now;
    status = TRACKER_SYNCING;	// We are trying for a new reading;
    timestamp.tv_sec = -1;
    return;
}


void vrpn_Tracker_Fastrak::get_report(void) {
  int ret;
  
  //fprintf(stderr,"get report %p\t%s:%d\n",this,  __FILE__, __LINE__);
  if (status == TRACKER_SYNCING) {
    
    if ((ret=vrpn_read_available_characters(serial_fd, buffer, 1)) !=  1 ||
	buffer[0] != '0') {
      return;
    }

    gettimeofday(&timestamp, NULL);
    status = TRACKER_PARTIAL;
    bufcount= ret;
  }

  /* Read as many bytes of the remaining as we can, storing them;
  // in the buffer.  We keep track of how many have been read so far;
  // and only try to read the rest.  The routine that calls this one;
  // makes sure we get a full reading often enough (ie, it is responsible;
  // for doing the watchdog timing to make sure the tracker hasn't simply;
  // stopped sending characters).;*/	
  if (status == TRACKER_PARTIAL) {
    if (reportLength < 0 || reportLength > 100) exit(-1);
    //fprintf(stderr, "reportLength = %d,bufC= %d\n", reportLength,bufcount);
    ret=vrpn_read_available_characters(serial_fd, &(buffer[bufcount]),
	reportLength-bufcount);
    if (ret < 0) {
      fprintf(stderr,"%s@%d: Error reading\n", __FILE__, __LINE__);
      status = TRACKER_FAIL;
      return;
    }
    //fprintf(stderr,"get report:get %d bytes\t%s:%d\n", 
	//   bufcount, __FILE__, __LINE__);
    bufcount += ret;
    if (bufcount < reportLength) {	// Not done -- go back for more
      return;
    }	
    //fprintf(stderr, "this time read: %d rL= %d, \n", ret, reportLength);
  }
  
  // now decode the report;
  //fprintf(stderr, "get_report: start decode %d\t%s:%d\n", 
	//  reportLength, __FILE__, __LINE__);


  if (!valid_report()) {
    bufcount = 0;
    status = TRACKER_SYNCING;
    return;
  }
  //fprintf(stderr, "get_report: valid list, %d\t%s:%d\n", 
	//  reportLength, __FILE__, __LINE__);
  xyz_quat_interpret();

  status = TRACKER_REPORT_READY;
  bufcount=0;

#ifdef VERBOSE
      print();
#endif
}


void vrpn_Tracker_Fastrak::mainloop()
{
  switch (status) {
    case TRACKER_REPORT_READY:
      {

	static int count = 0;
	if (count++ == 120) {
		//printf("  vrpn_Tracker_Fastrak: Got report\n");
		count = 0;
	}


	// Send the message on the connection
	if (connection) {
		char	msgbuf[1000];
		int	len = encode_to(msgbuf);
		if (connection->pack_message(len, timestamp,
			position_m_id, my_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
		  fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}
	} else {
		fprintf(stderr,"Tracker Fastrak: No valid connection\n");
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
		fprintf(stderr,"Tracker failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",current_time.tv_sec, current_time.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
		status = TRACKER_FAIL;
	}
      }
      break;

    case TRACKER_RESETTING:
	reset();
	break;

    case TRACKER_FAIL:
	fprintf(stderr, "Tracker failed, trying to reset (Try power cycle if more than 4 attempts made)\n");
	close(serial_fd);
	serial_fd = vrpn_open_commport(portname, baudrate);
	status = TRACKER_RESETTING;
	break;
   }
}

/* %%%%%%%%%%%%%%%%%% private member functions %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*****************************************************************************
 *
   set_all_units - higher-level function that sets on or off each of the
    	    	      units specified in setArray.
 
    output:
    	- each unit is turned on/off as specified (this is verified by 
	    t_f_set_unit())
    
    notes:
    	turn on any new units before trying to turn off any old units, 
    	since turning off the last unit is not allowed.
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::set_all_units()
{

    int	    i;


    /* turn ON new ones	*/
    for ( i = 0; i < T_F_MAX_NUM_STATIONS; i++ )
      {
	/* if station should be on, turn it on	*/
	if ( stationArray[i] == T_F_ON ) {
	  if ( set_unit(i, T_F_ON) == T_ERROR )
	    return (T_ERROR);
	  /* set hemisphere to be the normal +X	*/
	  set_hemisphere(i, Q_X, 1);
	  set_data_format(i);

	}
	
    }

    /* turn OFF old ones	*/
    for ( i = 0; i < T_F_MAX_NUM_STATIONS; i++ )
    {
      /* if station should be off, turn it off	*/
      if ( stationArray[i] == T_F_OFF )
    	if ( set_unit(i, T_F_OFF) == T_ERROR )
	    return(T_ERROR);
    }

    return(T_OK);

}	/* set_all_units */



/*****************************************************************************
 *
   set_unit - turns on or off a Fastrak station/unit, depending on the
    	    	    	onOff parameter
    
    input:
	- which unit is to be turned on/off
	- on or off

    output:
    	- T_OK on success
	- T_ERROR on failure after 5 tries
 
    notes:  this routine knows nothing about the tracker table's unitsOn
    	    array, nor about numUnits;  it just communicates w/ the fastrak
 *
 *****************************************************************************/

int
vrpn_Tracker_Fastrak::set_unit(int whichUnit, int onOff)

{
    
    char     stationString[] = T_F_C_STATION_TOGGLE;
    int	    	    unitOnArray[T_F_MAX_NUM_STATIONS];
    int	    	    numTries = 0;

    
    //fprintf(stderr, "here %p, %s:%d\n", this,__FILE__, __LINE__);
    /* figure out how many and which units are already REALLY on	*/
    if ( get_units(unitOnArray) < 0 )
      return(T_ERROR);

    //fprintf(stderr, "Here \t%s:%d\n", __FILE__, __LINE__);
    /* is unit already on or off?  */
    if ( unitOnArray[whichUnit] == onOff )
      return(T_OK);

    /*
     * replace the '#' placeholder with the number of the unit/station;
     *  add 1 to whichUnit since Fastrak units start from 1
     */
    stationString[1] = (char) ((int) '0' + (whichUnit+1));

    /* replace the '%' placeholder with a '0' for off and '1' for on */
    if ( onOff == 0 )
      stationString[3] = '0';
    else
      stationString[3] = '1';

    /* turn it off/on */
    do { 
      

    /* send enable command for this unit to fastrak	*/
      write(serial_fd, (const unsigned char *) stationString, 
	    strlen(stationString));
      ms_sleep(50);

      numTries++;
    
    }
    while ((verify_unit_status(whichUnit, onOff) == T_ERROR) &&
    	(numTries < 5) );

    if ( numTries >= 5 )
      return(T_ERROR);

    return(T_OK);

}	/* t_f_set_unit */



/*****************************************************************************
 *
   get1_units - low-level workhorse;  returns the number of 
    	    	      stations that are on, plus a vector indicating 
		      which ones are on.  

    called by: 
       	t_f_set_all_units()
       	t_f_verify_unit_status()
    	t_f_enable()
	t_f_disable()


   
   station record typically looks something like: "21l11000000";  the first
   T_F_STATUS_LENGTH bytes are status bytes (usually 3) followed by n chars
   representing the n different stations (here, n=8).  '1' signifies that
   the station is on, '0', off. 
    
    If the fastrak gives an error message in response to the stations
    status request, the routine assumes that this is a single station
    fastrak and just reports all stations off except #1.
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::get_units(int stationVector[T_F_MAX_NUM_STATIONS])
{


    char    this_buffer[T_MAX_READ_BUFFER_SIZE];
    
    int	    numStationsOn;
    int	    numTries = 0;
    int	    i;


    //fprintf(stderr, "get_units %p, \t%s:%d\n", this, __FILE__, __LINE__);
    /* pause continuous mode if on	*/
    if ( poll_mode() == T_ERROR )
      return(T_ERROR);
    //fprintf(stderr, "get_units %p,\t%s:%d\n", this, __FILE__, __LINE__);

    /* find out which stations are on:  ask fastrak for station record  
     *   we expect either a valid stations record or an error record (ie., from
     *   a 1-station Isotrak unit);  if we get garbage, we'll
     *   do this up to 5 times, then quit if no success
     */
    do { 
      my_flush();
      if ( numTries > 0 )
	/* this error may be caused by fastrak ignoring poll mode command */
	poll_mode();

      write(serial_fd, (const unsigned char *) T_F_C_RETRIEVE_STATIONS, 
    	    	    	    	strlen(T_F_C_RETRIEVE_STATIONS));
      ms_sleep(500);
      vrpn_read_available_characters(serial_fd, (unsigned char *) this_buffer, 
			      T_F_STATIONS_RECORD_LENGTH);
    
      numTries++;
    }while ( (this_buffer[T_F_RECORD_SUBTYPE] != T_F_STATIONS_SUBTYPE) &&
    	(this_buffer[T_F_RECORD_SUBTYPE] != T_F_ERROR_SUBTYPE) &&
	(numTries < 5) );

    /* resume continuous mode if nec    */
    if ( mode == T_F_M_CONTINUOUS )
      cont_mode();

    /* if we did 5 passes, probably didn't succeed, 
       so quit with error message  */
    if ( numTries >= 5 ) 
    {
      fprintf(stderr, "%s@%d: couldn't get valid stations report;\n",
	      __FILE__, __LINE__);
      fprintf(stderr, "   try cycling the power on the Fastrak.\n");
      return(T_ERROR);
    }

    /* check for error message-  some polhemi don't support this command*/
    if ( this_buffer[T_F_RECORD_SUBTYPE] == T_F_ERROR_SUBTYPE )
      {
	fprintf(stderr,"get_units: only one station is on\n");
	/* fill out vector of active stations   */
	stationVector[0] = T_F_ON;

	for ( i = 1; i < T_F_MAX_NUM_STATIONS; i++ )
	  stationVector[i] = T_F_OFF;

	/* tell caller that only one station is active, since this is the 
	 *  case for polhemi that don't support the stations command
	 */
	return(1);
      }

    /* buffer seems ok- parse return string to find out which units are on.  */
    numStationsOn = 0;
    for ( i = 0; i < T_F_MAX_NUM_STATIONS; i++ )
    {
    /* is this station on?  */
      if ( this_buffer[T_F_STATUS_LENGTH+i] == '1' )
	{
	/* station is already on	*/
    	stationVector[i] = T_F_ON;
	numStationsOn++;
	}
      else
    	stationVector[i] = T_F_OFF;
    }
    fprintf(stderr, "get_units: %d stations are on %p\t%s:%d\n", 
	    numStationsOn, this, __FILE__, __LINE__);
    return(numStationsOn);

}	/* get_units */




/*****************************************************************************
 *
   t_f_verify_unit_status - verify that the specified unit is on/off
 
    input:
	- which unit is to be checked
	- on or off (T_F_ON or T_F_OFF)

    output:
    	- T_OK if whichUnit is on/off 
	- T_ERROR is returned if unit status is incorrect;  also if there
	    is an error in the unit specified or in getting the stations
	    report
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::verify_unit_status(int whichUnit, int unitOn)
     /* int	    	    unitOn; 	 should unit be on or off?  */
{
    int	    onArray[T_F_MAX_NUM_STATIONS];


    if ( get_units(onArray) < 0 )
      return(T_ERROR);
    
    if ( onArray[whichUnit] == unitOn )
      return(T_OK);
    else
      return(T_ERROR);

}	/* verify_unit_status */



/*****************************************************************************
 *
   filter - set up filtering parameters for fastrak
    output:
    	- filtering is set according to #defines at top of file
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::filter()
{

    char    buffer[T_MAX_READ_BUFFER_SIZE];


    /* clear buffer and set extended command mode	*/
    my_flush();

    /* try to turn off pos'n filtering  */
    write(serial_fd, (const unsigned char *) T_F_C_SET_POSITION_FILTER, 
	  strlen(T_F_C_SET_POSITION_FILTER));

    /* any output here probably indicates an error message, so just quit */
    if ((vrpn_read_available_characters(serial_fd, (unsigned char *) buffer, 
	    T_READ_BUFFER_SIZE) != 0) &&
     (buffer[T_F_RECORD_SUBTYPE] == T_F_ERROR_SUBTYPE) )
	{
	/* hey, we tried.	*/
	my_flush();
	fprintf(stderr,
		"%s@%d: couldn't set position filter values.\n", 
		__FILE__, __LINE__);
	return(T_ERROR);
	}

    /* otherwise, it took the command; now disable orientation filter   */
    my_flush();

    write(serial_fd, (unsigned char *) T_F_C_SET_ORIENTATION_FILTER, 
				    strlen(T_F_C_SET_ORIENTATION_FILTER));

    if((vrpn_read_available_characters(serial_fd, (unsigned char *) buffer, 
		    T_READ_BUFFER_SIZE) != 0) &&
     (buffer[T_F_RECORD_SUBTYPE] == T_F_ERROR_SUBTYPE) )
	{
	my_flush();
	fprintf(stderr, 
		"%s@%d: couldn't set orientation filter values.\n",
		__FILE__, __LINE__);
	return(T_ERROR);
	}

    my_flush();

    return(T_OK);

}	/* t_f_filter */



/*****************************************************************************
 *
   get_status - get a status record from the fastrak

    input:
        NULL
    
    output:
    	- the status buffer is filled w/ status record and checked
    	- returns:
	     T_OK if buffer is OK
	     T_F_SPEW_MODE if it seems to be in continuous mode
	     T_F_NO_DATA if no data on read
	     T_ERROR otherwise
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::get_status()
{
    int	    	bytesRead;
    char    	statusBuffer[T_MAX_READ_BUFFER_SIZE];
    
    

    bufcount=0; // clear the buffer;


    /* send request for status record   */
    write(serial_fd, (unsigned char *) T_F_C_GET_STATUS, 
    	    	    	    	   strlen(T_F_C_GET_STATUS));
    sleep(1);
    /* do non-blocking read of status record    */
    bytesRead = vrpn_read_available_characters(serial_fd,
	(unsigned char *) statusBuffer, T_F_STATUS_RECORD_LENGTH);
    if ( bytesRead == T_F_STATUS_RECORD_LENGTH )
    {
      //printChar(statusBuffer, bytesRead); 
      /* we have correct length-  check a few chars to make sure 
	 this is a valid record */
	if ( (statusBuffer[T_F_RECORD_TYPE] != T_F_RETRIEVE_RECORD) ||
	 (statusBuffer[T_F_RECORD_SUBTYPE] != T_F_STATUS_SUBTYPE) )
	return(T_ERROR);
    //fprintf(stderr, "See here it is ok \t%s:%d\n", __FILE__, __LINE__);
    /* otherwise, all is well   */
    return(T_OK);
    }

    fprintf(stderr, "get_status %d \t%s:%d\n",bytesRead, __FILE__, __LINE__);
    /* if we get here, we either got too much data or not enough	*/

    /* no data means it's probably disconnected or not turned on */

    if ( bytesRead == 0 )
      return(T_F_NO_DATA);

    /* if we got too much data, chances are that it's in continuous mode */
    if ( bytesRead > T_F_STATUS_RECORD_LENGTH )
      return(T_F_SPEW_MODE);

    /* if we get here, i have no idea what's going on-  could be garbage on the
     *  serial line, wrong baud rate, or that the fastrak is flaking out.
     */
    return(T_ERROR);

}	/* get_status */


#define MAX_TRIES   5

int vrpn_Tracker_Fastrak::poll_mode()	{
    int	    	status;
    int		done = 0;
    int	    	numTries = 0;
    char    	statusBuffer[T_MAX_READ_BUFFER_SIZE];

    if (mode != T_F_M_CONTINUOUS) return T_OK;

    /* 
     *   keep trying to pause the fastrak til it shuts up.  we know the beast
     *   is truly paused if we get back a successful status record.
     */
    do {
      //fprintf(stderr, "poll_mode:try...%d,%p  \t%s:%d\n", 
	//      numTries, this, __FILE__, __LINE__);

      /* on second pass, there is a problem.  try sending a retrieve_stations
       *  request to wake it up.  there is no logic to sending this particular
       *  command-  I just found by experimentation that it worked for the
       *  polhemus, so try the same thing for fastrak.
       */
      if ( numTries > 0 ){
	
	/* we often seem to get trouble on the first try, so don't even
	 *  report it until we've tried a few times
	 */
	if ( numTries == 4 )
	    {
	      fprintf(stderr, "having trouble pausing Fastrak;%p\n  ",
		     this);
		fprintf(stderr, "still trying...\n");
	    }

    	write(serial_fd, (unsigned char *) T_F_C_RETRIEVE_STATIONS, 
	    	    	    	strlen(T_F_C_RETRIEVE_STATIONS));
	vrpn_read_available_characters(serial_fd,
		(unsigned char *) statusBuffer, T_F_STATIONS_RECORD_LENGTH);
      }

      /* try to set in poll mode	*/
      write(serial_fd, (unsigned char *) T_F_C_POLLING, 
    	    	    	    	       strlen(T_F_C_POLLING));

      /* pause 50 ms to allow fastrak buffer to stabilize	*/
      ms_sleep(50);
    
      numTries++;

      status = get_status();
    
      if ( status == T_OK )
    	done = 1;
    
      /* if no data, tracker probably not connected.  just bag it.    */
      else if ( status == T_F_NO_DATA )
    	return(T_ERROR);
    } while ( ( ! done ) && (numTries <= MAX_TRIES) );
    //fprintf(stderr, "poll_mode %p \t%s:%d\n", this, __FILE__, __LINE__);
    /* clear any leftover reports	*/
    my_flush();
    /* bad news */
    if ( numTries > MAX_TRIES )
    {
      fprintf(stderr, "%s@%d: pause of Fastrak failed.\n", 
	      __FILE__, __LINE__);
      return(T_ERROR);
    }

    return(T_OK);

}	/* poll_mode */

/*****************************************************************************
 *
   bin_ascii - sets fastrak into binary or ascii mode
 
    input:
    	- mode constant indicating binary or ascii
    
    output:
    	- fastrak is sent command to switch it into appropriate mode
    
    notes:
    	- no checking is done to make sure fastrak enters indicated mode
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::bin_ascii(int binAscii)
{

  if ( binAscii == T_F_M_ASCII )
    write(serial_fd, (unsigned char *) T_F_C_ASCII, 
    	    	    	    	       strlen(T_F_C_ASCII));

  else if ( binAscii == T_F_M_BINARY )
    write(serial_fd, (unsigned char *) T_F_C_BINARY, 
    	    	    	    	       strlen(T_F_C_BINARY));
  
  else
    {
    fprintf(stderr, 
	    "%s@%d:  bogus bin/ascii mode (%d) to bin_ascii\n", 
	    __FILE__, __LINE__, binAscii);
    
    return(T_ERROR);
    }

return(T_OK);

}	/* t_f_bin_ascii */


void vrpn_Tracker_Fastrak::ms_sleep(int ms) {
  struct timeval timeout;
  timeout.tv_usec = ms*1000; // 1 ms = 1000 us;
  timeout.tv_sec =0;
  select(0, NULL, NULL, NULL, &timeout);
  
}

/*****************************************************************************
 *
   t_f_cont_mode - set Fastrak into continuous mode
 
    input:
    	- pointer to current tracker table entry
    
    output:
    	- fastrak is put into continuous mode

 *
 *****************************************************************************/

void vrpn_Tracker_Fastrak::cont_mode()
{


  /* clear any leftover data	*/
  my_flush();

  /* set Fastrak to continous mode    */
  write(serial_fd, (unsigned char *) T_F_C_CONTINUOUS, 
    	    	    	    	   strlen(T_F_C_CONTINUOUS));
  ms_sleep(50);

}	/* cont_mode */





/*****************************************************************************
 *
   t_f_valid_report - tells whether the byte pointed to in the given buffer
    	    	    	is the beginning of a valid record for the given
			unit
 
    input:
	- pointer to beginning of buffer to check
	- index into buffer where data record starts
	- length of buffer
	
	- the index into trackerPtr->unitsOn for the unit that this
		should be the report for.  Ie., check for the station
		corresponding to trackerPtr->unitsOn[unitIndex].

		For example, if we're checking for the first unit
		(unitIndex == 0) on when stations 2 and 7 are enabled,
		unitsOn[0] = 1 and unitsOn[1] = 6 (since unit numbers
		start from 0 and stations start from 1).


    output:
    	- returns T_TRUE if record is valid, T_FALSE otherwise
    
    overview:

    	- check and see if the first three bytes are in the following format:
    	    '01 ', or more generally, xyz, where x = 0, y = station number, and
    	    z = ascii space char
	- Actually, we are now allowing lines of the form xyz, where z =
	    either the space character or the letters A-F, which indicate
	    scaled readings or out-of-range but are otherwise more of a pain
	    to notice than they are to ignore.
 
    	- once in a blue moon, the 0 byte will be the last in the buffer,
	    and we won't know which station's record we're looking at,
	    since that is normally the following byte.  in this case,
	    we look at the previous station's number.
 
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::valid_report()
{

    static char	    correctStatusBytes[] = T_F_STATUS_BYTES;    

    /* is first byte an ascii 0?	*/
    if ( buffer[0] != correctStatusBytes[0] )
      return (T_FALSE);

    /*   see if the station number byte is past the end of this buffer   */
    if (buffer[1] < '1' || buffer[1] >'4' )
      return(T_FALSE);
    if (!(buffer[2] == correctStatusBytes[2] || isalpha(buffer[2]))) 
	// should be a space or D-F;
      return(T_FALSE);
    return T_TRUE;

}	/* t_f_valid_report */



/*****************************************************************************
 *
   checkSubType - check third status byte for anything fishy
 
    input:
    	- tracker pointer
	- buffer of raw data
	- index into buffer
	- length of buffer
    
    output:
    	- true if sub-type byte is ok, false otherwise
    
    notes:
    	- the UNC fastrak occasionally outputs built-in test (BIT) information
	    in the third status byte.  normally, this byte is just a space
	    character;  if BIT information is output, then this char may be
	    a-z or A-Z.  we have seen D-F, which is apparently caused by
	    the sensor being too close to the source.  this routine just
	    checks to make sure that if the character isn't a space that
	    it is at least an ascii character.  a more exhaustive test may
	    have to be done if this generous test causes problems in 
	    parsing records.
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::checkSubType(int bufIndex, int bufferLength)
{
    static char	    correctStatusBytes[] = T_F_STATUS_BYTES;    
    int	    	    index;

    /* get the index to the status subtype byte */
    if ( bufIndex+2 < bufferLength )
      index = bufIndex + 2;
    else
      index = bufIndex+2 - reportLength;

    /* try checking for space char	*/
    if ( (buffer[index] == correctStatusBytes[2]) || isalpha(buffer[index]) )
      /* normal case  */
      return(T_TRUE);
    else 
      return(T_FALSE);

}	/* checkSubType */


/*****************************************************************************
 *
   xyz_quat_interpret - interpret report
 *
 *****************************************************************************/

const	int	STATUS_LENGTH = 3;	// Length of the status report, in bytes

int vrpn_Tracker_Fastrak::xyz_quat_interpret(void)
{
	int	i;
	float	dataList[50];	// Float list filled in from raw data
	float	*dataPtr;	// Goes through converted float list
	int	numDataItems = (reportLength-STATUS_LENGTH) / sizeof(float);

	// Find out which sensor the report is from by looking in status
	// The number is stored as an ASCII character, with 1 being the first
	// VRPN uses an integer with 0 being the first.
	sensor = buffer[1] - '0' - 1;

	// Skip the header and copy the floats from the raw buffer into the
	// data array, switching endian-ness if needed.
	T_F_REVERSE_BYTES( (unsigned char *)dataList, buffer+STATUS_LENGTH,
		sizeof(float), numDataItems);

	// Point dataPtr at the head of the list, pass through and retrieve
	// the position and orientation
	dataPtr = dataList;

	for (i = 0; i < 3; i++) {	// read position
		pos[i] = *dataPtr++;
	}

	// Q_W is first in the list, but last in the quat array
	quat[Q_W] = *dataPtr++;
	for (i = 0; i < 3; i++) {
		quat[i] = *dataPtr++;
	}

	return 0;
}

/*****************************************************************************
 *
   printBuffer - prints a buffer in hex.  debug routine, should be removed
    in the long run.
 *
 *****************************************************************************/

void vrpn_Tracker_Fastrak::printBuffer()
{
    int	    i;

    fprintf(stderr, "Buffer (%d bytes):", reportLength);

    for ( i = 0; i < totalReportLength; i++ )
      {
	if ( i % 15 == 0 )
	  fprintf(stderr, "\n");
    
	fprintf(stderr, "%3x", (unsigned) buffer[i]);
    }

    fprintf(stderr, "\n");

}	/* printBuffer */




void vrpn_Tracker_Fastrak::printChar(char *buf, int size) {
  int	    i;

  fprintf(stderr, "Buffer (%d bytes):", size);
  
  for ( i = 0; i < size; i++ )
    {
      if ( i % 15 == 0 )
	fprintf(stderr, "\n");
      
      fprintf(stderr, "[%c]", buf[i]);
    }

  fprintf(stderr, "\n");
}

/*****************************************************************************
 *
   t_f_replace_station_char - replace the char for which station in cmd string
 
    input:
	- trackerPtr
    	- command string
	- unit number
	- index into string
    
    output:
    	- string has correct station number
    
 *
 *****************************************************************************/

void vrpn_Tracker_Fastrak::replace_station_char(char* cmdString, 
					       int unitNum, int index)

{

  cmdString[index] = '0'+ (unitNum + 1);

}	/* t_f_replace_station_char */

/*****************************************************************************
 *
   t_f_enable - turn on a specified station/unit of the fastrak;  
    	updates the list and count of open units,  as well as adjusting
	the totalReportLength field for the tracker.
   
    input:
    	- pointer to current tracker table entry
	- which unit to enable
    
    output:
    	- station is turned on in fastrak
	- the tracker table variables totalReportLength, unitsOn, 
	    and numUnits are updated accordingly
   
    notes:
       
    	if the number of units == 0, then this is the first call to enable
   a unit. t_f_init() can't turn off station 1 (unit 0), so it's on, even 
   though nothing has been explicitly turned on.  if whichUnit != 0
   and numUnits == 0, then we have to turn on that unit,
   then turn off station 1.  (if whichUnit == 1, we'll see that it's already
   turned on and do the right thing.)
   
 *
 *****************************************************************************/


int vrpn_Tracker_Fastrak::enable(int whichUnit)
{
    int	    killStationOne = T_FALSE;

    
    /* at startup, there is at least one unit on, which the Fastrak refuses to
     *  turn off.  set a flag if it's startup time so that we'll know to turn
     *  off the 1st station later if necessary.  this flag is necessary because
     *  t_add_unit() updates trackerPtr->numUnits.
     */
    if ( (numUnits == 0) && (whichUnit != 0) )
      killStationOne = T_TRUE;

    fprintf(stderr, "enable \t%s:%d\n", __FILE__, __LINE__);
    /* turn this station on  and set its hemisphere to the default	*/
    if ( set_unit(whichUnit, T_F_ON) == T_ERROR )
      return(T_ERROR);

    /* set hemisphere to be the normal +X	*/
    set_hemisphere(whichUnit, Q_X, 1);
    

    /* update the list of units that are active */
    numUnits++;
    stationArray[whichUnit]= T_F_ON;

    /* update the totalReportLength to reflect this unit's status	*/
    totalReportLength += reportLength;

      /* now that the requested station is on, we can turn off station 1 if it
       *  wasn't really supposed to be on.  this is only true right at startup.
       */
      if ( killStationOne )
	{
	  /* at this point, the Fastrak has station 1 on by default, so
	   *  make the tracker table reflect this;  then turn it off w/ 
	   * t_f_disable().  We can't do this before this point because
	   * the Fastrak won't allow you to turn off all of the stations.
	   * t_f_disable() will update the tracker table appropriately.
	   */

	  /* update the list of units that are active */
	  numUnits++; totalReportLength += reportLength;

	  if (disable(0) == T_ERROR )
	    return(T_ERROR);
	}
    fprintf(stderr,"enable \t%s@%d\n", __FILE__, __LINE__);
    set_data_format(whichUnit);
    fprintf(stderr,"enable \t%s@%d\n", __FILE__, __LINE__);
    return(T_OK);

}	/* t_f_enable */

/*****************************************************************************
 *
   t_f_disable -  turn off a specified station/unit of the fastrak;  
    	updates the list and count of open units,  as well as adjusting
	the totalReportLength field for the tracker.
   
    input:
    	- pointer to current tracker table entry
	- which unit to disable
    
    output:
    	- station is turned off in fastrak unless it's the last station, 
	    which can't be turned off
	- the tracker table variables totalReportLength, unitsOn, 
	    and numUnits are updated accordingly
   
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::disable(int whichUnit)
{

  /* turn this station off if it's not the last one; in which case we can't  */
  if (numUnits > 1 )
    if ( set_unit(whichUnit, T_F_OFF) == T_ERROR )
	return(T_ERROR);

  /* update the list of units that should be active, even if we didn't 
   *   actually turn off that last unit
   */
  stationArray[whichUnit] = T_F_OFF;

  /* update the totalReportLength to reflect this unit's status	*/
  totalReportLength -= reportLength;

  return(T_OK);

}	/* t_f_disable */


/*****************************************************************************
 *
   t_f_set_data_format - set data format for unit in fastrak to quaternions
 
    input:
    	- pointer to current tracker table entry
	- unit number
    
    output:
    	- fastrak is set to report T_F_C_XYZ_QUAT regardless of the value
	    of the tracker's dataFormat (conversion to the correct type
	    will be done later by read routine)
    
    notes:
    	- this should only be called by the enable routine
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::set_data_format(int unitNum)
{

    int	    	numTries = 1;
    char	formatString[64] = T_F_C_XYZ_QUAT;
    strcpy(formatString, T_F_C_XYZ_QUAT);
    


    /* pause continuous mode if on	*/
    if ( poll_mode() == T_ERROR )
      return(T_ERROR);

    /* create format string for this station; station number is 2nd char   */
    replace_station_char(formatString, unitNum, 1);
    fprintf(stderr,"set_data_format \t%s@%d\n", __FILE__, __LINE__);

    do 
      {

	write(serial_fd, (unsigned char *) formatString, 
    	    	    	    	    	    strlen(T_F_C_XYZ_QUAT));
	ms_sleep(50);

	numTries++;
    }
    while ((verify_output_list(unitNum) == T_ERROR) &&
	(numTries <= 5) );

    if ( numTries > 5 )
    {
    fprintf(stderr, "%s:%d:  could not initialize data format in Fastrak.\n",
    	    __FILE__, __LINE__);
    fprintf(stderr, "  Try cycling the power on Fastrak\n");
    return(T_ERROR);
    }

    /* resume continuous mode if nec    */
    if (mode == T_F_M_CONTINUOUS )
      cont_mode();

    return(T_OK);

}	/* t_f_set_data_format */

/*****************************************************************************
 *
   t_f_set_hemisphere - sets the hemisphere for a given unit
   
    input:
    	- pointer to current tracker table entry
	- which unit to set the hemisphere for    
    
    output:
    	- station's hemisphere is set in fastrak 

 *
 *****************************************************************************/

void vrpn_Tracker_Fastrak::set_hemisphere(int whichUnit, int axis, 
					  int sign)
{
    int	    	    i;
    static char     hemisphereString[100];
    
    /* these should be treated as read-only */
    static int	    XYZaxes[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    int	    	    hemiAxis[3];


    /* select which axis via the axis arg, then multiply each element by sign   */
    for ( i = 0; i < 3; i++ )
    hemiAxis[i] = XYZaxes[axis][i] * sign;


    /* set hemisphere for enabled station; see fastrak.h for doc on format    */
    sprintf(hemisphereString, T_F_C_SET_HEMISPHERE, 
    	    	   whichUnit+1, hemiAxis[Q_X], hemiAxis[Q_Y], hemiAxis[Q_Z]);

    write(serial_fd, (unsigned char *) hemisphereString, 
    	    	    	    	    	    	strlen(hemisphereString));
    ms_sleep(50);

}	/* t_f_set_hemisphere */

/*****************************************************************************
 *
   t_f_verify_output_list - verifies that output is set to x,y,z and 
    	    	    	    quaternions
 
    input:
    	- pointer to current tracker table entry
    
    output:
    	- T_TRUE if output is set to x,y,z and quaternions
	- T_FALSE otherwise
    
    notes:  this only verifies that the output list is set to the type
    	    defined in fastrak.h as T_F_C_XYZ_QUAT;  to be general, 
	    it should check accept a string as an input parameter 
	    and parse the fastrak output and then compare them.
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::verify_output_list(int unitNum)
{

    char    	curOutputList[T_F_MAX_CMD_STRING_LENGTH];

    
    //fprintf(stderr,"verify_output_list %p, \t%s:%d\n",this, __FILE__,
	//    __LINE__);

    if ( get_output_list(unitNum, curOutputList) == T_ERROR )
      return(T_ERROR);

    /* note that the station number is not included in either of these strings,
     *  so no need to do a replacement of the unit/station number
     */

    if ( strncmp(curOutputList, T_F_XYZ_QUAT_OUTPUT_LIST, 
		 strlen(T_F_XYZ_QUAT_OUTPUT_LIST)) == 0)
      {     
	return(T_OK);}
    else
    {
      fprintf(stderr, "verify list failed%p, \t%s:%d\n",this, __FILE__, __LINE__);
      /* comparison failed:  see what we got  */
      return(T_ERROR);
    }

}	/* t_f_verify_output_list */


/*****************************************************************************
 *
   t_f_get_output_list -  returns the fastrak's output list, minus status 
    	    	    	    bytes (including station/unit number)
 
    input:
    	- pointer to current tracker table entry
    
    output:
    	- the fastrak's output list, minus status bytes;  eg., " 211"
    
    notes:
    	- verifies that record read in is in fact an output list record
 *
 *****************************************************************************/

int vrpn_Tracker_Fastrak::get_output_list(int unitNum, char * curOutputList)
{
    int	    	    bufLength;
    char    	    this_buffer[T_MAX_READ_BUFFER_SIZE];
    int	    	    numTries = 1;
    int	    	    i;
    char	    retrieveListCmd[64];
    int byteread =0;

    strcpy(retrieveListCmd, T_F_C_RETRIEVE_OUTPUT_LIST);
    //fprintf(stderr,"get_output_list %p, \t%s@%d\n", this, __FILE__, __LINE__);
    bufLength = strlen(T_F_XYZ_QUAT_OUTPUT_LIST) + T_F_STATUS_LENGTH;

/* 
 * loop until success or 5 failures-  send request for output list, verify
 *  number of bytes returned and record type.
 */

    /* get output list:  station number index == 1	*/
    //replace_station_char(retrieveListCmd, unitNum, 1);
    sprintf(retrieveListCmd,"O%d\r", unitNum+1);
    //printf("cmd = [%s], len= %d\n", retrieveListCmd, strlen(retrieveListCmd));
    byteread=0;
    do
    {
      my_flush(); 
      //vrpn_flushInputBuffer(serial_fd);
      //fprintf(stderr, "get_output_list: %d \t%s:%d\n",
	//    numTries, __FILE__, __LINE__);
      if (write(serial_fd,  retrieveListCmd, 
	  strlen(retrieveListCmd))!=3)
	  printf("bad write\n");

      //sleep(1);
      ms_sleep(50);
      byteread = vrpn_read_available_characters(serial_fd,
	(unsigned char *)this_buffer, bufLength);
      if ((byteread == bufLength) && 
      	 (this_buffer[T_F_RECORD_SUBTYPE] == T_F_OUTPUT_LIST_SUBTYPE))
	 break;
      numTries++;	
      printChar(this_buffer, byteread);
    } while (numTries <= 5);
    
    if ( numTries > 5 )
    {
      fprintf(stderr, 
	    " Fastrak is acting up-  could not get output list.\n");
      fprintf(stderr, "  Try cycling the power and trying again.\n"); 
      return(T_ERROR);
    }

    /* otherwise, all is well- copy output list to curOutputList, skipping
     * status bytes (which include station/unit number)
     */

    for ( i = 0; i < bufLength-T_F_STATUS_LENGTH; i++ )
      curOutputList[i] = this_buffer[T_F_STATUS_LENGTH+i];
    curOutputList[i]=0;
    return(T_OK);

}	/* t_f_get_output_list */

#endif  // defined(sgi) || defined(linux)
#endif  // VRPN_CLIENT_ONLY

