#include "vrpn_BaseConnectionController.h"



//**************************************************************************
//**************************************************************************
// 
// {{{ vrpn_ConnectionController constructors and destructor
// 
//**************************************************************************
//**************************************************************************


vrpn_BaseConnectionController::vrpn_BaseConnectionController(
	char * local_logfile,
	vrpn_int32 local_logmode,
	char * remote_logfile,
	vrpn_int32 remote_logmode,
    vrpn_float64 dFreq, 
    vrpn_int32 cOffsetWindow
	):
	d_local_logmode(local_logmode),
	d_remote_logmode(remote_logmode)
{

    // init for clock server/client
    // need to init gettimeofday for the pc
    struct timeval tv;
    gettimeofday(&tv, NULL);

	vrpn_int32	i;

	// Lots of constants used to be set up here.  They were moved
	// into the constructors in 02.10;  this will create a slight
        // increase in maintenance burden keeping the constructors consistient.

	// offset of clocks on connected machines -- local - remote
	// (this should really not be here, it should be in adjusted time
	// connection, but this makes it a lot easier
	tvClockOffset.tv_sec=0;
	tvClockOffset.tv_usec=0;

	// Set up the default handlers for the different types of user
	// incoming messages.  Initially, they are all set to the
	// NULL handler, which tosses them out.
	// Note that system messages always have negative indices.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		my_types[i].who_cares = NULL;
		my_types[i].cCares = 0;
		my_types[i].name = NULL;
	}


	for (i = 0; i < vrpn_CONNECTION_MAX_SERVICES; i++) {
		my_services[i] = NULL;
	}

	gettimeofday(&start_time, NULL);

	// copy logfile names
	if( local_logfile ){
		d_local_logname = new char [1 + strlen(local_logfile)];
		if (!d_local_logname) {
			fprintf(stderr, "vrpn_BaseConnectionController::vrpn_BaseConnectionController:  "
					"Out of memory!\n");
			status = BROKEN;
			return;
		}
		strcpy(d_local_logname,local_logfile,strlen(local_logfile)+1);
	}
	if( remote_logfile ){
		d_remote_logname = new char [1 + strlen(remote_logfile)];
		if (!d_remote_logname) {
			fprintf(stderr, "vrpn_BaseConnectionController::vrpn_BaseConnectionController:  "
					"Out of memory!\n");
			status = BROKEN;
			return;
		}
		strcpy(d_remote_logname,remote_logfile,strlen(remote_logfile)+1);
	}

#ifdef _WIN32

  // Make sure sockets are set up
  // TCH 2 Nov 98 after Phil Winston

  WSADATA wsaData;
  int status;

  status = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (status) {
    fprintf(stderr, "vrpn_Connection::init():  "
                    "Failed to set up sockets.\n");
    fprintf(stderr, "WSAStartup failed with error code %d\n", status);
    exit(0);
  }

#endif  // _WIN32

}

vrpn_BaseConnectionController::~vrpn_BaseConnectionController()
{
    // delete stuff in services/types/callbacks lists
    vrpn_int32 i;
    for (i = 0; i < num_my_services; ++i) {
        delete my_services[i];
    }
    for (i = 0; i < num_my_types; ++i) {
        delete my_types[i].name;
        delete my_types[i].who_cares;
    }
    // XXX callbacks

    //...XXX...
}

// }}}
// 
//**************************************************************************
//**************************************************************************
// 
// {{{ vrpn_BaseConnectionController services and types

// 
//**************************************************************************
//**************************************************************************

vrpn_int32
vrpn_BaseConnectionController::register_service(
    const char * const name )
{
    vrpn_int32	i;

    //fprintf(stderr, "vrpn_Connection::register_service:  "
    //        "%d services;  new name \"%s\"\n", num_my_services, name);

    // See if the name is already in the list.  If so, return it.
    for (i = 0; i < num_my_services; i++) {
        if (strcmp(my_services[i], name) == 0) {
            // fprintf(stderr, "It's already there, #%d\n", i);
            return i;
        }
    }

    // See if there are too many on the list.  If so, return -1.
    if (num_my_services >= vrpn_CONNECTION_MAX_SERVICES) {
   	fprintf(stderr, "vrpn: vrpn_Connection::register_service:  "
                "Too many! (%d)\n", num_my_services);
   	return -1;
    }

    if (!my_services[num_my_services]) {
        //  fprintf(stderr, "Allocating a new name entry\n");
        my_services[num_my_services] = new cName;
        if (!my_services[num_my_services]) {
            fprintf(stderr, "vrpn_Connection::register_service:  "
                    "Can't allocate memory for new record\n");
            return -1;
        }
    }

    // Add this one into the list
    strncpy(my_services[num_my_services], name, sizeof(cName) - 1);
    num_my_services++;

#if 1
    register_service_with_connections( name, num_my_services-1 );
#else
    //XXXXXXXXXXXXXXXXXXXX
    //XXX  push this down into BaseConnection
    //XXXXXXXXXXXXXXXXXXXX

    // If we're connected, pack the service description
    if (connected()) {
   	pack_service_description(num_my_services - 1);
    }

    // If the other side has declared this service, establish the
    // mapping for it.
    endpoint.newLocalService(name, num_my_services - 1);
#endif

    // One more in place -- return its index
    return num_my_services - 1;
}


