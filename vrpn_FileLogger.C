#include <vrpn_FileLogger.h>



//**************************************************************************
//**************************************************************************
//
// vrpn_FileLogger: public: c'tors and d'tors
//
//**************************************************************************
//**************************************************************************

vrpn_FileLogger::vrpn_FileLogger(char *_d_logname,
								 vrpn_int32 _d_logmode, 
								 vrpn_int32 _d_logfilehandle,
								 FILE *_d_logfile,
								 vrpn_LOGLIST *_d_logbuffer,
								 vrpn_LOGLIST *_d_firstlogentry,
								 vrpnLogFilterEntry *_d_logfilters) :
    d_logbuffer (_d_logbuffer),
    d_first_log_entry (_d_firstlogentry),
    d_logmode (_d_logmode),
    d_logfile_handle (_d_logfilehandle),
    d_logfile (_d_logfile),
    d_log_filters (_d_logfilters)
{

		d_logname = new char[strlen(logfile_name)+1];
		strcpy(d_logname,logfile_name,strlen(logfile_name)+1);
		open_log();
}

vrpn_FileLogger::~vrpn_FileLogger(void)
{
	close_log();
}


//**************************************************************************
//**************************************************************************
//
// vrpn_FileLogger: public: logging functions
//
//**************************************************************************
//**************************************************************************

// differs a bit from old log_message found in OneConnection. 
// type translation from remote to local is done outside this class
vrpn_int32 vrpn_FileLogger::log_message (vrpn_int32 len, struct timeval time,
                vrpn_int32 type, vrpn_int32 sender, const char * buffer)
{
  vrpn_LOGLIST * lp;
  vrpn_HANDLERPARAM p;

  lp = new vrpn_LOGLIST;
  if (!lp) {
    fprintf(stderr, "vrpn_FileLogger::log_message:  "
                    "Out of memory!\n");
    return -1;
  }
  lp->data.type = htonl(type);
  lp->data.sender = htonl(sender);

  // adjust the time stamp by the clock offset (as in do_callbacks_for)
  struct timeval tvTemp = vrpn_TimevalSum(time, tvClockOffset);
  
  lp->data.msg_time.tv_sec = htonl(tvTemp.tv_sec);
  lp->data.msg_time.tv_usec = htonl(tvTemp.tv_usec);

  lp->data.payload_len = htonl(len);
  lp->data.buffer = new char [len];
  if (!lp->data.buffer) {
    fprintf(stderr, "vrpn_FileLogger::log_message:  "
                    "Out of memory!\n");
    delete lp;
    return -1;
  }
  // need to explicitly override the const
  // NOTE: then this should probably not be a const char * (weberh 9/14/98)
  memcpy((char *) lp->data.buffer, buffer, len);

  // filter (user) messages
  if (type >= 0) {  // do not filter system messages

      p.type = local_type_id(type);
      p.sender = local_sender_id(sender);
	  

	  p.msg_time.tv_sec = time.tv_sec;
	  p.msg_time.tv_usec = time.tv_usec;
	  p.payload_len = len;
	  p.buffer = lp->data.buffer;

	  if (check_log_filters(p)) {  // abort logging
		  delete [] (char *) lp->data.buffer;
		  delete lp;
		  return 0;  // this is not a failure - do not return nonzero!
	  }
  }

  lp->next = d_logbuffer;
  lp->prev = NULL;
  if (d_logbuffer)
    d_logbuffer->prev = lp;
  d_logbuffer = lp;
  if (!d_first_log_entry)
    d_first_log_entry = lp;

  return 0;
}


// virtual
vrpn_int32 vrpn_FileLogger::register_log_filter (vrpn_LOGFILTER filter,
												 void * userdata) {
  vrpnLogFilterEntry * newEntry;

  newEntry = new vrpnLogFilterEntry;
  if (!newEntry) {
    fprintf(stderr, "vrpn_FileLogger::register_log_filter:  "
                    "Out of memory.\n");
    return -1;
  }

  newEntry->filter = filter;
  newEntry->userdata = userdata;
  newEntry->next = endpoint.d_log_filters;
  endpoint.d_log_filters = newEntry;

  return 0;
}

vrpn_int32 vrpn_OneConnection::check_log_filters (vrpn_HANDLERPARAM message) {

	vrpnLogFilterEntry * nextFilter;

	for (nextFilter = d_log_filters; nextFilter; nextFilter = nextFilter->next)
		if ((*nextFilter->filter)(nextFilter->userdata, message))
			return 1;  // don't log

	return 0;
}

//**************************************************************************
//**************************************************************************
//
// vrpn_FileLogger: protected: setup and teardown
//
//**************************************************************************
//**************************************************************************

