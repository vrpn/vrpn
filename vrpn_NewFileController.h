#ifndef VRPN_FILE_CONTROLLER_H
#define VRPN_FILE_CONTROLLER_H

#include "vrpn_Shared.h"
#include "vrpn_CommonSystemIncludes.h"
#include "vrpn_ConnectionCommonStuff.h"
#include "vrpn_FileConnectionInterface.h"
#include "vrpn_NewFileConnection.h"
#include "vrpn_ClientConnectionController.h"

//=============================================================================
// vrpn_NewFileController
//
// Stefan Sain, July 1999
//
// This class is what the user program instantiates to get the functionality
// of vrpn_FileConnectionInterface. It is completely different 
// from the previous incarnation of vrpn_NewFileController
//=============================================================================
class vrpn_NewFileController 
{
private: // data members

    vrpn_FileConnectionInterface * d_pfcci;

public: // c'tors and init
    vrpn_NewFileController( vrpn_FileConnectionInterface * );

    static vrpn_NewFileController * get_FileController( vrpn_ClientConnectionController * pccc );
	
public: // playback functions

    virtual vrpn_int32 time_since_connection_open (struct timeval * elapsed_time);
	
	// rate of 0.0 is paused, 1.0 is normal speed
    virtual void set_replay_rate(vrpn_float32 rate);
	
	// resets to the beginning of the file
    virtual vrpn_int32 reset ();      

	// returns 1 if we're at the end of file
	virtual vrpn_int32 eof();
	
	// end_time for play_to_time() is an elapsed time
	// returns -1 on error or EOF, 0 on success
    virtual vrpn_int32 play_to_time (vrpn_float64 end_time);
    virtual vrpn_int32 play_to_time (struct timeval end_time);
	
	// end_filetime is an absolute time, corresponding to the
	// timestamps of the entries in the file,
	// returns -1 on error or EOF, 0 on success
    virtual vrpn_int32 play_to_filetime(timeval end_filetime);
	
	// plays the next entry, returns -1 or error or EOF, 0 otherwise
    virtual vrpn_int32 playone();
	
	// plays at most one entry, but won't play past end_filetime
	// returns 0 on success, 1 if at end_filetime, -1 on error or EOF
    virtual vrpn_int32 playone_to_filetime(timeval end_filetime);

	// returns the elapsed time of the file
    virtual timeval get_length();
    virtual vrpn_float64 get_length_secs();

	// returns the timestamp of the earliest in time user message
    virtual timeval get_lowest_user_timestamp();
	
	// jump_to_time sets the current position to the given elapsed time
    virtual vrpn_int32 jump_to_time(vrpn_float64 newtime);
    virtual vrpn_int32 jump_to_time(timeval newtime);

};




	


#endif  // VRPN_FILE_CONTROLLER_H
