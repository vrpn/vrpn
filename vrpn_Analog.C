/*
 * Author          : Ruigang Yang
 * Created On      : Tue Mar 17 16:01:46 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Thu May  7 11:40:20 1998
 * Update Count    : 47
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Analog.C,v $
 * $Date: 1999/02/01 20:53:06 $
 * $Author: weberh $
 * $Revision: 1.10 $
 * 
 * $Log: vrpn_Analog.C,v $
 * Revision 1.10  1999/02/01 20:53:06  weberh
 * cleaned up and added nice and multithreading for practical usage.
 *
 * Revision 1.9  1999/01/29 22:18:26  weberh
 * cleaned up analog.C and added class to support national instruments a/d card
 *
 * Revision 1.8  1999/01/29 19:42:01  weberh
 * cleaned up analog and shared code, added new class to read national
 * instrument d/a card.
 *
 * Revision 1.7  1998/12/02 19:44:53  hudson
 * Converted JoyFly so it could run on the same server as the Joybox whose
 * data it is processing.  (Previously they had to run on different servers,
 * which could add significant latency to the tracker and make startup awkward.)
 * Added some header comments to vrpn_Serial and some consts to vrpn_Tracker.
 *
 * Revision 1.6  1998/11/23 23:24:04  lovelace
 * Fixed bug in vrpn_Button.C that only showed up under Win32 CLIENT_ONLY.
 * Also, in vrpn_Analog.C fixed <unistd.h> to not be included under Win32.
 * Updated Win32 project file and added VC++ 5.0 NMake file (vrpn.mak).
 *
 * Revision 1.5  1998/11/05 22:45:41  taylorr
 * This version strips out the serial-port code into vrpn_Serial.C.
 *
 * It also makes it so all the library files compile under NT.
 *
 * It also fixes an insidious initialization bug in the forwarder code.
 *
 * Revision 1.4  1998/06/26 15:48:50  hudson
 * Wrote vrpn_FileConnection.
 * Changed connection naming convention.
 * Changed all the base classes to reflect the new naming convention.
 * Added #ifdef sgi around vrpn_sgibox.
 *
 * Revision 1.3  1998/05/14 14:45:23  ryang
 * modified vrpn_sgibox, so output is clamped to +-0.5, max 2 rounds
 *
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

static char rcsid[] = "$Id: vrpn_Analog.C,v 1.10 1999/02/01 20:53:06 weberh Exp $";

#include "vrpn_Analog.h"
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

extern int vrpn_open_commport(char *portname, long baud);

vrpn_Analog::vrpn_Analog (const char * name, vrpn_Connection * c) {
  // If the connection is valid, use it to register this button by
   // name and the button change report by name.
  char * servicename;
  servicename = vrpn_copy_service_name(name);
   connection = c;
   if (connection != NULL) {
      my_id = connection->register_sender(servicename);
      channel_m_id = connection->register_message_type("Analog Channel");
      if ( (my_id == -1) || (channel_m_id == -1) ) {
      	fprintf(stderr,"vrpn_Analog: Can't register IDs\n");
         connection = NULL;
      }
   }
   num_channel = 0;
   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
  if (servicename)
    delete [] servicename;
}

void vrpn_Analog::print(void ) {
  printf("Analog Report: ");
  for (int i=0; i< num_channel; i++) {
    //    printf("Channel[%d]= %f\t", i, channel[i]);
    printf("%f\t", channel[i]);
  }
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
vrpn_Serial_Analog::vrpn_Serial_Analog (const char * name, vrpn_Connection * c,
				        const char * port, int baud) :
         vrpn_Analog(name, c) {
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


// ************* CLIENT ROUTINES ****************************

vrpn_Analog_Remote::vrpn_Analog_Remote (const char * name,
                                        vrpn_Connection * c) : 
	vrpn_Analog (name, c ? c : vrpn_get_connection_by_name(name)),
	change_list (NULL)
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

	// At the start, as far as the client knows, the device could have
	// max channels -- the number of channels is specified in each
	// message.
	num_channel=vrpn_CHANNEL_MAX;
	for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
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

	cp.msg_time = p.msg_time;
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




