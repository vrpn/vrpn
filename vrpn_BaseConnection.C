#include "vrpn_BaseConnection.h"



vrpn_BaseConnection::vrpn_BaseConnection(
    // takes ownership of rat, and deletes in destructor
    vrpn_BaseConnectionManager::RestrictedAccessToken * rat,
    
    const char *  local_logfile_name,
    vrpn_int32    local_log_mode,
    
    const char *  remote_logfile_name,
    vrpn_int32    remote_log_mode
    )
    
    : d_manager_token             (rat),
      d_local_logmode                (local_log_mode),
      d_remote_logmode               (remote_log_mode),
      num_registered_remote_services (0),
      num_registered_remote_types    (0)
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
        handle_incoming_service_message;

    system_messages[-vrpn_CONNECTION_TYPE_DESCRIPTION] =
        handle_incoming_type_message;

    system_messages[-vrpn_CONNECTION_LOG_DESCRIPTION] =
        handle_incoming_log_message;
    
    
    // initialize logging data member values
    if (local_logfile_name) {
        d_local_logname = new char [1 + strlen(local_logfile_name)];
        if (!d_local_logname) {
            fprintf(stderr, "vrpn_BaseConnection::vrpn_Connection:  "
                    "Out of memory!\n");
            return;
        }
        strcpy(d_local_logname, local_logfile_name);
    }
    
    if (remote_logfile_name) {
        d_remote_logname = new char [1 + strlen(remote_logfile_name)];
        if (!d_remote_logname) {
            fprintf(stderr, "vrpn_BaseConnection::vrpn_Connection:  "
                    "Out of memory!\n");
            return;
        }
        strcpy(d_remote_logname, remote_logfile_name);
    }
    
}

vrpn_BaseConnection::~vrpn_BaseConnection()
{
    // close logging and delete dynamically allocated memory
    if (d_local_logname)
        close_log();
    if(d_local_logname)
        delete d_local_logname;
    if(d_remote_logname)
        delete d_remote_logname;
    delete d_manager_token;
}

//==========================================================================
//==========================================================================
//
// vrpn_BaseConnection: protected: system message handlers
//
//==========================================================================
//==========================================================================

//static
vrpn_int32 vrpn_BaseConnection::handle_incoming_log_message (
	void * userdata,
	vrpn_HANDLERPARAM p) 
{
	vrpn_BaseConnection * me = (vrpn_BaseConnection *) userdata;
	vrpn_int32 retval = 0;

	// if we're already logging, ignore the name (?)
	if (!me->d_local_logname) {
		me->d_local_logname = new char [p.payload_len];
		if (!me->d_local_logname) {
			fprintf(stderr, "vrpn_BaseConnection::handle_incoming_log_message:  "
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
		fprintf(stderr, "vrpn_BaseConnection::handle_incoming_log_message:  "
				"Remote BaseConnection requested logging while logging.\n");


	// OR the remotely-requested logging mode with whatever we've
	// been told to do locally
	me->d_local_logmode |= p.service;
	
	return retval;
}


vrpn_int32	vrpn_BaseConnection::handle_incoming_type_message(void *userdata,
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
	if (me->register_remote_type(type_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote type %s\n", type_name);
		return -1;
	}

	return 0;
}

vrpn_int32	vrpn_BaseConnection::handle_incoming_service_message(void *userdata,
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
	if (me->register_remote_service(service_name, local_id) == -1) {
		fprintf(stderr, "vrpn: Failed to add remote service %s\n", service_name);
		return -1;
	}

	return 0;
}


//==========================================================================
//==========================================================================
//
// vrpn_BaseConnection: public: logging functions
//
//==========================================================================
//==========================================================================

vrpn_int32 vrpn_BaseConnection::register_log_filter (
    vrpn_LOGFILTER filter, 
    void * userdata)
{
    return d_logger_ptr->register_log_filter (filter,userdata);
}

//==========================================================================
//==========================================================================
//
// vrpn_BaseConnection: protected: logging functions
//
//==========================================================================
//==========================================================================

vrpn_int32 vrpn_BaseConnection::open_log()
{
    d_logger_ptr = NULL;
    // HACKISH - d_local_{logname,logmode} are protected members of
    // vrpn_BaseConnection
    d_logger_ptr = new vrpn_FileLogger(d_local_logname, d_local_logmode);
    if( !d_logger_ptr ){
        cout << endl << "vrpn_BaseConnection: error opening logfile" << endl;
        return -1;
    }
    return 0;
}

vrpn_int32 vrpn_BaseConnection::close_log()
{
    delete d_logger_ptr;
    return 0;
}











