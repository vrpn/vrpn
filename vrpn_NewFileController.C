#include "vrpn_NewFileController.h"

// initialize static variable
vrpn_FileController * vrpn_FileController::sp_pfc = 0;

//**************************************************************************
//**************************************************************************
//
// vrpn_FileController: public: c'tors and init
//
//**************************************************************************
//**************************************************************************

vrpn_FileController::vrpn_FileController( vrpn_FileConnection * pfc )
{
	// convert it to a FileConnection_ControllerInterface *
	// and use it.
	p_pfcci = dynamic_cast<vrpn_FileConnection_ControllerInterface*>(pfc);
	
	//...
}

FileController *
vrpn_FileController::get_FileController( vrpn_ClientConnectionController * pccc )
{
	BaseConnection * pbc = pccc->get_BaseConnection();
	FileConnection * pfc = dynamic_cast<FileConnection*>(pbc);
	// or however you want to do this
	
	if (! pfc) return 0;  // pbc not a FileConnection*
	if (! sp_pfc) {
		sp_pfc = new FileController( pfc );
	}
	if (sp_pfc->p_pfcci == pfc) {  // may need smarter == test
		return sp_pfc;
	} else {
		// what do we do here?
		// This is an error condition.
		// It's a singleton.
		// i say we just return NULL and have the user deal w/ it - SS
		return NULL;
	}
}


//**************************************************************************
//**************************************************************************
//
// vrpn_FileController: public: playback functions
//
//**************************************************************************
//**************************************************************************

vrpn_int32 vrpn_FileController::time_since_connection_open (struct timeval * elapsed_time)
{
	return p_pfcci->time_since_connection_open (elapsed_time);
}
	
// rate of 0.0 is paused, 1.0 is normal speed
void svrpn_FileController::et_replay_rate(vrpn_float32 rate)
{
	return p_pfcci->set_replay_rate(rate);
}
	
// resets to the beginning of the file
vrpn_int32 vrpn_FileController::reset ()      
{
	return p_pfcci->reset();
}

// returns 1 if we're at the end of file
vrpn_int32 vrpn_FileController::eof()
{
	return p_pfcci->eof();
}

// end_time for play_to_time() is an elapsed time
// returns -1 on error or EOF, 0 on success
vrpn_int32 vrpn_FileController::play_to_time (vrpn_float64 end_time)
{
	return p_pfcci->play_to_time(end_time);
}

vrpn_int32 vrpn_FileController::play_to_time (struct timeval end_time)
{
	return p_pfcci->play_to_time(end_time);
}
	
// end_filetime is an absolute time, corresponding to the
// timestamps of the entries in the file,
// returns -1 on error or EOF, 0 on success
vrpn_int32 vrpn_FileController::play_to_filetime(timeval end_filetime)
{
	return p_pfcci->play_to_filetime(end_filetime);
}
	
// plays the next entry, returns -1 or error or EOF, 0 otherwise
vrpn_int32 vrpn_FileController::playone()
{
	return p_pfcci->playone();
}
	
// plays at most one entry, but won't play past end_filetime
// returns 0 on success, 1 if at end_filetime, -1 on error or EOF
vrpn_int32 vrpn_FileController::playone_to_filetime(timeval end_filetime)
{
	return p_pfcci->playone_to_filetime(end_filetime);
}

// returns the elapsed time of the file
timeval vrpn_FileController::get_length()
{
	return p_pfcci->get_length();
}

vrpn_float64 vrpn_FileController::get_length_secs()
{
	return p_pfcci->get_length_secs();
}

// returns the timestamp of the earliest in time user message
timeval vrpn_FileController::get_lowest_user_timestamp()
{
	return p_pfcci->get_lowest_user_timestamp();
}

// jump_to_time sets the current position to the given elapsed time
vrpn_int32 vrpn_FileController::jump_to_time(vrpn_float64 newtime)
{
	return p_pfcci->jump_to_time(newtime);
}

vrpn_int32 vrpn_FileController::jump_to_time(timeval newtime)
{
	return p_pfcci->jump_to_time(newtime);
}
