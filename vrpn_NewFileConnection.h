#ifndef VRPN_FILE_CONNECTION_H
#define VRPN_FILE_CONNECTION_H

#include "vrpn_BaseConnection.h"

// vrpn_FileConnection
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
// vrpn_File_Controller and pass your vrpn_FileConnection to its constructor.

// Logfiles are recorded as *sent*, not as translated by the receiver,
// so we still need to have all the correct names for senders and types
// registered.

// September 1998:  by default preloads the entire log file on startup.
// This causes a delay (nontrivial for large logs) but should help smooth
// playback.  The infrastructure is also in place for playing backwards,
// but we still have the problem that the semantics of reverse often don't
// make sense.

class vrpn_FileConnection 
	: public vrpn_BaseConnection, 
	  protected vrpn_FileConnection_ControllerInterface 
{

	friend class FileController;

public: // c'tors & d'tors

    vrpn_FileConnection (const char * file_name);
    virtual ~vrpn_FileConnection (void);

    virtual vrpn_int32 mainloop (const struct timeval * timeout = NULL);

    virtual vrpn_int32 time_since_connection_open (struct timeval * elapsed_time);

    virtual vrpn_FileConnection *get_FileConnection() { return this; }

    // XXX the following should not be public if we want vrpn_FileConnection
    //     to have the same interface as vrpn_Connection
    //
    //     If so handler functions for messages for these operations 
    //     should be made, and functions added to vrpn_File_Controller which
    //     generate the messages.  This seemed like it would be messy
    //     since most of these functions have return values


public: // playback functions

	// rate of 0.0 is paused, 1.0 is normal speed
    void set_replay_rate(vrpn_float32 rate);

	// resets to the beginning of the file
    vrpn_int32 reset (void);      

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


protected: // playback functions

    void play_to_user_message();

	// helper function for mainloop()
    vrpn_int32 need_to_play(timeval filetime);

	// checks the cookie at
	// the head of the log file;
	//  exit on error!
    virtual vrpn_int32 read_cookie (void); 
    
	// appends entry to d_logTail
	virtual vrpn_int32 read_entry (void);  

	// returns 0 on success, 1 on EOF, -1 on error
    virtual vrpn_int32 close_file (void);

    // handlers for VRPN control messages
    static vrpn_int32 handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_reset (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_play_to_time (void *, vrpn_HANDLERPARAM);
	
protected: // data members
	
    // tokens for VRPN control messages

    vrpn_int32 d_controllerId;

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
};


#endif  // VRPN_FILE_CONNECTION_H
