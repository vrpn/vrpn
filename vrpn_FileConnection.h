#ifndef VRPN_FILE_CONNECTION_H
#define VRPN_FILE_CONNECTION_H

// {{{ vrpn_File_Connection
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
// }}}

#include "vrpn_Connection.h"

class vrpn_File_Connection : public vrpn_Connection
{
public:
    vrpn_File_Connection (const char * file_name, 
                         const char * local_in_logfile_name = NULL,
                         const char * local_out_logfile_name = NULL);
    virtual ~vrpn_File_Connection (void);
    
    virtual int mainloop (const timeval * timeout = NULL);
    
    virtual int time_since_connection_open (timeval * elapsed_time);

    virtual vrpn_File_Connection * get_File_Connection (void);

    // Pretend to send pending report, really just clear the buffer.
    virtual int     send_pending_reports (void);

    // {{{ fileconnections-specific methods (playback control)
public:
    // XXX the following should not be public if we want vrpn_File_Connection
    //     to have the same interface as vrpn_Connection
    //
    //     If so handler functions for messages for these operations 
    //     should be made, and functions added to vrpn_File_Controller which
    //     generate the messages.  This seemed like it would be messy
    //     since most of these functions have return values

    // rate of 0.0 is paused, 1.0 is normal speed
    void set_replay_rate(vrpn_float32 rate) {
        d_filetime_accum.set_replay_rate( rate );
    }
    
    // resets to the beginning of the file
    int reset (void);      

    // returns 1 if we're at the end of file
    int eof();

    // end_time for play_to_time() is an elapsed time
    // returns -1 on error or EOF, 0 on success
    int play_to_time (vrpn_float64 end_time);
    int play_to_time (timeval end_time);

    // end_filetime is an absolute time, corresponding to the
    // timestamps of the entries in the file,
    // returns -1 on error or EOF, 0 on success
    int play_to_filetime(const timeval end_filetime);

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

    // returns the name of the file
    const char *get_filename();

    // jump_to_time sets the current position to the given elapsed time
    int jump_to_time(vrpn_float64 newtime);
    int jump_to_time(timeval newtime);

    // Not very useful.
    // Limits the number of messages played out on any one call to mainloop.
    // 0 => no limit.
    void limit_messages_played_back (vrpn_int32 max_playback);

    // }}}
    // {{{ tokens for VRPN control messages (data members)
protected:
    vrpn_int32 d_controllerId;

    vrpn_int32 d_set_replay_rate_type;
    vrpn_int32 d_reset_type;
    vrpn_int32 d_play_to_time_type;
    //long d_jump_to_time_type;

    // }}}
    // {{{ time-keeping
protected:
    timeval d_time;  // current time in file
    timeval d_start_time;  // time of first record in file

    // wallclock time at the (beginning of the) last call
    // to mainloop that played back an event
    timeval d_last_time;  // XXX remove

    class FileTime_Accumulator
    {
        // accumulates the amount of time that we will advance
        // filetime by when we next play back messages.
        timeval d_filetime_accum_since_last_playback;  
        
        // wallclock time when d_filetime_accum_since_last_playback
        // was last updated
        timeval d_time_of_last_accum;
        
        // scale factor between stream time and wallclock time
        vrpn_float32 d_replay_rate;

    public:
        FileTime_Accumulator();
        
        // return accumulated time since last reset
        const timeval & accumulated (void) {
            return d_filetime_accum_since_last_playback;
        }

        // return last time accumulate_to was called
        const timeval & time_of_last_accum (void) {
            return d_time_of_last_accum;
        }

        vrpn_float32 replay_rate (void) { return d_replay_rate; }
        
        // add (d_replay_rate * (now_time - d_time_of_last_accum))
        // to d_filetime_accum_since_last_playback
        // then set d_time_of_last_accum to now_time
        void accumulate_to (const timeval & now_time);

        // if current rate is non-zero, then time is accumulated
        // before d_replay_rate is set to new_rate
        void set_replay_rate (vrpn_float32 new_rate);
        
        // set d_time_of_last_accum to now_time
        // and set d_filetime_accum_since_last_playback to zero
        void reset_at_time (const timeval & now_time);
    };
    FileTime_Accumulator d_filetime_accum;
    
    // }}}
    // {{{ actual mechanics of the logfile
protected:
    char *d_fileName;
    FILE * d_file;

    void play_to_user_message();

    // helper function for mainloop()
    int need_to_play(timeval filetime);

    // checks the cookie at
    // the head of the log file;
    //  exit on error!
    virtual int read_cookie (void);  

    virtual int read_entry (void);  // appends entry to d_logTail
      // returns 0 on success, 1 on EOF, -1 on error
    virtual int close_file (void);

    // }}}
    // {{{ handlers for VRPN control messages
protected:
    static int handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static int handle_reset (void *, vrpn_HANDLERPARAM);
    static int handle_play_to_time (void *, vrpn_HANDLERPARAM);
    //static int handle_jump_to_time (void *, vrpn_HANDLERPARAM);

    // }}}
    // {{{ [TCH 16 Sept 98] support for preloading
protected:
    vrpn_LOGLIST * d_logHead;  // where the log begins
    vrpn_LOGLIST * d_logTail;  // the most recently read-in record
    vrpn_LOGLIST * d_currentLogEntry;  // most recently replayed
    vrpn_LOGLIST * d_startEntry;  // potentially after initial system messages
    // }}}

protected:
    vrpn_int32 d_max_message_playback;
};


#endif  // VRPN_FILE_CONNECTION_H