vrpn_int32 vrpn_FileLogger::open_log (void) {

  if (!d_logname) {
	  fprintf(stderr, "vrpn_FileLogger::open_log:  "
			  "Log file has no name.\n");
	  return -1;
  }
  if (d_logfile) {
	  fprintf(stderr, "vrpn_FileLogger::open_log:  "
			  "Log file is already open.\n");
	  return 0;  // not a catastrophic failure
  }

  // Can't use this because MICROSOFT doesn't support Unix standards!

  // Create the file in write-only mode.
  // Permissions are 744 (user read/write, group & others read)
  // Return an error if it already exists (on some filesystems;
  // O_EXCL doesn't work on linux)

  //d_logfile_handle = open(d_logname, O_WRONLY | O_CREAT | O_EXCL,
  //                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
  // check to see if it exists
  d_logfile = fopen(d_logname, "r");
  if (d_logfile) {
	  fprintf(stderr, "vrpn_FileLogger::open_log:  "
			  "Log file \"%s\" already exists.\n", d_logname);
	  d_logfile = NULL;
  } else
	  d_logfile = fopen(d_logname, "wb");
  
  if (!d_logfile) {
	  fprintf(stderr, "vrpn_FileLogger::open_log:  "
			  "Couldn't open log file \"%s\".\n", d_logname);

	  // Try to write to "/tmp/vrpn_emergency_log"
	  d_logfile = fopen("/tmp/vrpn_emergency_log", "r");
	  if (d_logfile)
		  d_logfile = NULL;
	  else
		  d_logfile = fopen("/tmp/vrpn_emergency_log", "wb");

	  if (!d_logfile) {
		  fprintf(stderr, "vrpn_FileLogger::open_log:\n  "
				  "Couldn't open emergency log file "
				  "\"/tmp/vrpn_emergency_log\".\n");
		  
		  return -1;
	  } else
		  fprintf(stderr, "Writing to /tmp/vrpn_emergency_log instead.\n");
	  
  }
  
  return 0;
}


vrpn_int32 vrpn_FileLogger::close_log (void) {

  char magicbuf [501];  // HACK
  vrpn_LOGLIST * lp;
  vrpn_int32 host_len;
  vrpn_int32 final_retval = 0;
  vrpn_int32 retval;

  // Write out the log header (magic cookie)
  // TCH 20 May 1999

  // There's at least one hack here:
  //   What logging mode should a client that plays back the log at a
  // later time be forced into?  I believe NONE, but there might be
  // arguments the other way?

  retval = write_vrpn_cookie(magicbuf, vrpn_cookie_size() + 1,
                             vrpn_LOG_NONE);
  if (retval < 0) {
    fprintf(stderr, "vrpn_FileLogger::close_log:  "
                    "Couldn't create magic cookie.\n");
    final_retval = -1;
  }

  retval = fwrite(magicbuf, 1, vrpn_cookie_size(), d_logfile);
  if (retval != vrpn_cookie_size()) {
    fprintf(stderr, "vrpn_FileLogger::close_log:  "
                    "Couldn't write magic cookie to log file "
                    "(got %d, expected %d).\n",
            retval, vrpn_cookie_size());
    lp = d_logbuffer;
    final_retval = -1;
  }


  // Write out the messages in the log,
  // starting at d_first_log_entry and working backwards

  if (!d_logfile) {
    fprintf(stderr, "vrpn_FileLogger::close_log:  "
                    "Log file is not open!\n");

    // Abort writing out log without destroying data needed to
    // clean up memory.

    d_first_log_entry = NULL;
    final_retval = -1;
  }

  for (lp = d_first_log_entry; lp && !final_retval; lp = lp->prev) {

    retval = fwrite(&lp->data, 1, sizeof(lp->data), d_logfile);
        //vrpn_noint_block_write(d_logfile_handle, (char *) &lp->data,
        //                            sizeof(lp->data));
    
    if (retval != sizeof(lp->data)) {
      fprintf(stderr, "vrpn_FileLogger::close_log:  "
                      "Couldn't write log file (got %d, expected %d).\n",
              retval, sizeof(lp->data));
      lp = d_logbuffer;
      final_retval = -1;
      continue;
    }

    host_len = ntohl(lp->data.payload_len);
    retval = fwrite(lp->data.buffer, 1, host_len, d_logfile);

    if (retval != host_len) {
      fprintf(stderr, "vrpn_FileLogger::close_log:  "
                      "Couldn't write log file.\n");
      lp = d_logbuffer;
      final_retval = -1;
      continue;
    }
  }

  retval = fclose(d_logfile);
  if (retval) {
    fprintf(stderr, "vrpn_FileLogger::close_log:  "
                    "close of log file failed!\n");
    final_retval = -1;
  }

  if (d_logname)
    delete [] d_logname;

  // clean up the linked list
  while (d_logbuffer) {
    lp = d_logbuffer->next;
    delete [] (char *) d_logbuffer->data.buffer;  // ugly cast
    delete d_logbuffer;
    d_logbuffer = lp;
  }

  d_logname = NULL;
  d_first_log_entry = NULL;

  d_logfile_handle = -1;
  d_logfile = NULL;

  return final_retval;
}
