#include "vrpn_NewFileController.h"

//**************************************************************************
//**************************************************************************
//
// vrpn_NewFileController: public: c'tors and init
//
//**************************************************************************
//**************************************************************************

// create one of these this way
//
//   void foo (vrpn_ClientConnectionController* pccc) {
//      vrpn_NewFileController* pfc
//          = vrpn_NewFileController::get_FileController (pccc);
//
//      if (! pfc) {
//         // it wasn't a file connection
//      } else {
//         // you're golden
//      }

vrpn_NewFileController::vrpn_NewFileController (
    //vrpn_NewFileConnection* pfc
    vrpn_FileConnectionInterface* pfcci)
{
    // convert it to a FileConnectionInterface *
    // and use it.
    //p_pfcci = dynamic_cast<vrpn_FileConnectionInterface*>(pfc);
    d_pfcci = pfcci;
}


vrpn_NewFileController*
vrpn_NewFileController::get_FileController(
    vrpn_ClientConnectionController* pccc )
{
    vrpn_BaseConnection* pbc = pccc->get_BaseConnection();

    // {{{ change this
    //     let's do it differently later

    // get a handle to the FileConnection fro the BaseConnection
#ifdef VRPN_HAVE_DYNAMIC_CAST
    vrpn_NewFileConnection* pfconn
        = dynamic_cast<vrpn_NewFileConnection*>(pbc);
#else
    vrpn_NewFileConnection* pfconn = pbc->get_FileConnectionPtr();
#endif

    if (! pfconn) {
        // the BaseConnection* wasn't actually a FileConnection*
        return 0;
    }
    
    // }}}

    // ok.  we now have a valid FileConnection ptr.  Let's use it
    // to create a new FileController, and then return it.
    
    // create a FileController attached to the FileConnection
    return new vrpn_NewFileController (pfconn);
}


//**************************************************************************
//**************************************************************************
//
// vrpn_NewFileController: public: playback functions
//
//**************************************************************************
//**************************************************************************

vrpn_int32 vrpn_NewFileController::time_since_connection_open (struct timeval * elapsed_time)
{
	return d_pfcci->time_since_connection_open (elapsed_time);
}
	
// rate of 0.0 is paused, 1.0 is normal speed
void vrpn_NewFileController::set_replay_rate(vrpn_float32 rate)
{
	d_pfcci->set_replay_rate(rate);
}
   
// resets to the beginning of the file
vrpn_int32 vrpn_NewFileController::reset ()      
{
	return d_pfcci->reset();
}

// returns 1 if we're at the end of file
vrpn_int32 vrpn_NewFileController::eof()
{
	return d_pfcci->eof();
}

// end_time for play_to_time() is an elapsed time
// returns -1 on error or EOF, 0 on success
vrpn_int32 vrpn_NewFileController::play_to_time (vrpn_float64 end_time)
{
	return d_pfcci->play_to_time(end_time);
}

vrpn_int32 vrpn_NewFileController::play_to_time (struct timeval end_time)
{
	return d_pfcci->play_to_time(end_time);
}
	
// end_filetime is an absolute time, corresponding to the
// timestamps of the entries in the file,
// returns -1 on error or EOF, 0 on success
vrpn_int32 vrpn_NewFileController::play_to_filetime(timeval end_filetime)
{
	return d_pfcci->play_to_filetime(end_filetime);
}
	
// plays the next entry, returns -1 or error or EOF, 0 otherwise
vrpn_int32 vrpn_NewFileController::playone()
{
	return d_pfcci->playone();
}
	
// plays at most one entry, but won't play past end_filetime
// returns 0 on success, 1 if at end_filetime, -1 on error or EOF
vrpn_int32 vrpn_NewFileController::playone_to_filetime(timeval end_filetime)
{
	return d_pfcci->playone_to_filetime(end_filetime);
}

// returns the elapsed time of the file
timeval vrpn_NewFileController::get_length()
{
	return d_pfcci->get_length();
}

vrpn_float64 vrpn_NewFileController::get_length_secs()
{
	return d_pfcci->get_length_secs();
}

// returns the timestamp of the earliest in time user message
timeval vrpn_NewFileController::get_lowest_user_timestamp()
{
	return d_pfcci->get_lowest_user_timestamp();
}

// jump_to_time sets the current position to the given elapsed time
vrpn_int32 vrpn_NewFileController::jump_to_time(vrpn_float64 newtime)
{
	return d_pfcci->jump_to_time(newtime);
}

vrpn_int32 vrpn_NewFileController::jump_to_time(timeval newtime)
{
	return d_pfcci->jump_to_time(newtime);
}