vrpn_int32
vrpn_BaseConnectionController::register_message_type(
    const char * const name )
{
    vrpn_int32	i;

    // See if the name is already in the list.  If so, return it.
    i = get_message_type_id(name);
    if (i != -1)
        return i;

    // See if there are too many on the list.  If so, return -1.
    if (num_my_types == vrpn_CONNECTION_MAX_TYPES) {
        fprintf(stderr,
                "vrpn: vrpn_Connection::register_message_type:  "
                "Too many! (%d)\n", num_my_types);
        return -1;
    }

    if (!my_types[num_my_types].name) {
        my_types[num_my_types].name = new cName;
        if (!my_types[num_my_types].name) {
            fprintf(stderr, "vrpn_Connection::register_message_type:  "
                    "Can't allocate memory for new record\n");
            return -1;
        }
    }

    // Add this one into the list and return its index
    strncpy(my_types[num_my_types].name, name, sizeof(cName) - 1);
    my_types[num_my_types].who_cares = NULL;
    my_types[num_my_types].cCares = 0;  // TCH 28 Oct 97 - redundant?
    num_my_types++;

#if 1
    register_type_with_connections( name, num_my_types-1 );
#else
    //XXXXXXXXXXXXXXXXXXXX
    //XXX  push this down into BaseConnection
    //XXXXXXXXXXXXXXXXXXXX

    // If we're connected, pack the type description
    if (connected()) {
        pack_type_description(num_my_types-1);
    }

    if (endpoint.newLocalType(name, num_my_types - 1)) {  //XXX-JJ
        my_types[num_my_types - 1].cCares = 1;  // TCH 28 Oct 97
    }
#endif

    // One more in place -- return its index
    return num_my_types - 1;
}


vrpn_int32
vrpn_BaseConnectionController::get_service_id(
    const char * const name ) const
{
    vrpn_int32	i;

    // See if the name is already in the list.  If so, return it.
    for (i = 0; i < num_my_services; i++)
        if (strcmp(my_services[i], name) == 0)
            return i;

    return -1;
}


vrpn_int32
vrpn_BaseConnectionController::get_message_type_id(
    const char * const name ) const
{
    vrpn_int32	i;

    // See if the name is already in the list.  If so, return it.
    for (i = 0; i < num_my_types; i++)
        if (strcmp(my_types[i].name, name) == 0)
            return i;

    return -1;
}


const char *
vrpn_BaseConnectionController::get_service_name(
    vrpn_int32 service_id ) const
{
    if ((service_id < 0) || (service_id >= num_my_services)) {
        return NULL;
    }
    return (const char *) my_services[service_id];
}


const char *
vrpn_BaseConnectionController::get_message_type_name(
    vrpn_int32 type_id ) const
{
    if ((type_id < 0) || (type_id >= num_my_types)) {
        return NULL;
    }
    return (const char *) my_types[type_id].name;
}

// }}}
// 
//**************************************************************************
//**************************************************************************
//
// {{{ vrpn_BaseConnectionController: public: logging get functions
//
//**************************************************************************
//**************************************************************************

vrpn_int32 vrpn_BaseConnectionController::get_local_logmode()
{
    return d_local_logmode;
}

// it is assumed the caller has provided ample storage for the filename
void vrpn_BaseConnectionController::get_local_logfile_name(char * logname_copy)
{
    strncpy( logname_copy,
             d_local_logname,
             strlen(d_local_logname)+1 );
}

vrpn_int32 vrpn_BaseConnectionController::get_remote_logmode()
{
    return d_remote_logmode;
}

// it is assumed the caller has provided ample storage for the filename
void vrpn_BaseConnectionController::get_remote_logfile_name(char * logname_copy)
{
    strncpy( logname_copy,
             d_remote_logname,
             strlen(d_remote_logname)+1 );
}


// }}}
