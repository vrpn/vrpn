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

	vrpn_FileLogger(vrpn_LOGLIST *_d_logbuffer,
					vrpn_LOGLIST *_d_firstlogentry, 
					char *_d_logname,
					long _d_logmode, 
					int _d_logfilehandle,
					FILE *_d_logfile, 
					vrpnLogFilterEntry *_d_logfilters );
	~vrpn_FileLogger(void);

public: // logging functions

	virtual vrpn_int32 log_message (vrpn_int32 payload_len, 
									struct timeval time,
									vrpn_int32 type, 
									vrpn_int32 sender, 
									const char * buffer);

	// Returns nonzero if we shouldn't log this message.
	virtual vrpn_int32 check_log_filters (vrpn_HANDLERPARAM message);

protected: // setup and teardown

	virtual vrpn_int32 open_log(void);
	virtual vrpn_int32 close_log(void);

private: // data members

	vrpn_LOGLIST * d_logbuffer;  // last entry in log
	vrpn_LOGLIST * d_first_log_entry;  // first entry in log
	char * d_logname;            // name of file to write log to
	vrpn_int32 d_logmode;              // logging incoming, outgoing, or both
	
	vrpn_in32 d_logfile_handle;
	FILE * d_logfile;
	vrpnLogFilterEntry *d_logfilters;

#endif VRPN_FILE_LOGGER_H
