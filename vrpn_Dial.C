#include "vrpn_Dial.h"
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "vrpn_cygwin_hack.h"

//#define VERBOSE

vrpn_Dial::vrpn_Dial (const char * name, vrpn_Connection * c) {
  // If the connection is valid, use it to register this button by
   // name and the button change report by name.
  char * servicename;
  servicename = vrpn_copy_service_name(name);
   connection = c;
   if (connection != NULL) {
      my_id = connection->register_sender(servicename);
      change_m_id = connection->register_message_type("Dial update");
      if ( (my_id == -1) || (change_m_id == -1) ) {
      	fprintf(stderr,"vrpn_Dial: Can't register IDs\n");
         connection = NULL;
      }
   }
   num_dials = 0;
   // Set the time to 0 just to have something there.
   timestamp.tv_usec = timestamp.tv_sec = 0;
  if (servicename)
    delete [] servicename;
}

vrpn_Connection *vrpn_Dial::connectionPtr() {
  return connection;
}

vrpn_int32	vrpn_Dial::encode_to(char *buf, vrpn_int32 buflen, vrpn_int32 dial, vrpn_float64 delta)
{
	vrpn_int32	buflensofar = buflen;

	// Message includes: vrpn_int32 dialNum, vrpn_float64 delta
	// We pack them with the delta first, so that everything is aligned
	// on the proper boundary.

	if (vrpn_buffer( &buf, &buflensofar, delta)) {
		fprintf(stderr,"vrpn_Dial::encode_to: Can't buffer delta\n");
		return -1;
	}
	if (vrpn_buffer( &buf, &buflensofar, dial)) {
		fprintf(stderr,"vrpn_Dial::encode_to: Can't buffer dial\n");
		return -1;
	}
	return sizeof(vrpn_float64) + sizeof(vrpn_int32);
}

// This will report any changes that have appeared in the dial values,
// and then clear the dials back to 0. It only sends dials that have
// changed (nonzero values)
void vrpn_Dial::report_changes (void) {

  char	msgbuf[1000];
  vrpn_int32 i;
  vrpn_int32 len;

  if (connection) {
    for (i = 0; i < num_dials; i++) {
	if (dials[i] != 0) {
		len = encode_to(msgbuf, sizeof(msgbuf), i, dials[i]);
		if (connection->pack_message(len, timestamp,
		    change_m_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      			fprintf(stderr,"vrpn_Dial: can't write message: tossing\n");
      		}
		dials[i] = 0;	// We've reported it the change.
        }
    }
  }
}

// This will report the dials whether they have changed or not; this
// will result in some 0 dial values being sent. Normally, code will
// use the report_changes method to avoid flooding the system with
// un-needed messages.
// It then clears the dials back to 0.
void vrpn_Dial::report (void) {

  char	msgbuf[1000];
  vrpn_int32 i;
  vrpn_int32 len;

  if (connection) {
    for (i = 0; i < num_dials; i++) {
		len = encode_to(msgbuf, sizeof(msgbuf), i, dials[i]);
		if (connection->pack_message(len, timestamp,
		    change_m_id, my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
      			fprintf(stderr,"vrpn_Dial: can't write message: tossing\n");
      		}
		dials[i] = 0;	// We've reported it the change.
    }
  }
}

// ************* CLIENT ROUTINES ****************************

vrpn_Dial_Remote::vrpn_Dial_Remote (const char * name,
                                        vrpn_Connection * c) : 
	vrpn_Dial (name, c ? c : vrpn_get_connection_by_name(name)),
	change_list (NULL)
{
	vrpn_int32	i;

	// Register a handler for the change callback from this device,
	// if we got a connection.
	if (connection != NULL) {
	  if (connection->register_handler(change_m_id, handle_change_message,
	    this, my_id)) {
		fprintf(stderr,"vrpn_Dial_Remote: can't register handler\n");
		connection = NULL;
	  }
	} else {
		fprintf(stderr,"vrpn_Dial_Remote: Can't get connection!\n");
	}

	// At the start, as far as the client knows, the device could have
	// max channels -- the number of channels is specified in each
	// message.
	num_dials=vrpn_DIAL_MAX;
	for (i = 0; i < vrpn_DIAL_MAX; i++) {
		dials[i] = 0.0;
	}
	gettimeofday(&timestamp, NULL);
}

void	vrpn_Dial_Remote::mainloop(const struct timeval * timeout)
{
  if (connection) { 
    connection->mainloop(timeout); 
  }
}

int vrpn_Dial_Remote::register_change_handler(void *userdata,
			vrpn_DIALCHANGEHANDLER handler)
{
	vrpn_DIALCHANGELIST	*new_entry;

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_DIAL_Remote::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpn_DIALCHANGELIST) == NULL) {
		fprintf(stderr,
		    "vrpn_Dial_Remote::register_handler: Out of memory\n");
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

int vrpn_Dial_Remote::unregister_change_handler(void *userdata,
			vrpn_DIALCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_DIALCHANGELIST	*victim, **snitch;

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
		   "vrpn_Dial_Remote::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Dial_Remote::handle_change_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
	vrpn_Dial_Remote *me = (vrpn_Dial_Remote *)userdata;
	vrpn_DIALCB	cp;
	vrpn_DIALCHANGELIST *handler = me->change_list;
	const char	*bufptr = p.buffer;

	cp.msg_time = p.msg_time;
	vrpn_unbuffer(&bufptr, &cp.change);
	vrpn_unbuffer(&bufptr, &cp.dial);

	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, cp);
		handler = handler->next;
	}

	return 0;
}




