#ifndef VRPN_FILE_CONNECTION_H
#define VRPN_FILE_CONNECTION_H

#include "vrpn_FileConnectionInterface.h"
#include "vrpn_BaseConnection.h"

// vrpn_NewFileConnection
//
// MODIFIED
// By: Stefan Sain, July 1999
//
// This class has been changed to inherit from BaseConnection in the
// new vrpn connection scheme. Log files are now written out by the
// vrpn_FileLogger class. 

// XXX - when its final, write something about inheritance of interface
//

// Tom Hudson, June 1998

// This class *reads* a file written out by vrpn_Connection's logging hooks.

// The interface exactly matches that of vrpn_Connection.  To do things that
// are meaningful on log replay but not on live networks, create a
// vrpn_File_Manager and pass your vrpn_NewFileConnection to its constructor.

// Logfiles are recorded as *sent*, not as translated by the receiver,
// so we still need to have all the correct names for senders and types
// registered.

// September 1998:  by default preloads the entire log file on startup.
// This causes a delay (nontrivial for large logs) but should help smooth
// playback.  The infrastructure is also in place for playing backwards,
// but we still have the problem that the semantics of reverse often don't
// make sense.

class vrpn_NewFileConnection 
    : public vrpn_BaseConnection,
      protected vrpn_FileConnectionInterface 
{
    friend class vrpn_NewFileManager;
    
    // {{{ c'tors, d'tors
public: // c'tors & d'tors
    
    vrpn_NewFileConnection(
        vrpn_BaseConnectionManager::RestrictedAccessToken *,
        const char *  file_name,
        const char *  local_logfile_name = NULL,
        vrpn_int32    local_log_mode     = vrpn_LOG_NONE);

    virtual ~vrpn_NewFileConnection ();

    virtual vrpn_int32 time_since_connection_open (struct timeval * elapsed_time);

    virtual vrpn_NewFileConnection *get_FileConnection() { return this; }
   
    // }}}
    // {{{ public type_id and service_id stuff
public:  

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

    // was: newLocalSender
    virtual vrpn_int32 register_local_service(
        const char *service_name,  // e.g. "tracker0"
        vrpn_int32 local_id );    // from manager
    
    // was: newLocalType
    virtual vrpn_int32 register_local_type(
        const char *type_name,   // e.g. "tracker_pos"
        vrpn_int32 local_id );   // from manager

    // }}}
    // {{{ status
public:  
    
    // a connection was made
    vrpn_bool connected() const
	{ return (status == vrpn_CONNECTION_CONNECTED); }

    // no errors
    vrpn_bool doing_okay() const { return (status >= 0); }

    // get status of connection
    vrpn_int32 get_status() const { return status; }
    
    // }}}
    // {{{ sending and receiving
public:  


    // Call each time through program main loop to handle receiving any
    // incoming messages and sending any packed messages.
    // Returns -1 when connection dropped due to error, 0 otherwise.
    // (only returns -1 once per connection drop).
    // Optional argument is TOTAL time to block on select() calls;
    // there may be multiple calls to select() per call to mainloop(),
    // and this timeout will be divided evenly between them.
    virtual vrpn_int32 mainloop (const struct timeval * timeout = NULL);

    // functions for sending messages and receiving messages
    // the ConnectionManager will call these functions

    vrpn_int32 queue_outgoing_message(
        vrpn_uint32  len, 
        timeval      time,
        vrpn_int32   type, 
        vrpn_int32   service, 
        const char * buffer,
        vrpn_uint32  class_of_service, 
        vrpn_bool    sent_mcast );


    vrpn_int32 handle_incoming_messages (const timeval * pTimeout = NULL);

    virtual vrpn_int32 handle_incoming_udp_message (void * userdata, 
                                                    vrpn_HANDLERPARAM p);

    // * send pending report (that have been packed), and clear the buffer
    // * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual vrpn_int32 send_pending_reports(){ return 0;}
    
    // }}}
    // {{{ playback functions - are public in base class
protected: 

	// rate of 0.0 is paused, 1.0 is normal speed
    void set_replay_rate(vrpn_float32 rate);

	// resets to the beginning of the file
    vrpn_int32 reset ();      

	// returns 1 if we're at the end of file
    vrpn_int32 eof();

	// end_time for play_to_time() is an elapsed time
	// returns -1 on error or EOF, 0 on success
    vrpn_int32 play_to_time (vrpn_float64 end_time);
    vrpn_int32 play_to_time (struct timeval end_time);
	
	// end_filetime is an absolute time, corresponding to the
	// timestamps of the entries in the file,
	// returns -1 on error or EOF, 0 on success
    vrpn_int32 play_to_filetime(timeval end_filetime);
	
      // plays the next entry, returns -1 or error or EOF, 0 otherwise
    vrpn_int32 playone();
	// plays at most one entry, but won't play past end_filetime
	// returns 0 on success, 1 if at end_filetime, -1 on error or EOF
    vrpn_int32 playone_to_filetime(timeval end_filetime);

	// returns the elapsed time of the file
    timeval get_length();
    vrpn_float64 get_length_secs();

	// returns the timestamp of the earliest in time user message
    timeval get_lowest_user_timestamp();
	
	// jump_to_time sets the current position to the given elapsed time
    vrpn_int32 jump_to_time(vrpn_float64 newtime);
    vrpn_int32 jump_to_time(timeval newtime);

    // }}}
    // {{{ playback functions
protected: 

    void play_to_user_message();

	// helper function for mainloop()
    vrpn_int32 need_to_play(timeval filetime);

	// checks the cookie at
	// the head of the log file;
	//  exit on error!
    virtual vrpn_int32 read_cookie (); 
    
	// appends entry to d_logTail
	virtual vrpn_int32 read_entry ();  

	// returns 0 on success, 1 on EOF, -1 on error
    virtual vrpn_int32 close_file ();

    // handlers for VRPN control messages
    static vrpn_int32 handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_reset           (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_play_to_time    (void *, vrpn_HANDLERPARAM);

    // }}}	
    // {{{ data members
protected: 

    vrpn_int32 status;
    
    // tokens for VRPN control messages

    vrpn_int32 d_managerId;

    vrpn_int32 d_set_replay_rate_type;
    vrpn_int32 d_reset_type;
    vrpn_int32 d_play_to_time_type;
    //long d_jump_to_time_type;

    // time-keeping

    struct timeval d_time;  // current time in file
    struct timeval d_start_time;  // time of first record in file
    struct timeval d_last_time;  // wallclock time of last call to mainloop

    vrpn_float32 d_rate;  // scale factor for wallclock time

    // the actual mechanics of the logfile

    FILE * d_file;

    // TCH 16 Sept 98
    // support for preloading

    vrpn_LOGLIST * d_logHead;  // where the log begins
    vrpn_LOGLIST * d_logTail;  // the most recently read-in record
    vrpn_LOGLIST * d_currentLogEntry;  // most recently replayed
    vrpn_LOGLIST * d_startEntry;  // potentially after initial system messages

    // }}}

#ifndef HAVE_DYNAMIC_CAST
public:
    // this is a temporary measure until you can assume
    // dynamic_cast in all compilers
    virtual vrpn_NewFileConnection* get_FileConnectionPtr()
    {
        // all other BaseConnection objects return NULL
        return this;
    }
#endif
};

#endif  // VRPN_FILE_CONNECTION_H
