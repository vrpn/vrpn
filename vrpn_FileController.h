#ifndef VRPN_FILE_CONTROLLER_H
#define VRPN_FILE_CONTROLLER_H

#ifndef _WIN32
#include <sys/time.h>  // for struct timeval
#else
#include "vrpn_Shared.h"
#endif

class vrpn_Connection;  // from vrpn_Connection.h

// class vrpn_File_Controller
// Tom Hudson, July 1998

// Controls a file connection (logfile playback).
// Can be attached to any vrpn_Connection.
// vrpn_File_Connections will respond to the messages.

class vrpn_File_Controller {

  public:

    vrpn_File_Controller (vrpn_Connection *);
    ~vrpn_File_Controller (void);

    void set_replay_rate (float = 1.0);
      // Sets the rate at which the file is replayed.

    void reset (void);
      // Returns to the beginning of the file.
      // Does NOT reset rate to 1.0.
      // Equivalent to set_to_time(< 0L, 0L >)

    void play_to_time (struct timeval t);
      // Goes to an arbitrary elapsed time t in the file,
      // triggering all events between the current time and t.
      // Does not work in the past (use reset() first).

    //void jump_to_time (struct timeval t);

  protected:

    vrpn_Connection * d_connection;

    long d_myId;

    long d_set_replay_rate_type;
    long d_reset_type;
    long d_play_to_time_type;
    //long d_jump_to_time_type;
};




#endif  // VRPN_FILE_CONTROLLER_H
