#include "vrpn_NewFileConnection.h"

#define CHECK(x) if (x == -1) return -1

// Since the PRELOAD feature was added, I suspect it would
// take some work to revert to the non-preload mode.  That is,
// both modes were not tested as features were added.  It shouldn't
// be hard to go back, but... -PBW 3/99


//==========================================================================
//==========================================================================
//
// vrpn_NewFileConnection: public: c'tors and d'tors
//
//==========================================================================
//==========================================================================


vrpn_NewFileConnection::vrpn_NewFileConnection (
    vrpn_BaseConnectionController::RestrictedAccessToken * rat,
    const char * file_name,
    const char * local_logfile_name,
    vrpn_int32   local_log_mode)
    : vrpn_BaseConnection (rat, local_logfile_name, local_log_mode),
      d_rate (1.0f),
      d_file (NULL),
      d_logHead (NULL),
      d_logTail (NULL),
      d_currentLogEntry (NULL)
{
    const char * bare_file_name;
    
    // necessary to initialize properly in mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;
    
    bare_file_name = vrpn_copy_file_name(file_name);
    if (!bare_file_name) {
        fprintf(stderr, "vrpn_NewFileConnection:  Out of memory!\n");
        status = vrpn_CONNECTION_BROKEN;
        return;
    }
    
    d_file = fopen(bare_file_name, "rb");
    if (!d_file) {
        fprintf(stderr, "vrpn_NewFileConnection:  "
                "Could not open file \"%s\".\n", bare_file_name);
        delete [] (char *) bare_file_name;
        status = vrpn_CONNECTION_BROKEN;
        return;
    } else {
        status = vrpn_CONNECTION_CONNECTED;
    }
    
    delete [] (char *) bare_file_name;
    
    // PRELOAD
    // TCH 16 Sept 1998
    
    //fprintf(stderr, "vrpn_NewFileConnection::vrpn_NewFileConnection: Preload...\n");
    
    if (read_cookie() < 0) {
        status = vrpn_CONNECTION_BROKEN;
        return;
    }
    while (!read_entry());
    d_currentLogEntry = d_logHead;
    
    //fprintf(stderr, "vrpn_NewFileConnection::vrpn_NewFileConnection: Done preload.\n");

    d_startEntry = d_logHead;
    d_start_time = d_startEntry->data.msg_time;  
    d_time = d_start_time;
    
    // this needs to be a parameter if we want this to be optional
    vrpn_int32 fPlayToFirstUserMessage = 1;
    // This is useful to play the initial system messages
    //(the sender/type ones) automatically.  These might not be
    // time synched so if we don't play them automatically they
    // can mess up playback if their timestamps are later then
    // the first user message.
    if (fPlayToFirstUserMessage) {
        play_to_user_message();
        d_startEntry = d_currentLogEntry;
        if (d_startEntry) {
            d_start_time = d_startEntry->data.msg_time;
        }
    }
}


// virtual
vrpn_NewFileConnection::~vrpn_NewFileConnection () {
	vrpn_LOGLIST * np;
	
	close_file();

	while (d_logHead) {
		np = d_logHead->next;
		if (d_logHead->data.buffer)
			delete [] (char *) d_logHead->data.buffer;
		delete d_logHead;
		d_logHead = np;
	}
}

//==========================================================================
//==========================================================================
//
// vrpn_NewFileConnection: public: sending and receiving
//
//==========================================================================
//==========================================================================

// virtual
vrpn_int32 vrpn_NewFileConnection::mainloop (const struct timeval * timeout)
{
	vrpn_int32 retval = 0;

	switch( status ){
	case vrpn_CONNECTION_LISTEN:
		// have reached end of file
		// but reset() will change state to
		// vrpn_CONNECTION_CONNECTED
		break;
	case vrpn_CONNECTION_CONNECTED:
		retval = handle_incoming_messages(timeout);
		break;
	case vrpn_CONNECTION_BROKEN:
	default:
		// error - exit program?
		cerr << "vrpn_NewFileConnection: mainloop: status = " << status << endl;
		exit(1);
	}
	   
	return retval;
}


