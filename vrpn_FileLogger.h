//
// file vrpn_FileLogger.h
//
// This file implements logging functionality that was formerly
// a part of vrpn_Connection.

#ifndef VRPN_FILE_LOGGER_H
#define VRPN_FILE_LOGGER_H
#include <stdio.h>  // for FILE

#ifndef VRPN_SHARED_H
#include "vrpn_Shared.h"
#endif

#include <vrpn_ConnectionOldCommonStuff.h>

class vrpn_FileLogger {

public: // c'tors and d'tors

    vrpn_FileLogger(char *_d_logname,
                    vrpn_int32 _d_logmode, 
                    vrpn_int32 _d_logfilehandle = 0,
                    FILE *_d_logfile = NULL,
                    vrpn_LOGLIST *_d_logbuffer = NULL,
                    vrpn_LOGLIST *_d_firstlogentry = NULL, 
                    vrpnLogFilterEntry *_d_logfilters  = NULL);
    ~vrpn_FileLogger(void);

public: // logging functions

    virtual vrpn_int32 log_message (vrpn_int32 payload_len, 
                                    struct timeval time,
                                    vrpn_int32 type, 
                                    vrpn_int32 sender, 
                                    const char * buffer);


    // Sets up a filter function for logging.
    // Any user message to be logged is first passed to this function,
    // and will only be logged if the function returns zero (XXX).
    // NOTE:  this only affects local logging - remote logging
    // is unfiltered!  Only user messages are filtered;  all system
    // messages are logged.
    // Returns nonzero on failure.
    virtual vrpn_int32 register_log_filter (vrpn_LOGFILTER filter, 
                                            void * userdata);

    // Returns nonzero if we shouldn't log this message.
    virtual vrpn_int32 check_log_filters (vrpn_HANDLERPARAM message);

protected: // setup and teardown

    virtual vrpn_int32 open_log(void);
    virtual vrpn_int32 close_log(void);

private: // data members

    vrpn_LOGLIST * d_logbuffer;       // last entry in log
    vrpn_LOGLIST * d_first_log_entry; // first entry in log
    char * d_logname;                 // name of file to write log to
    vrpn_int32 d_logmode;             // logging incoming, outgoing, or both
	
    vrpn_int32 d_logfile_handle;
    FILE * d_logfile;
    vrpnLogFilterEntry *d_logfilters;
    
};

#endif VRPN_FILE_LOGGER_H
