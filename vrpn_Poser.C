#ifndef _WIN32_WCE
#include <time.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef	_WIN32_WCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <ctype.h>


// NOTE: a vrpn poser must accept poser data (pos and
//       ori info) which represent the transformation such 
//       that the pos info is the position of the origin of
//       the poser coord sys in the source coord sys space, and the
//       quat represents the orientation of the poser relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the poser's)

//      borrows heavily from the vrpn_Tracker code, as the poser is basically
//      the inverse of a tracker

#ifdef linux
#include <termios.h>
#endif

// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h 
// and unistd.h
#include "vrpn_Shared.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif

#ifdef	_WIN32
#ifndef _WIN32_WCE
#include <io.h>
#endif
#endif

#include "vrpn_Poser.h"


//#define VERBOSE
// #define READ_HISTOGRAM

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}


vrpn_Poser::vrpn_Poser (const char * name, vrpn_Connection * c) : 
    vrpn_BaseClass(name, c)
{
	vrpn_BaseClass::init();

	// Set the current time to zero, just to have something there
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	// Set the position to the origin and the orientation to identity
	// just to have something there in case nobody fills them in later
	pos[0] = pos[1] = pos[2] = 0.0;
	d_quat[0] = d_quat[1] = d_quat[2] = 0.0;
	d_quat[3] = 1.0;

    // Set the velocity to zero and the orientation to identity
	// just to have something there in case nobody fills them in later
	vel[0] = vel[1] = vel[2] = 0.0;
	vel_quat[0] = vel_quat[1] = vel_quat[2] = 0.0;
	vel_quat[3] = 1.0;
	vel_quat_dt = 1;
}

int vrpn_Poser::register_types(void)
{
	// Register this poser device and the needed message types
	if (d_connection) {
        position_m_id = d_connection->register_message_type("vrpn_Poser Pos_Quat");
        velocity_m_id = d_connection->register_message_type("vrpn_Poser Velocity");
	}
	return 0;
}

// virtual
vrpn_Poser::~vrpn_Poser (void) {

}

/*
int vrpn_Poser::register_server_handlers(void)
{
    if (d_connection){
        // Register a handler for the position change callback for this device
 		if (register_autodeleted_handler(position_m_id,
				handle_change_message, this, d_sender_id))
		{
			fprintf(stderr,"vrpn_Poser:can't register position handler\n");
			return -1;
		}

    } else {
		return -1;
	}
    return 0;
}
*/


// NOTE:    you need to be sure that if you are sending vrpn_float64 then 
//          the entire array needs to remain aligned to 8 byte boundaries
//	        (malloced data and static arrays are automatically alloced in
//          this way).  Assumes that there is enough room to store the
//          entire message.  Returns the number of characters sent.
int	vrpn_Poser::encode_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: vrpn_float64 pos[3], vrpn_float64 d_quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, pos[0]);
   vrpn_buffer(&bufptr, &buflen, pos[1]);
   vrpn_buffer(&bufptr, &buflen, pos[2]);

   vrpn_buffer(&bufptr, &buflen, d_quat[0]);
   vrpn_buffer(&bufptr, &buflen, d_quat[1]);
   vrpn_buffer(&bufptr, &buflen, d_quat[2]);
   vrpn_buffer(&bufptr, &buflen, d_quat[3]);

   return 1000 - buflen;
}

int vrpn_Poser::encode_vel_to(char *buf)
{
    char *bufptr = buf;
    int buflen = 1000;

    // Message includes: vrpn_float64 vel[3], vrpn_float64 vel_quat[4], vrpn_float64 vel_quat_dt
    // Byte order of each needs to be reversed to match network standard

    vrpn_buffer(&bufptr, &buflen, vel[0]);
    vrpn_buffer(&bufptr, &buflen, vel[1]);
    vrpn_buffer(&bufptr, &buflen, vel[2]);

    vrpn_buffer(&bufptr, &buflen, vel_quat[0]);
    vrpn_buffer(&bufptr, &buflen, vel_quat[1]);
    vrpn_buffer(&bufptr, &buflen, vel_quat[2]);
    vrpn_buffer(&bufptr, &buflen, vel_quat[3]);

    vrpn_buffer(&bufptr, &buflen, vel_quat_dt);

    return 1000 - buflen;
}



