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
// vrpn_BaseConnection: public: type and service id functions
//
//==========================================================================
//==========================================================================

// * register a new local {type,service} that that manager
//   has assigned a {type,service}_id to.
// * in addition, look to see if this {type,service} has
//   already been registered remotely (newRemoteType/Service)
// * if so, record the correspondence so that
//   local_{type,service}_id() can do its thing
// * XXX proposed new name:
//         register_local_{type,service}
//
//Return 1 if this {type,service} was already registered
//by the other side, 0 if not.

// return a 1 if the service was already registered remotely, else
// return a 0
vrpn_int32 vrpn_BaseConnection::register_local_service(
    const char* service_name,
    vrpn_int32 local_id)
{
    // iterate through the registered_remote_services array looking
    // for emtries w/ a local id value of -1. when you find one, then
    // you do a strcmp to see if it is the same service you are
    // registering. if so, fill in the local_id value. if you don't
    // finad a matching remote service, then do nothing.

    if (local_id >= vrpn_CONNECTION_MAX_SERVICES) {
		fprintf(stderr,"vrpn_BaseConnection::register_local_service:"
                " Too many services locally (%d)\n",
                local_id);
		return -1;
	}

    for( int i=0; i < num_registered_remote_services; i++ ){
        // service not regiatered locally yet
        if( registered_remote_services[i].local_id == -1 ){
            // is it the one we're registering now?
            if( strcmp(registered_remote_services[i].name,service_name) == 0 ){
                registered_remote_services[i].local_id = local_id;
                return 1;
            }
        }
    }

    return 0;
}


// return a 1 if the type was already registered remotely, else
// return a 0
vrpn_int32 vrpn_BaseConnection::register_local_type(
    const char* type_name,
    vrpn_int32 local_id)
{
    // iterate through the registered_remote_types array looking
    // for emtries w/ a local id value of -1. when you find one, then
    // you do a strcmp to see if it is the same type you are
    // registering. if so, fill in the local_id value. if you don't
    // finad a matching remote type, then do nothing.

    if (local_id >= vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,"vrpn_BaseConnection::register_local_type:"
                " Too many types locally (%d)\n",
                local_id);
		return -1;
	}

    for( int i=0; i < num_registered_remote_types; i++ ){
        // type not regiatered locally yet
        if( registered_remote_types[i].local_id == -1 ){
            // is it the one we're registering now?
            if( strcmp(registered_remote_types[i].name,type_name) == 0 ){
                registered_remote_types[i].local_id = local_id;
                return 1;
            }
        }
    }

    return 0;

}

// Adds a new remote type/service and returns its index.
// Returns -1 on error.
// * called by the ConnectionManager when the peer on the
//   other side of this connection has sent notification
//   that it has registered a new type/service
// * don't call this function if the type/service has
//   already been locally registered
// * XXX proposed new name:
//         register_remote_{type,service}
    
// Adds a new remote type/service and returns its index
// was: newRemoteSender
vrpn_int32 vrpn_BaseConnection::register_remote_service(
    const cName service_name,  // e.g. "tracker0"
    vrpn_int32 remote_id )    // from manager
{
	if (num_registered_remote_services >= vrpn_CONNECTION_MAX_SERVICES) {
		fprintf(stderr,"vrpn: Too many services from other side (%d)\n",
                        num_registered_remote_services);
		return -1;
	}


    // sanity check: does remote id match next available space in
    // array?
    num_registered_remote_services++;
    if( remote_id != num_registered_remote_services ){
        fprintf(stderr,"vrpn_BaseConnection::register_remote_service:"
                ": remote_id does not match next available spot in "
                " array\n");
		return -1;
    }


    // should this be const_iterator?
    char** service_itr = d_manager_token->services_begin();
    char** const service_end = d_manager_token->services_end();
    for (int i=0;  service_itr != service_end;  ++service_itr, ++i) {
        
        if( *service_itr ){ // space for service has been allocated
            if( strcmp(*service_itr, service_name) == 0 ){
                // we have a match
                registered_remote_services[remote_id].name = new cName;
                if( !registered_remote_services[remote_id].name ){
                    fprintf(stderr,"vrpn_BaseConnection::register_remote_service: "
                            "Can't allocate memory for new record\n");
                    return -1;
                }
                strncpy(registered_remote_services[remote_id].name, 
                        service_name, sizeof(cName));
                registered_remote_services[remote_id].local_id = i;
                return remote_id;
            }
        }  
    }
    

    // service has not been registered locally
    // make sure entry is blank
    if (registered_remote_services[remote_id].name) {
        delete registered_remote_services[remote_id].name;
        registered_remote_services[remote_id].name = NULL;
    }
    registered_remote_services[remote_id].local_id = -1;

    return remote_id;

}

