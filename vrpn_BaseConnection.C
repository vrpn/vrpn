

//////////////////////////////////////////////
// List of connections that are already open
// 
typedef	struct vrpn_CONS_LIST_STRUCT {
	char	name[1000];	// The name of the connection
	vrpn_BaseConnection	*c;	// The connection
	struct vrpn_CONS_LIST_STRUCT *next;	// Next on the list
} vrpn_CONNECTION_LIST;


vrpn_BaseConnection::vrpn_BaseConnection(
	ConnectionControllerCallbackInterface* ccci,
	char * local_logfile_name,
	vrpn_int32 local_log_mode,
	char * remote_logfile_name,
	vrpn_int32 remote_logmode
	):
	d_callback_interface_ptr(ccci),
	d_local_logmode(local_log_mode),
	d_remote_logmode(remote_log_mode),
	num_registered_remote_services (0),
    num_registered_remote_types (0)
{

	vrpn_int32 i;

	// Set all of the local IDs to -1, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).
	num_registered_remote_services = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_SERVICES; i++) {
		registered_remote_services[i].local_id = -1;
		registered_remote_services[i].name = NULL;
	}

	num_registered_remote_types = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		registered_remote_types[i].local_id = -1;
		registered_remote_types[i].name = NULL;
	}


	// Set up the handlers for the system messages.  Skip any ones
	// that we don't know how to handle.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		system_messages[i] = NULL;
	}
	system_messages[-vrpn_CONNECTION_SERVICE_DESCRIPTION] =
		handle_service_message;
	system_messages[-vrpn_CONNECTION_TYPE_DESCRIPTION] =
		handle_type_message;
	system_messages[-vrpn_CONNECTION_UDP_DESCRIPTION] =
		handle_UDP_message;
	system_messages[-vrpn_CONNECTION_LOG_DESCRIPTION] =
	        handle_log_message;

  
	// initialize logging data member values
   if (local_logfile_name) {
	   d_local_logname = new char [1 + strlen(local_logfile_name)];
	   if (!d_local_logname) {
		   fprintf(stderr, "vrpn_BaseConnection::vrpn_Connection:  "
				   "Out of memory!\n");
		   status = BROKEN;
		   return;
	   }
	   strcpy(d_local_logname, local_logfile_name);
   }

   if (remote_logfile_name) {
	   d_remote_logname = new char [1 + strlen(remote_logfile_name)];
	   if (!d_remote_logname) {
		   fprintf(stderr, "vrpn_BaseConnection::vrpn_Connection:  "
				   "Out of memory!\n");
		   status = BROKEN;
		   return;
	   }
	   strcpy(d_remote_logname, remote_logfile_name);
   }

}

vrpn_BaseConnection::~vrpn_BaseConnection()
{
	// delete dynamically allocated memory
	if(d_local_logname)
		delete d_local_logname;
	if(d_remote_logname)
		delete d_remote_logname;
}

//**************************************************************************
//**************************************************************************
//
// vrpn_BaseConnection: protected: system message handlers
//
//**************************************************************************
//**************************************************************************

//static
vrpn_int32 vrpn_BaseConnection::handle_log_message (
	void * userdata,
	vrpn_HANDLERPARAM p) 
{
	vrpn_BaseConnection * me = (vrpn_BaseConnection *) userdata;
	vrpn_int32 retval = 0;

	// if we're already logging, ignore the name (?)
	if (!me->d_local_logname) {
		me->d_local_logname = new char [p.payload_len];
		if (!me->d_local_logname) {
			fprintf(stderr, "vrpn_BaseConnection::handle_log_message:  "
					"Out of memory!\n");
			return -1;
		}
		strncpy(me->d_local_logname, p.buffer, p.payload_len);
		me->d_local_logname[p.payload_len - 1] = '\0';
		retval = me->open_log();

		// Safety check:
		// If we can't log when the client asks us to, close the connection.
		// Something more talkative would be useful.
		// The problem with implementing this is that it's over-strict:  clients
		// that assume logging succeeded unless the connection was dropped
		// will be running on the wrong assumption if we later change this to
		// be a notification message.
		//if (retval == -1) {
		//drop_connection();
		//status = DROPPED;
		//}
	} else
		fprintf(stderr, "vrpn_BaseConnection::handle_log_message:  "
				"Remote BaseConnection requested logging while logging.\n");


	// OR the remotely-requested logging mode with whatever we've
	// been told to do locally
	me->d_logmode |= p.service;
	
	return retval;
}

