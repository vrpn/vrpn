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
 * Created On      : Tue Mar 17 16:01:46 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Wed May  6 13:02:11 1998
 * Update Count    : 46
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Analog.C,v $
 * $Date: 1998/05/06 18:00:37 $
 * $Author: ryang $
 * $Revision: 1.2 $
 * 
 * $Log: vrpn_Analog.C,v $
 * Revision 1.2  1998/05/06 18:00:37  ryang
 * v0.1 of vrpn_sgibox
 *
 * Revision 1.1  1998/03/18 23:12:21  ryang
 * new analog device and joystick channell
 * D
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

static char rcsid[] = "$Id: vrpn_Analog.C,v 1.2 1998/05/06 18:00:37 ryang Exp $";

#include "vrpn_Analog.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int vrpn_open_commport(char *portname, long baud);

vrpn_Analog::vrpn_Analog(char *name, vrpn_Connection *c) {
  // If the connection is valid, use it to register this button by
   // name and the button change report by name.
   connection = c;
   if (connection != NULL) {
      my_id = connection->register_sender(name);
      channel_m_id = connection->register_message_type("Analog Channel");
      if ( (my_id == -1) || (channel_m_id == -1) ) {
      	fprintf(stderr,"vrpn_Analog: Can't register IDs\n");
         connection = NULL;
      }
   }
   num_channel = 0;
   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
}

void vrpn_Analog::print(void ) {
  printf("Analog Report:");
  for (int i=0; i< num_channel; i++) 
    printf("Channel[%d]= %f\t", i, channel[i]);
  printf("\n");
}

int vrpn_Analog::encode_to(char *buf)
{
   // Message includes: long AnalogNum, long state
   // Byte order of each needs to be reversed to match network standard

   double *longbuf = (double *)(buf);
   int	index = 0;

   longbuf[index++]= htond((double) num_channel);
   for (int i=0; i< num_channel; i++) {
     longbuf[index++] = htond(channel[i]);
     last[i] = channel[i];
   }
   //fprintf(stderr, "encode___ %x %x", buf[0], buf[1]);
   return index*sizeof(double);
}
void vrpn_Analog::report_changes() {
  int i;
  int change = 0;
  if (connection) {
    for (i = 0; i < num_channel; i++) {
      if (channel[i]!= last[i]) change =1;
      last[i] = channel[i];
    }
    if (!change) return;
      
    // there is indeed some change, send it;
    gettimeofday(&timestamp, NULL);
    char	msgbuf[1000];
    int	len = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
    print();
#endif
    if (connection->pack_message(len, timestamp,
				 channel_m_id, my_id, msgbuf,
				 vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"vrpn_Analog: cannot write message: tossing\n");
    }
  }else 
    fprintf(stderr,"vrpn_Analog: No valid connection\n");
}

#ifndef VRPN_CLIENT_ONLY
#ifndef	_WIN32
vrpn_Serial_Analog::vrpn_Serial_Analog(char *name, vrpn_Connection *c,
				       char *port, int baud): vrpn_Analog(name, c) {
   // Find out the port name and baud rate;
   if (port == NULL) {
	fprintf(stderr,"vrpn_Analog_Serial: NULL port name\n");
	status = ANALOG_FAIL;
	return;
   } else {
	strncpy(portname, port, sizeof(portname));
	portname[sizeof(portname)-1] = '\0';
   }
   baudrate = baud;

   // Open the serial port we're going to use
   if ( (serial_fd=vrpn_open_commport(portname, baudrate)) == -1) {
	fprintf(stderr,"vrpn_Tracker_Serial: Cannot Open serial port\n");
	status = ANALOG_FAIL;
   }

   // Reset the tracker and find out what time it is
   status = ANALOG_RESETTING;
   gettimeofday(&timestamp, NULL);
}
#endif  // _WIN32
#endif  // VRPN_CLIENT_ONLY


#ifndef VRPN_CLIENT_ONLY

