#ifndef VRPN_FILE_CONNECTION_CONTROLLER_INTERFACE_H
#define VRPN_FILE_CONNECTION_CONTROLLER_INTERFACE_H

//=============================================================================
// vrpn_FileConnection_ControllerInterface
//
// Stefan Sain, July 1999
//
// This class serves purely as an abstract interface for the vrpn_FileConnection 
// class to inherit from. It contains all the log playback control 
// functions originally found in the old version of vrpn_FileConnection
//=============================================================================

#include "vrpn_Shared.h"
#include "vrpn_CommonSystemIncludes.h"
#include "vrpn_ConnectionCommonStuff.h"

class vrpn_FileConnection_ControllerInterface 
{

public:

    vrpn_FileConnection_ControllerInterface();
    virtual ~vrpn_FileConnection_ControllerInterface();
	
    virtual vrpn_int32 time_since_connection_open (struct timeval * elapsed_time) = 0;
	
	// rate of 0.0 is paused, 1.0 is normal speed
    virtual void set_replay_rate(vrpn_float32 rate) = 0;
	
	// resets to the beginning of the file
    virtual vrpn_int32 reset () = 0;      

	// returns 1 if we're at the end of file
	virtual vrpn_int32 eof() = 0;
	
	// end_time for play_to_time() is an elapsed time
	// returns -1 on error or EOF, 0 on success
    virtual vrpn_int32 play_to_time (vrpn_float64 end_time) = 0;
    virtual vrpn_int32 play_to_time (struct timeval end_time) = 0;
	
	// end_filetime is an absolute time, corresponding to the
	// timestamps of the entries in the file,
	// returns -1 on error or EOF, 0 on success
    virtual vrpn_int32 play_to_filetime(timeval end_filetime) = 0;
	
	// plays the next entry, returns -1 or error or EOF, 0 otherwise
    virtual vrpn_int32 playone() = 0;
	
	// plays at most one entry, but won't play past end_filetime
	// returns 0 on success, 1 if at end_filetime, -1 on error or EOF
    virtual vrpn_int32 playone_to_filetime(timeval end_filetime) = 0;

	// returns the elapsed time of the file
    virtual timeval get_length() = 0;
    virtual vrpn_float64 get_length_secs() = 0;

	// returns the timestamp of the earliest in time user message
    virtual timeval get_lowest_user_timestamp() = 0;
	
	// jump_to_time sets the current position to the given elapsed time
    virtual vrpn_int32 jump_to_time(vrpn_float64 newtime) = 0;
    virtual vrpn_int32 jump_to_time(timeval newtime) = 0;
	
protected:

    virtual void play_to_user_message() = 0;
	
	// helper function for mainloop()
    virtual vrpn_int32 need_to_play(timeval filetime) = 0;
	
	// checks the cookie at
	// the head of the log file;
	//  exit on error!
    virtual vrpn_int32 read_cookie () = 0;
    
	// appends entry to d_logTail
	virtual vrpn_int32 read_entry () = 0; 

	// returns 0 on success, 1 on EOF, -1 on error
    virtual vrpn_int32 close_file () = 0;

    // handlers for VRPN control messages
    static vrpn_int32 handle_set_replay_rate (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_reset (void *, vrpn_HANDLERPARAM);
    static vrpn_int32 handle_play_to_time (void *, vrpn_HANDLERPARAM);

};

#endif // VRPN_FILE_CONNECTION_CONTROLLER_INTERFACE_H
