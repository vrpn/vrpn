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

class vrpn_File_Connection : public vrpn_Connection {

  public:

    vrpn_File_Connection (const char * file_name);
    virtual ~vrpn_File_Connection (void);

    virtual int mainloop (const struct timeval * timeout = NULL);

    virtual int time_since_connection_open (struct timeval * elapsed_time);

    // XXX the following should not be public if we want vrpn_File_Connection
    //     to have the same interface as vrpn_Connection
    //
    //     If so handler functions for messages for these operations 
    //     should be made, and functions added to vrpn_File_Controller which
    //     generate the messages.  This seemed like it would be messy
    //     since most of these functions have return values

      // rate of 0.0 is paused, 1.0 is normal speed
    void set_replay_rate(vrpn_float32 rate);

      // resets to the beginning of the file
    int reset (void);      

      // returns 1 if we're at the end of file
    int eof();

      // end_time for play_to_time() is an elapsed time
      // returns -1 on error or EOF, 0 on success
    int play_to_time (vrpn_float64 end_time);
    int play_to_time (struct timeval end_time);

      // end_filetime is an absolute time, corresponding to the
      // timestamps of the entries in the file,
      // returns -1 on error or EOF, 0 on success
    int play_to_filetime(timeval end_filetime);

      // plays the next entry, returns -1 or error or EOF, 0 otherwise
    int playone();
      // plays at most one entry, but won't play past end_filetime
      // returns 0 on success, 1 if at end_filetime, -1 on error or EOF
    int playone_to_filetime(timeval end_filetime);

      // returns the elapsed time of the file
    timeval get_length();
    double get_length_secs();

      // returns the timestamp of the earliest in time user message
    timeval get_lowest_user_timestamp();

      // jump_to_time sets the current position to the given elapsed time
    int jump_to_time(vrpn_float64 newtime);
    int jump_to_time(timeval newtime);

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
    struct timeval d_last_time;  // wallclock time of last call to mainloop

    vrpn_float32 d_rate;  // scale factor for wallclock time

    // the actual mechanics of the logfile

    FILE * d_file;

    void play_to_user_message();
      // helper function for mainloop()
    int need_to_play(timeval filetime);

    virtual int read_entry (void);  // appends entry to d_logTail
      // returns 0 on success, 1 on EOF, -1 on error
    virtual int close_file (void);

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
    vrpn_LOGLIST * d_startEntry;  // potentially after initial system messages
};


#endif  // VRPN_FILE_CONNECTION_H