vrpn_int32 vrpn_NewFileConnection::queue_outgoing_message(
		vrpn_uint32 len, 
        struct timeval time,
        vrpn_int32 type, 
        vrpn_int32 service, 
        const char * buffer,
        vrpn_uint32 class_of_service, 
        vrpn_bool sent_mcast )
{

    // might want to log outgoing messages
    if (d_local_logmode & vrpn_LOG_OUTGOING) { // FileLogger object exists
        if (d_logger_ptr->log_message(len, time, type, service, buffer)){
            return -1;
        }
    }

    return 0;
}

vrpn_int32 vrpn_NewFileConnection::handle_incoming_messages( 
    const struct timeval * pTimeout)
{
    // timeout ignored for now, needs to be added

    timeval now_time;
    gettimeofday(&now_time, NULL);
	
    if ((d_last_time.tv_sec == 0) && (d_last_time.tv_usec == 0)) {
          // If first iteration, consider 0 time elapsed
        d_last_time = now_time;
        return 0;
    }

	// scale elapsed time by d_rate (rate of replay);
	// this gives us the time to advance (skip_time)
	// our clock to (next_time).
    timeval skip_time = vrpn_TimevalDiff(now_time, d_last_time);
    skip_time = vrpn_TimevalScale(skip_time, d_rate);
    timeval end_time = vrpn_TimevalSum(d_time, skip_time);
    //fprintf(stderr, "skip = %d %d\n", skip_time.tv_sec, skip_time.tv_usec);

      // Had to add need_to_play() because at fractional rates
      // (even just 1/10th) the d_time didn't accumulate properly
      // because tiny intervals after scaling were too small for
      // for a timeval to represent (1us minimum).
    if (need_to_play(end_time)) {
        d_last_time = now_time;
        return play_to_filetime(end_time);
    } else {
		// we don't set d_last_time so that we can more
		// accurately measure the (larger) interval next
		// time around
        return 0;
    }
}

//==========================================================================
//==========================================================================
//
// vrpn_NewFileConnection: public: type and service id functions
//
//==========================================================================
//==========================================================================

// was: newLocalSender
vrpn_int32 vrpn_NewFileConnection::register_local_service(
    const char *service_name,  // e.g. "tracker0"
    vrpn_int32 local_id )      // from controller
{
    vrpn_int32 service_id = 
        vrpn_BaseConnection::register_local_service(
            service_name,
            local_id);

    if( strcmp("vrpn File Controller",service_name) == 0 ){
        d_controllerId = service_id;
    }

    return service_id;
}

// was: newLocalType
vrpn_int32 vrpn_NewFileConnection::register_local_type(
    const char *type_name,   // e.g. "tracker_pos"
    vrpn_int32 local_id )    // from controller
{
    vrpn_int32 type_id = 
        vrpn_BaseConnection::register_local_type(
            type_name,
            local_id);


    // register handlers if these types are being registered
    if( strcmp("vrpn File set replay rate",type_name) == 0 ){
        d_controller_token->register_handler(
            type_id, handle_set_replay_rate,
            this, d_controllerId);
    } 
    else if( strcmp("vrpn File reset",type_name) == 0 ){
        d_controller_token->register_handler(
            type_id, handle_reset, 
            this, d_controllerId);
    } 
    else if( strcmp("vrpn File play to time",type_name) == 0 ){
        d_controller_token->register_handler(
            type_id, handle_play_to_time,
            this, d_controllerId);
    }

    return type_id;

}

//==========================================================================
//==========================================================================
//
// vrpn_NewFileConnection: protected: playback functions
//
//==========================================================================
//==========================================================================