// This routine will read any available characters from the handle into
// the buffer specified, up to the number of bytes requested.  It returns
// the number of characters read or -1 on failure.  Note that it only
// reads characters that are available at the time of the read, so less
// than the requested number may be returned.
#ifndef _WIN32
int vrpn_Serial_Analog::read_available_characters(char *buffer,
	int bytes)
{
   int bRead;

   // on sgi's (and possibly other architectures) the folks from 
   // ascension have noticed that a read command will not necessarily
   // read everything available in the read buffer (see the following file:
   // ~hmd/src/libs/tracker/apps/ascension/FLOCK232/C/unix.txt
   // For this reason, we loop and try to fill the buffer, stopping our
   // loop whenever no chars are available.
   int cReadThisTime = 0;
   int cSpaceLeft = bytes;
   char *pch = buffer;

   do {
     if ( (cReadThisTime = read(serial_fd, (char *)pch, cSpaceLeft)) == -1) {
       perror("Tracker: cannot read from serial port");
       fprintf(stderr, "this = %p, buffer = %p, %d\n", this, pch, bytes);  
       return -1;
     }
     cSpaceLeft -= cReadThisTime;
     pch += cReadThisTime;
   }  while ((cReadThisTime!=0) && (cSpaceLeft>0));
   bRead = pch - buffer;
 

   if (bRead > 100 || bRead < 0) 
     fprintf(stderr, "  vrpn_Serial_Analog: Read %d bytes\n",bRead);

   return bRead;
}
#endif  // _WIN32
#endif // vrpn_client_only

// ************* CLIENT ROUTINES ****************************

vrpn_Analog_Remote::vrpn_Analog_Remote(char *name) : 
	vrpn_Analog(name, vrpn_get_connection_by_name(name)),
	change_list(NULL)
{
	int	i;

	// Register a handler for the change callback from this device,
	// if we got a connection.
	if (connection != NULL) {
	  if (connection->register_handler(channel_m_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Analog_Remote: can't register handler\n");
		connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Analog_Remote: Can't get connection!\n");
	}

	// XXX These should be read from a description message that comes
	// from the Analog device (as a response to a query?).  For now,
	// we assume a JoyStick
	num_channel = 7;
	for (i = 0; i < num_channel; i++) {
		channel[i] = last[i] = 0;
	}
	gettimeofday(&timestamp, NULL);
}

void	vrpn_Analog_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); 
	}
}

int vrpn_Analog_Remote::register_change_handler(void *userdata,
			vrpn_ANALOGCHANGEHANDLER handler)
{
	vrpn_ANALOGCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_Analog_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_ANALOGCHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_Analog_Remote::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
	new_entry->next = change_list;
	change_list = new_entry;

	return 0;
}

int vrpn_Analog_Remote::unregister_change_handler(void *userdata,
			vrpn_ANALOGCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_ANALOGCHANGELIST	*victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &change_list;
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) )) {
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		   "vrpn_Analog_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Analog_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Analog_Remote *me = (vrpn_Analog_Remote *)userdata;
	double *params = (double*)(p.buffer);
	vrpn_ANALOGCB	cp;
	vrpn_ANALOGCHANGELIST *handler = me->change_list;

	//fprintf(stderr, "Analog_Remote::handle_chg_msg\n");
	// Fill in the parameters to the button from the message;

	// XXX here we assume that analog device is a Joystick     ****

	if (p.payload_len != (7+1)*sizeof(double)) {
		fprintf(stderr,"vrpn_Analog: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 2*sizeof(long));
		return -1;
	}
	cp.msg_time = p.msg_time;
	//char * buf= (char *) p.buffer;

	cp.num_channel = ntohd(params[0]);

	double * chandata = (double *) params+1;
	for (int i=0; i< cp.num_channel; i++) 
	  cp.channel[i] = ntohd(chandata[i]);

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, cp);
		handler = handler->next;
	}

	return 0;
}




