#include "vrpn_Analog.h"
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "vrpn_cygwin_hack.h"

//#define VERBOSE

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
  for (vrpn_int32 i=0; i< num_channel; i++) {
    //    printf("Channel[%d]= %f\t", i, channel[i]);
    printf("%f\t", channel[i]);
  }
  printf("\n");
}

vrpn_Connection *vrpn_Analog::connectionPtr() {
  return connection;
}

vrpn_int32 vrpn_Analog::encode_to(char *buf)
{
   // Message includes: vrpn_float64 AnalogNum, vrpn_float64 state
   // Byte order of each needs to be reversed to match network standard
   // XXX This is passing an int in the double for the num_channel

   vrpn_float64 *longbuf = (vrpn_float64 *)(buf);
   vrpn_int32	index = 0;

   longbuf[index++]= htond((vrpn_float64) num_channel);
   for (vrpn_int32 i=0; i< num_channel; i++) {
     longbuf[index++] = htond(channel[i]);
     last[i] = channel[i];
   }
   //fprintf(stderr, "encode___ %x %x", buf[0], buf[1]);
   return index*sizeof(vrpn_float64);
}
void vrpn_Analog::report_changes (void) {

  vrpn_int32 i;
  vrpn_int32 change = 0;

  if (connection) {
    for (i = 0; i < num_channel; i++) {
      if (channel[i] != last[i]) change =1;
      last[i] = channel[i];
    }
    if (!change) {

#ifdef VERBOSE
    fprintf(stderr, "No change.\n");
#endif

      return;
    }
  }
      
  // there is indeed some change, send it;
  report();
}

void vrpn_Analog::report (void) {

    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf [125];
    char * msgbuf = (char *) fbuf;

    vrpn_int32  len;

    gettimeofday(&timestamp, NULL);
    len = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
    print();
#endif
    if (connection && connection->pack_message(len, timestamp,
                                 channel_m_id, my_id, msgbuf,
                                 vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"vrpn_Analog: cannot write message: tossing\n");
    }
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


vrpn_Analog_Server::vrpn_Analog_Server (const char * name,
                                        vrpn_Connection * c) :
    vrpn_Analog (name, c) {

  num_channel = 0;

}

//virtual
vrpn_Analog_Server::~vrpn_Analog_Server (void) {

}

//virtual
void vrpn_Analog_Server::report_changes (void) {
  vrpn_Analog::report_changes();
}

//virtual
void vrpn_Analog_Server::report (void) {
  vrpn_Analog::report();
}

vrpn_float64 * vrpn_Analog_Server::channels (void) {
  return channel;
}

vrpn_int32 vrpn_Analog_Server::numChannels (void) const {
  return num_channel;
}

vrpn_int32 vrpn_Analog_Server::setNumChannels (vrpn_int32 sizeRequested) {
  if (sizeRequested < 0) sizeRequested = 0;
  if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;

  num_channel = sizeRequested;

  return num_channel;
}


// ************* CLIENT ROUTINES ****************************

vrpn_Analog_Remote::vrpn_Analog_Remote (const char * name,
                                        vrpn_Connection * c) : 
	vrpn_Analog (name, c ? c : vrpn_get_connection_by_name(name)),
	change_list (NULL)
{
	vrpn_int32	i;

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

void	vrpn_Analog_Remote::mainloop(const struct timeval * timeout)
{
  if (connection) { 
    connection->mainloop(timeout); 
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
	vrpn_float64 *params = (vrpn_float64*)(p.buffer);
	vrpn_ANALOGCB	cp;
	vrpn_ANALOGCHANGELIST *handler = me->change_list;

	cp.msg_time = p.msg_time;
	cp.num_channel = (long)ntohd(params[0]);

	vrpn_float64 * chandata = (vrpn_float64 *) params+1;
	for (vrpn_int32 i=0; i< cp.num_channel; i++) 
	  cp.channel[i] = ntohd(chandata[i]);

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, cp);
		handler = handler->next;
	}

	return 0;
}




