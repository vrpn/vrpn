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
 * Created On      : Tue Mar 17 17:14:01 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Tue Mar 24 21:13:57 1998
 * Update Count    : 60
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_Joystick.C,v $
 * $Date: 1998/03/25 22:09:24 $
 * $Author: taylorr $
 * $Revision: 1.3 $
 * 
 * $Log: vrpn_Joystick.C,v $
 * Revision 1.3  1998/03/25 22:09:24  taylorr
 * Compiles under g++
 *
 * Revision 1.2  1998/03/25 02:15:24  ryang
 * add button report function
 *
 * Revision 1.1  1998/03/18 23:12:22  ryang
 * new analog device and joystick channell
 * D
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

static char rcsid[] = "$Id: vrpn_Joystick.C,v 1.3 1998/03/25 22:09:24 taylorr Exp $";

#include "vrpn_Joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static const double JoyScale[] = {1019, 227, 208, 400, 200, 213, 422};

static long  duration(struct timeval t1, struct timeval t2)
{
  if (t2.tv_sec == -1) return 0;
  return (t1.tv_usec - t2.tv_usec) +
    1000000L * (t1.tv_sec - t2.tv_sec);
}

vrpn_Joystick::vrpn_Joystick(char * name, 
		    vrpn_Connection * c, char * portname,int baud, 
			     double update_rate):
      vrpn_Serial_Analog(name, c, portname, baud), vrpn_Button(name, c)
{ 
  num_buttons = 2;  // only has 2 buttons
  num_channel = 7;
  for(int i=0; i<7; i++) resetval[i] = -1;
  MAX_TIME_INTERVAL = 1000000/update_rate;
}


void vrpn_Joystick::mainloop(void) {
  switch (status) {
  case ANALOG_REPORT_READY:

    report_changes(); // report any button event;

    // Send the message on the connection;
    if (vrpn_Analog::connection) {
      char	msgbuf[1000];
      int	len = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
      print();
#endif
      if (vrpn_Analog::connection->pack_message(len, vrpn_Analog::timestamp,
				   channel_m_id, vrpn_Analog::my_id, msgbuf,
				   vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}
	} else {
	  fprintf(stderr,"Tracker Fastrak: No valid connection\n");
	}
        
	// Ready for another report
	status = ANALOG_SYNCING;
    break;
  case ANALOG_SYNCING:
  case ANALOG_PARTIAL:
    {	
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	if ( duration(current_time,vrpn_Analog::timestamp) < MAX_TIME_INTERVAL) {
		get_report();
	} else {
	  //get_report();

// This code does not compile with g++
// The first code does not compile under CC.
// What a mess.
#ifdef __GNUC__
	  memcpy((char*)(&vrpn_Analog::timestamp), &current_time,
		sizeof(current_time));
#else
	  vrpn_Analog::timestamp.tv_sec = current_time.tv_sec;
	  vrpn_Analog::timestamp.tv_usec = current_time.tv_usec;
#endif
	  status = ANALOG_REPORT_READY;
		// send out the last report again;
	}
    }
    break;
  case ANALOG_RESETTING:
  case ANALOG_FAIL:
    reset();
    break;
  }
}

/****************************************************************************/
/* Send request for status to joysticks and reads response.
*/
void vrpn_Joystick::reset() {
  char request[256];
  int write_rt, bytesread;

  fprintf(stderr, "Going into Joystick::reset()\n");
    /* Request baseline report for comparison */
    request[0] = 'r';
    request[1]  = 0;
    write_rt = write(serial_fd, request, sizeof(request));
    if (write_rt < 0) {
      fprintf(stderr, "vrpn_Joystick::reset: write failed\n");
      status = ANALOG_FAIL;
      return;
    }
    sleep(1);
    
    bytesread= read_available_characters(buffer, 16);
    if (bytesread != 16) {
      fprintf(stderr, "vrpn_Joystick::reset: got only %d char from \
joystick, should be 16, trying again\n", bytesread);
      status = ANALOG_FAIL;
    }
    else{  
      status = ANALOG_REPORT_READY;
      for (int i=0; i< num_channel; i++) { 
	parse(i*2);
      }
      /* only need report when state has changed */
      request[0] = 'j';
      write(serial_fd, request, sizeof(request));
    }
}

void vrpn_Joystick::get_report() {
  int bytesread =0;
  if (status == ANALOG_SYNCING) { 
    bytesread =read_available_characters(buffer, 1);
    if (bytesread == 1 && (buffer[0] >> 7) == 0) {
      status = ANALOG_PARTIAL;
    }
  }
  bytesread = read_available_characters(buffer+1, 1);
  if (bytesread  ==0)  
	return;
  parse(0);
  bytesread = read_available_characters(buffer, 2);
  while (bytesread ==2) {
    parse(0);
    bytesread = read_available_characters(buffer, 2);
  }
  status = ANALOG_REPORT_READY;
}

/****************************************************************************/
/* Decodes bytes as follows:
     First byte of set recieved (from high order bit down):
        -bit == 0 to signify first byte
        -empty bit
        -3 bit channel label
        -3 bits (out of 10) of channel reading.
     Second byte of set recieved (from high order bit down):
        -bit == 1 to signify second byte
        -7 bits (out of 10)of channel reading.
 * Returns 1 if a complete set of bytes has been read.
 * Returns 0 if another byte is needed to complete the set.
*/


void vrpn_Joystick::parse (int index)
{

   static unsigned int temp;
   static unsigned int mask1 = 7, mask2 = 127, left = 2, right = 1;
   
   int chan;
   int value;
   
   chan = buffer[index] >> 3; /* isolate channel */
   if (chan > 7) return; // chan 7 is the button;
   if (chan == 7) {
     /******************************************************************
     *  The least most significant bit should be the left button and the next
     *  should be the right button.  In the original reading the two bits are
     *  opposite this.  shall we swaps the two least significant bits. */;
     
     buttons[0] = ( buffer[index+1] &  left)?1:0;
     buttons[1] = ( buffer[index+1] &  right)?1:0;
     return ;
   }

   temp = buffer[index] & mask1;  /* isolate  value bits */
   temp = temp << 7;              /* position  value bits */
   
   /* strip off check bit */
   value = (buffer[index+1] & mask2) | temp;
   if (resetval[chan] == -1) resetval[chan]= value;
   else channel[chan] = (value-resetval[chan])/JoyScale[chan];
   if (channel[chan] > 0.5) channel[chan] = 0.5;
   else if (channel[chan] < -0.5) channel[chan] = -0.5;
   //printf("Joystick::parse: channel[%d] = %f\n", chan, value);
}                     /* end px_sparse */