// XXX - requires functions only found in NetConnection
// Get the UDP port description from the other side and connect the
// outgoing UDP socket to that port so we can send lossy but time-
// critical (tracker) messages that way.

vrpn_int32 vrpn_BaseConnection::handle_UDP_message(
	void *userdata,
	vrpn_HANDLERPARAM p)
{
	char	rhostname[1000];	// name of remote host
	vrpn_BaseConnection * me = (vrpn_BaseConnection *)userdata;

#ifdef	VERBOSE
	printf("  Received request for UDP channel to %s\n", p.buffer);
#endif

	// Get the name of the remote host from the buffer (ensure terminated)
	strncpy(rhostname, p.buffer, sizeof(rhostname));
	rhostname[sizeof(rhostname)-1] = '\0';

	// Open the UDP outbound port and connect it to the port on the
	// remote machine.
	// (remember that the service field holds the UDP port number)
	me->udp_outbound = connect_udp_to(rhostname, (int)p.service);
	if (me->udp_outbound == -1) {
	    perror("vrpn: vrpn_BaseConnection: Couldn't open outbound UDP link");
	    return -1;
	}

	// Put this here because currently every connection opens a UDP
	// port and every host will get this data.  Previous implementation
	// did it in connect_tcp_to, which only gets called by servers.

	strncpy(me->rhostname, rhostname,
		sizeof(me->rhostname));

#ifdef	VERBOSE
	printf("  Opened UDP channel to %s:%d\n", rhostname, p.service);
#endif
	return 0;
}



vrpn_int32	vrpn_BaseConnection::handle_type_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_BaseConnection * me = (vrpn_BaseConnection*)userdata;
	cName	type_name;
	vrpn_int32	i;
	vrpn_int32	local_id;

	if (p.payload_len > sizeof(cName)) {
		fprintf(stderr,"vrpn: vrpn_BaseConnection::Type name too long\n");
		return -1;
	}

	// Find out the name of the type (skip the length)
	strncpy(type_name, p.buffer + sizeof(vrpn_int32),
                p.payload_len - sizeof(vrpn_int32));

	// Use the exact length packed into the start of the buffer
	// to figure out where to put the trailing '\0'
	i = ntohl(*((vrpn_int32 *) p.buffer));
	type_name[i] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side type: '%s'\n", type_name);
#endif
	// If there is a corresponding local type defined, find the mapping.
	// If not, clear the mapping.
	// Tell that the other side cares about it if found.
	local_id = -1; // None yet
	/*
	for (i = 0; i < me->num_my_types; i++) {
		if (strcmp(type_name, me->my_types[i].name) == 0) {
			local_id = i;
			me->my_types[i].cCares = 1;  // TCH 28 Oct 97
			break;
		}
	}
	*/
	// XXX - need this function in BaseConnection
	local_id = get_message_type_id(type_name);
	if (me->register_remote_type(type_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote type %s\n", type_name);
		return -1;
	}

	return 0;
}

vrpn_int32	vrpn_BaseConnection::handle_service_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_BaseConnection * me = (vrpn_BaseConnection*)userdata;
	cName	service_name;
	vrpn_int32	i;
	vrpn_int32	local_id;

	if (p.payload_len > sizeof(cName)) {
	        fprintf(stderr,"vrpn: vrpn_BaseConnection::Service name too long\n");
		return -1;
	}

	// Find out the name of the service (skip the length)
	strncpy(service_name, p.buffer + sizeof(vrpn_int32),
		p.payload_len - sizeof(vrpn_int32));

	// Use the exact length packed into the start of the buffer
	// to figure out where to put the trailing '\0'
	i = ntohl(*((vrpn_int32 *) p.buffer));
	service_name[i] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side service: '%s'\n", service_name);
#endif
	// If there is a corresponding local service defined, find the mapping.
	// If not, clear the mapping.
	local_id = -1; // None yet
	/*
	for (i = 0; i < me->num_my_services; i++) {
		if (strcmp(service_name, me->my_services[i]) == 0) {
			local_id = i;
			break;
		}
	}
	*/
	// XXX - need this function in BaseConnection
	local_id = me->get_service_id(service_name);
	if (me->register_remote_service(service_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote service %s\n", service_name);
		return -1;
	}

	return 0;
}