// Adds a new remote type/service and returns its index
// was: newRemoteType
vrpn_int32 vrpn_BaseConnection::register_remote_type(
    const cName type_name,    // e.g. "tracker_pos"
    vrpn_int32 remote_id )     // from manager
{
	if (num_registered_remote_types >= vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,"vrpn: Too many types from other side (%d)\n",
                        num_registered_remote_types);
		return -1;
	}


    // sanity check: does remote id match next available space in
    // array?
    num_registered_remote_types++;
    if( remote_id != num_registered_remote_types ){
        fprintf(stderr,"vrpn_BaseConnection::register_remote_type:"
                ": remote_id does not match next available spot in "
                " array\n");
		return -1;
    }


    typedef vrpn_BaseConnectionManager::vrpnLocalMapping vrpnLocalMapping;
    const vrpnLocalMapping*       type_itr = d_manager_token->types_begin();
    const vrpnLocalMapping* const type_end = d_manager_token->types_end();
    for (int j=0;  type_itr != type_end;  ++type_itr, ++j ) {

        if( type_itr ){ // space for type has been allocated
            if( strcmp(type_itr->name, type_name) == 0 ){
                // we have a match
                registered_remote_types[remote_id].name = new cName;
                if( !registered_remote_types[remote_id].name ){
                    fprintf(stderr,"vrpn_BaseConnection::register_remote_type: "
                            "Can't allocate memory for new record\n");
                    return -1;
                }
                strncpy(registered_remote_types[remote_id].name, 
                        type_name, sizeof(cName));
                registered_remote_types[remote_id].local_id = j;
                return remote_id;
            }
        }  
    }

    // type has not been registered locally
    // make sure entry is blank
    if (registered_remote_types[remote_id].name) {
        delete registered_remote_types[remote_id].name;
        registered_remote_types[remote_id].name = NULL;
    }
    registered_remote_types[remote_id].local_id = -1;

    return remote_id;

}



// Give the local mapping for the remote type or service.
// Returns -1 if there isn't one.
// Pre: have already registered this type/service remotely
//      and locally using register_local_{type,service}
//      and register_remote_{type_service}
// * XXX proposed new name:
//         translate_remote_{type,service}_to_local


// Give the local mapping for the remote type
// was: local_type_id
vrpn_int32 vrpn_BaseConnection::translate_remote_type_to_local( 
    vrpn_int32 remote_type ) 
{
    if (remote_type < num_registered_remote_types) {
        return registered_remote_types[remote_type].local_id;
    } else {
        return -1;
    }
}

// Give the local mapping for the remote service
// was: local_sender_id
vrpn_int32 vrpn_BaseConnection::translate_remote_service_to_local( 
    vrpn_int32 remote_service )
{
    if (remote_service < num_registered_remote_services) {
        return registered_remote_services[remote_service].local_id;
    } else {
        return -1;
    }
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
	vrpn_int32	remote_id;

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
	remote_id = p.type;
	if (me->register_remote_type(type_name, remote_id) == -1) {
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
	vrpn_int32	remote_id;

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
	remote_id = p.service;
	if (me->register_remote_service(service_name, remote_id) == -1) {
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