// Advances through the file, calling callbacks, up until
// a user message (type >= 0) is encountered)
// NOTE: assumes pre-load (could be changed to not)
void vrpn_NewFileConnection::play_to_user_message()
{
    if (!d_currentLogEntry) {
        return;
    }
    
    while (d_currentLogEntry != NULL && d_currentLogEntry->data.type < 0) {

        vrpn_HANDLERPARAM &header = d_currentLogEntry->data;

		if (header.type != vrpn_CONNECTION_UDP_DESCRIPTION) {
			if (system_messages[-header.type](this, header)) {
				fprintf(stderr, "vrn_FileConnection::play_to_user_message: "
						"Nonzero system return.\n");
			}
		}
        d_currentLogEntry = d_currentLogEntry->next;
		
        if (d_currentLogEntry) {
			// we advance d_time one ahead because they're may be 
			// a large gap in the file (forward or backwards) between
			// the last system message and first user one
            d_time = d_currentLogEntry->data.msg_time;
        }        
    }
}


vrpn_int32 vrpn_NewFileConnection::jump_to_time(vrpn_float64 newtime)
{
    return jump_to_time(vrpn_MsecsTimeval(newtime * 1000));
}

// assumes preload(?)
vrpn_int32 vrpn_NewFileConnection::jump_to_time(timeval newtime)
{
    d_time = vrpn_TimevalSum(d_start_time, newtime);
	
    if (!d_currentLogEntry) {
        d_currentLogEntry = d_logTail;
    }

      // search backwards
    while (vrpn_TimevalGreater(d_currentLogEntry->data.msg_time, d_time)) {
        if (d_currentLogEntry->prev) {
            d_currentLogEntry = d_currentLogEntry->prev;
        } else {
            return 1;
        }
    }

	// or forwards, as needed
    while (vrpn_TimevalGreater(d_time, d_currentLogEntry->data.msg_time)) {
        if (d_currentLogEntry->next) {
            d_currentLogEntry = d_currentLogEntry->next;
        } else {
            return 1;
        }        
    }

    return 0;
}


  // checks if there is at least one log entry that occurs
  // between the current d_time and the given filetime
vrpn_int32 vrpn_NewFileConnection::need_to_play(timeval filetime)
{
   if (!d_currentLogEntry) {
        vrpn_int32 retval = read_entry();
        if (retval < 0)
            return -1;  // error reading from file
        if (retval > 0)
            return 0;  // end of file;  nothing to replay
        d_currentLogEntry = d_logTail;  // better be non-NULL
    } 

    vrpn_HANDLERPARAM & header = d_currentLogEntry->data;

    return vrpn_TimevalGreater(filetime, header.msg_time);
}

// plays to an elapsed end_time (in seconds)
vrpn_int32 vrpn_NewFileConnection::play_to_time(vrpn_float64 end_time)
{
    return play_to_time(vrpn_MsecsTimeval(end_time * 1000));
}

// plays to an elapsed end_time
vrpn_int32 vrpn_NewFileConnection::play_to_time(timeval end_time)
{
    return play_to_filetime(vrpn_TimevalSum(d_start_time, end_time));
}



  // plays all entries between d_time and end_filetime
  // returns -1 on error, 0 on success
vrpn_int32 vrpn_NewFileConnection::play_to_filetime(timeval end_filetime) {
    
    vrpn_int32 ret;
    
    while (!(ret = playone_to_filetime(end_filetime))) {
        /* empty loop */
    }
    
    if (ret == 1) {
        // playone_to_filetime finished or EOF no error for us
        // Set log position to the exact requested ending time,
        // don't leave it at the last log entry time
        d_time = end_filetime;
        ret = 0;
    }
    
    return ret;
}

  // returns 1 if we're at the EOF, -1 on error
