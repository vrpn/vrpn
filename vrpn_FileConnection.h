#ifndef VRPN_FILE_CONNECTION_H
#define VRPN_FILE_CONNECTION_H

#include "vrpn_Connection.h"

// vrpn_File_Connection
//
// Tom Hudson, June 1998

// This class *reads* a file written out by vrpn_Connection's logging hooks.

// The interface exactly matches that of vrpn_Connection.  To do things that
// are meaningful on log replay but not on live networks, create a
// vrpn_File_Controller and pass your vrpn_File_Connection to its constructor.

// Logfiles are recorded as *sent*, not as translated by the receiver,
// so we still need to have all the correct names for senders and types
// registered.

// September 1998:  by default preloads the entire log file on startup.
// This causes a delay (nontrivial for large logs) but should help smooth
// playback.  The infrastructure is also in place for playing backwards,
// but we still have the problem that the semantics of reverse often don't
// make sense.

// Note: this is based on a vrpn_Connection, not a vrpn_Synchronized_Connection
// because the message logs are recorded with adjusted time stamps (and a 
// vrpn_Clock object can not correctly sync over a file connection).

class vrpn_File_Connection : public vrpn_Connection {

  public:

    vrpn_File_Connection (const char * file_name);
    virtual ~vrpn_File_Connection (void);

    virtual int mainloop (const struct timeval * timeout = NULL);

    virtual int time_since_connection_open (struct timeval * elapsed_time);

  protected:

    // tokens for VRPN control messages

    vrpn_int32 d_controllerId;

    vrpn_int32 d_set_replay_rate_type;
    vrpn_int32 d_reset_type;
    vrpn_int32 d_play_to_time_type;
    //long d_jump_to_time_type;

    // time-keeping

    struct timeval d_time;  // current time in file
    struct timeval d_start_time;  // time of first record in file
    struct timeval d_runtime;  // elapsed file time (d_time - d_start_time)

    struct timeval d_now_time;  // wallclock time of last call to mainloop
    struct timeval d_next_time;  // elapsed wallclock time

    vrpn_float32 d_rate;  // scale factor for wallclock time

    // the actual mechanics of the logfile

    FILE * d_file;

    virtual int read_entry (void);  // appends entry to d_logTail
      // returns 0 on success, 1 on EOF, -1 on error
    virtual int close_file (void);

    // why not expose these two?

    virtual int reset (void);
      // resets to the beginning of the file
    virtual int play_to_time (struct timeval newtime);
    //virtual int jump_to_time (struct timeval newtime);
      // resets to the given elapsed time

    // handlers for VRPN control messages

    static int handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static int handle_reset (void *, vrpn_HANDLERPARAM);
    static int handle_play_to_time (void *, vrpn_HANDLERPARAM);
    //static int handle_jump_to_time (void *, vrpn_HANDLERPARAM);

    // TCH 16 Sept 98
    // support for preloading

    vrpn_LOGLIST * d_logHead;  // where the log begins
    vrpn_LOGLIST * d_logTail;  // the most recently read-in record
    vrpn_LOGLIST * d_currentLogEntry;  // most recently replayed
};


#endif  // VRPN_FILE_CONNECTION_H



