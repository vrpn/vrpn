#ifndef VRPN_FILE_CONNECTION_H
#define VRPN_FILE_CONNECTION_H

#include "vrpn_Connection.h"

// vrpn_File_Connection
//
// Tom Hudson, June 1998

// This class *reads* a file written out by vrpn_Connection's logging hooks.

// Logfiles are recorded as *sent*, not as translated by the receiver,
// so we still need to have all the correct names for senders and types
// registered.

class vrpn_File_Connection : public vrpn_Connection {

  public:

    vrpn_File_Connection (const char * file_name);
    virtual ~vrpn_File_Connection (void);

    virtual int mainloop (void);

  protected:

    // tokens for VRPN control messages

    long d_controllerId;

    long d_set_replay_rate_type;
    long d_reset_type;

    // time-keeping

    struct timeval d_time;  // current time in file
    struct timeval d_start_time;  // time of first record in file
    struct timeval d_runtime;  // elapsed file time (d_time - d_start_time)

    struct timeval d_now_time;  // wallclock time of last read
    struct timeval d_next_time;  // elapsed wallclock time

    float d_rate;  // scale factor for wallclock time

    // the actual mechanics of the logfile

    int d_file_handle;

    virtual int close_file (void);

    // handlers for VRPN control messages

    static int handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static int handle_reset (void *, vrpn_HANDLERPARAM);
};


#endif  // VRPN_FILE_CONNECTION_H



