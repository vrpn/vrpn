#include "vrpn_BaseConnectionController.h"



////////////////////////////////////////////////////////
// vrpn_ConnectionController constructors and destructor
//

vrpn_ConnectionController::vrpn_ConnectionController()
{
    // ...XXX...

    // init for clock server/client
    // need to init gettimeofday for the pc
    struct timeval tv;
    gettimeofday(&tv, NULL);


}

vrpn_ConnectionController::~vrpn_ConnectionController()
{
    //...XXX...
}


//**************************************************************************
//**************************************************************************
//
// vrpn_BaseConnectionController: public: logging get functions
//
//**************************************************************************
//**************************************************************************

vrpn_int32 vrpn_BaseConnectionController::get_local_logmode(void)
{
    return d_local_logmode;
}

// it is assumed the caller has provided ample storage for the filename
void vrpn_BaseConnectionController::get_local_logfile_name(char * logname_copy)
{
    strcpy(logname_copy,d_local_logname,strlen(d_local_logname)+1);
}

vrpn_int32 vrpn_BaseConnectionController::get_remote_logmode(void)
{
    return d_remote_logmode;
}

// it is assumed the caller has provided ample storage for the filename
void vrpn_BaseConnectionController::get_remote_logfile_name(char * logname_copy)
{
    strcpy(logname_copy,d_remote_logname,strlen(d_remote_logname)+1);
}