//////////////////////////////////////////////////////////////////////////////////////
// Server Code

vrpn_Poser_Server::vrpn_Poser_Server (const char * name, vrpn_Connection * c) :
    vrpn_Poser(name, c)
{
//	register_server_handlers();

    change_list = NULL;
    velchange_list = NULL;

    // Make sure that we have a valid connection

    if (d_connection == NULL) {
		fprintf(stderr,"vrpn_Poser_Server: No connection\n");
		return;
	}

    // Register a handler for the position change callback for this device
 	if (register_autodeleted_handler(position_m_id,
			handle_change_message, this, d_sender_id)) {
		fprintf(stderr,"vrpn_Poser:can't register position handler\n");
		d_connection = NULL;
	}

    // Register a handler for the velocity change callback for this device
    if (register_autodeleted_handler(velocity_m_id,
			handle_vel_change_message, this, d_sender_id)) {
		fprintf(stderr,"vrpn_Poser:can't register velocity handler\n");
		d_connection = NULL;
	}
}

void vrpn_Poser_Server::mainloop()
{
	// Call the generic server mainloop routine, since this is a server
	server_mainloop();
}

int vrpn_Poser_Server::register_change_handler(void *userdata,
        vrpn_POSERCHANGEHANDLER handler)
{
    vrpn_POSERCHANGELIST *new_entry;

    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr, 
            "vrpn_Poser_Server::register_handler: NULL handler\n");
        return -1;
    }

    // Allocate and initialize the new entry
    if ((new_entry = new vrpn_POSERCHANGELIST) == NULL) {
        fprintf(stderr,
            "vrpn_Poser_Server::register_handler: Out of memory\n");
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

int vrpn_Poser_Server::register_change_handler(void *userdata,
        vrpn_POSERVELCHANGEHANDLER handler)
{
    vrpn_POSERVELCHANGELIST *new_entry;

    // Ensure that the handler is non-NULL
    if (handler == NULL) {
        fprintf(stderr,
            "vrpn_Poser_Server::register_handler: NULL handler\n");
        return -1;
    }

    // Allocate and initialize the new entry
    if ((new_entry = new vrpn_POSERVELCHANGELIST) == NULL) {
        fprintf(stderr,
            "vrpn_Poser_Server::register_handler: Out of memory\n");
        return -1;
    }
    new_entry->handler = handler;
    new_entry->userdata = userdata;

    // Add this handler to the chain at the beginning (don't check to see
    // if it is already there, since duplication is okay).
    new_entry->next = velchange_list;
    velchange_list = new_entry;

    return 0;
}

int vrpn_Poser_Server::unregister_change_handler(void *userdata,
        vrpn_POSERCHANGEHANDLER handler)
{
	// The pointer at *snitch points to victim
	vrpn_POSERCHANGELIST *victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(change_list);
	victim = *snitch;
	while ((victim != NULL) &&
            ((victim->handler != handler) ||
            (victim->userdata != userdata) )) {
	    snitch = &((*snitch)->next);
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		  "vrpn_Poser_Server::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Poser_Server::unregister_change_handler(void *userdata,
        vrpn_POSERVELCHANGEHANDLER handler)
{
// The pointer at *snitch points to victim
	vrpn_POSERVELCHANGELIST *victim, **snitch;

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(velchange_list);
	victim = *snitch;
	while ((victim != NULL) &&
            ((victim->handler != handler) ||
            (victim->userdata != userdata) )) {
	    snitch = &((*snitch)->next);
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		  "vrpn_Poser_Server::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

int vrpn_Poser_Server::handle_change_message(void *userdata,
	    vrpn_HANDLERPARAM p)
{
	vrpn_Poser_Server *me = (vrpn_Poser_Server *)userdata;
	const char *params = (p.buffer);
	vrpn_POSERCB pp;
	vrpn_POSERCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the poser from the message
	if (p.payload_len != (7*sizeof(vrpn_float64)) ) {
		fprintf(stderr,"vrpn_Poser: change message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 7*sizeof(vrpn_float64) );
		return -1;
	}
	pp.msg_time = p.msg_time;

	for (i = 0; i < 3; i++) {
	 	vrpn_unbuffer(&params, &pp.pos[i]);
	}
	for (i = 0; i < 4; i++) {
		vrpn_unbuffer(&params, &pp.quat[i]);
	}

    // XXX 
    // Should we check the pose against max/min values here?
    // Automatically send back a message here???

	handler = me->change_list;
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, pp);
		handler = handler->next;
	}

    return 0;
}

int vrpn_Poser_Server::handle_vel_change_message(void *userdata,
        vrpn_HANDLERPARAM p)
{
    vrpn_Poser_Server *me = (vrpn_Poser_Server *)userdata;
	const char *params = (p.buffer);
	vrpn_POSERVELCB pp;
	vrpn_POSERVELCHANGELIST *handler;
	int	i;

	// Fill in the parameters to the poser from the message
	if (p.payload_len != (8*sizeof(vrpn_float64)) ) {
		fprintf(stderr,"vrpn_Poser: velocity message payload error\n");
		fprintf(stderr,"             (got %d, expected %d)\n",
			p.payload_len, 8*sizeof(vrpn_float64) );
		return -1;
	}
	pp.msg_time = p.msg_time;

	for (i = 0; i < 3; i++) {
	 	vrpn_unbuffer(&params, &pp.vel[i]);
	}
	for (i = 0; i < 4; i++) {
		vrpn_unbuffer(&params, &pp.vel_quat[i]);
	}
    vrpn_unbuffer(&params, &pp.vel_quat_dt);

    // XXX 
    // Should we check the velocity against max/min values here?
    // Automatically send back a message here???

	handler = me->velchange_list;
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	while (handler != NULL) {
		handler->handler(handler->userdata, pp);
		handler = handler->next;
	}

    return 0;
}



//////////////////////////////////////////////////////////////////////////////////////
// Client Code

vrpn_Poser_Remote::vrpn_Poser_Remote (const char * name, vrpn_Connection *cn) :
	vrpn_Poser (name, cn)
{
	// Make sure that we have a valid connection
	if (d_connection == NULL) {
		fprintf(stderr,"vrpn_Poser_Remote: No connection\n");
		return;
	}
	
	// Find out what time it is and put this into the timestamp
	gettimeofday(&timestamp, NULL);
}

// The remote poser has to un-register its handlers when it
// is destroyed to avoid seg faults (this is taken care of by
// using autodeleted handlers above). It should also remove all
// remaining user-registered callbacks to free up memory.

vrpn_Poser_Remote::~vrpn_Poser_Remote()
{
	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the lists
}

void vrpn_Poser_Remote::mainloop()
{
	if (d_connection) { d_connection->mainloop(); }
	client_mainloop();
}

int vrpn_Poser_Remote::set_pose(struct timeval t, 
                                vrpn_float64 position[3], 
                                vrpn_float64 quaternion[4]) 
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Update the time
    timestamp.tv_sec = t.tv_sec;
    timestamp.tv_usec = t.tv_usec;

    // Pack position request
    memcpy(pos, position, sizeof(pos));
    memcpy(d_quat, quaternion, sizeof(d_quat));
    len = encode_to(msgbuf);
    if (d_connection->pack_message(len, timestamp,
        position_m_id, d_sender_id, msgbuf,
        vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}

int vrpn_Poser_Remote::set_pose_velocity(struct timeval t, 
                                            vrpn_float64 position[3], 
                                            vrpn_float64 quaternion[4], 
                                            vrpn_float64 interval)
{
    char msgbuf[1000];
    vrpn_int32 len;

    // Update the time
    timestamp.tv_sec = t.tv_sec;
    timestamp.tv_usec = t.tv_usec;

    // Pack velocity request
    memcpy(vel, position, sizeof(pos));
    memcpy(vel_quat, quaternion, sizeof(vel_quat));
    vel_quat_dt = interval;
    len = encode_vel_to(msgbuf);
    if (d_connection->pack_message(len, timestamp,
        velocity_m_id, d_sender_id, msgbuf,
        vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr, "vrpn_Poser_Remote: can't write a message: tossing\n");
        return -1;
    }

    return 0;
}