vrpn_int32 vrpn_NewFileConnection::eof()
{
    if (d_currentLogEntry) {
        return 0;
    } 
      // read from disk if not in memory
    vrpn_int32 ret = read_entry();

    if (ret == 0) {
        d_currentLogEntry = d_logTail;  // better be non-NULL
    }
	
	if (ret == 1)
		status = vrpn_CONNECTION_LISTEN;
	
    return ret;
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
vrpn_int32 vrpn_NewFileConnection::playone() 
{
    static timeval tvMAX = { LONG_MAX, LONG_MAX };
	
    vrpn_int32 ret = playone_to_filetime(tvMAX);
    if (ret != 0) {
		// consider a 1 return from playone_to_filetime() to be
		// an error since it should never reach tvMAX
        return -1;
    } else {
        return 0;
    }
}

// plays at most one entry which comes before end_filetime
// returns
//   -1 on error (including EOF, call eof() to test)
//    0 for normal result (played one entry)
//    1 if we hit end_filetime
vrpn_int32 vrpn_NewFileConnection::playone_to_filetime(timeval end_filetime)
{
	// read from disk if not in memory
    if (!d_currentLogEntry) {
        vrpn_int32 retval = read_entry();
        if (retval != 0)
            return -1;  // error reading from file or EOF
        d_currentLogEntry = d_logTail;  // better be non-NULL
    }

    vrpn_HANDLERPARAM & header = d_currentLogEntry->data;

    if (vrpn_TimevalGreater(header.msg_time, end_filetime)) {
		// there are no entries to play after the current
		// but before end_filetime
	    return 1;
    }

	// advance current file position
    d_time = header.msg_time;

	// Handle this log entry
    if (header.type >= 0) {
        if (translate_remote_type_to_local(header.type) >= 0) {
            if (d_controller_token->do_callbacks_for(
                translate_remote_type_to_local(header.type),
                translate_remote_service_to_local(header.service),
                header.msg_time, header.payload_len,
                header.buffer)) {
                return -1;     
            }
        }
            
    } else {  // system handler            

	    if (header.type != vrpn_CONNECTION_UDP_DESCRIPTION) {
	        if (system_messages[-header.type](this, header)) {
		        fprintf(stderr, "vrpn_NewFileConnection::playone_to_filename:  "
						"Nonzero system return.\n");
		        return -1;
	        }
	    }
    }        
	
      // Advance to next entry
    if (d_currentLogEntry) {
        d_currentLogEntry = d_currentLogEntry->next;
    }

    return 0;
}


vrpn_float64 vrpn_NewFileConnection::get_length_secs()
{
    return vrpn_TimevalMsecs(get_length())/1000;
}


//virtual
timeval vrpn_NewFileConnection::get_length()
{
    timeval len = {0, 0};

    if (d_logHead && d_logTail) {
        len = vrpn_TimevalDiff(d_logTail->data.msg_time,
                               d_start_time);
    }

    return len;
}


timeval vrpn_NewFileConnection::get_lowest_user_timestamp()
{
    timeval low = {LONG_MAX, LONG_MAX};

	// The first timestamp in the file may not be the lowest
	// because of time synchronization.
    if (d_logHead) {        
        vrpn_LOGLIST *pEntry;
        for (pEntry = d_logHead; pEntry != NULL; pEntry = pEntry->next) {            
            if (pEntry->data.type >= 0 &&
                vrpn_TimevalGreater(low, pEntry->data.msg_time)) {

                low = pEntry->data.msg_time;
            }
        }
    }

    return low;
}


// Returns the time since the connection opened.
// Some subclasses may redefine time.

// virtual
vrpn_int32 vrpn_NewFileConnection::time_since_connection_open
                                (timeval * elapsed_time) 
{
	*elapsed_time = vrpn_TimevalDiff(d_time, d_start_time);
	return 0;
}

// Reads a cookie from the logfile and calls check_vrpn_cookie()
// (from vrpn_Connection.C) to check it.
// Returns -1 on no cookie or cookie mismatch (which should cause abort),
// 0 otherwise.

// virtual
vrpn_int32 vrpn_NewFileConnection::read_cookie () 
{
	char readbuf [501];  // HACK!
	vrpn_int32 retval;
  
	retval = fread(readbuf, vrpn_cookie_size(), 1, d_file);
	if (retval <= 0) {
		fprintf(stderr, "vrpn_NewFileConnection::read_cookie:  "
				"No cookie.  If you're sure this is a logfile, "
				"run add_vrpn_cookie on it and try again.\n");
		return -1;
	}

	retval = check_vrpn_cookie(readbuf);
	if (retval < 0)
		return -1;

	return 0;
}

// virtual
vrpn_int32 vrpn_NewFileConnection::read_entry () 
{
	vrpn_LOGLIST * newEntry;
	vrpn_int32 retval;

	newEntry = new vrpn_LOGLIST;
	if (!newEntry) {
		fprintf(stderr, "vrpn_NewFileConnection::read_entry: Out of memory.\n");
		return -1;
	}

	if (!d_file) {
		fprintf(stderr, "vrpn_NewFileConnection::read_entry: no open file\n");
		return -1;
	}

	// get the header of the next message
	
	vrpn_HANDLERPARAM & header = newEntry->data;
	retval = fread(&header, sizeof(header), 1, d_file);

	// return 1 if nothing to read OR end-of-file;
	// the latter isn't an error state
	if (retval <= 0) {
		// Don't close the file because we might get a reset message...
		// close_file();
		return 1;
	}

	header.type = ntohl(header.type);
	header.service = ntohl(header.service);
	header.msg_time.tv_sec = ntohl(header.msg_time.tv_sec);
	header.msg_time.tv_usec = ntohl(header.msg_time.tv_usec);
	header.payload_len = ntohl(header.payload_len);

	// get the body of the next message
	
	header.buffer = new char [header.payload_len];
	if (!header.buffer) {
		fprintf(stderr, "vrpn_NewFileConnection::read_entry:  "
				"Out of memory.\n");
		return -1;
	}

	retval = fread((char *) header.buffer, 1, header.payload_len, d_file);
	
	// return 1 if nothing to read OR end-of-file;
	// the latter isn't an error state
	if (retval <= 0) {
		// Don't close the file because we might get a reset message...
		//close_file();
		return 1;
	}

	// doubly-linked list maintenance
	
	newEntry->next = NULL;
	newEntry->prev = d_logTail;
	if (d_logTail)
		d_logTail->next = newEntry;
	d_logTail = newEntry;
	if (!d_logHead)
		d_logHead = d_logTail;

	return 0;
}

// virtual
vrpn_int32 vrpn_NewFileConnection::close_file () {

	if (d_file)
		fclose(d_file);

	d_file = NULL;

	return 0;
}


vrpn_int32 vrpn_NewFileConnection::reset()
{
    d_currentLogEntry = d_startEntry;
    d_time = d_startEntry->data.msg_time;
	// reset for mainloop()
    d_last_time.tv_usec = d_last_time.tv_sec = 0;

	// reset status to vrpn_CONNECTION_CONNECTED
	// could have been vrpn_CONNECTION_LISTEN if eof
	// was reached
	status = vrpn_CONNECTION_CONNECTED;
    return 0;
}


void vrpn_NewFileConnection::set_replay_rate(vrpn_float32 rate)
{
    d_rate = rate;
}



//==========================================================================
//==========================================================================
//
// vrpn_NewFileConnection: protected: handlers
//
//==========================================================================
//==========================================================================

// static
vrpn_int32 
vrpn_NewFileConnection::handle_set_replay_rate(void * userdata, 
											vrpn_HANDLERPARAM p) 
{
	vrpn_NewFileConnection * me = (vrpn_NewFileConnection *) userdata;
	
	vrpn_int32 value = ntohl(*(vrpn_int32 *) (p.buffer));
	me->set_replay_rate(*((vrpn_float32 *) &value));

	return 0;
}

// static
vrpn_int32 
vrpn_NewFileConnection::handle_reset(void * userdata, 
								  vrpn_HANDLERPARAM) 
{
	vrpn_NewFileConnection * me = (vrpn_NewFileConnection *) userdata;
	fprintf(stderr, "In vrpn_NewFileConnection::handle_reset().\n");
	return me->reset();
}

// static
vrpn_int32 
vrpn_NewFileConnection::handle_play_to_time(void * userdata,
                                            vrpn_HANDLERPARAM p) 
{
	vrpn_NewFileConnection * me = (vrpn_NewFileConnection *) userdata;
	timeval newtime;

	newtime.tv_sec = ((vrpn_int32 *) (p.buffer))[0];
	newtime.tv_usec = ((vrpn_int32 *) (p.buffer))[1];

	return me->play_to_time(newtime);
}
