#include "vrpn_FileConnection.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#include <netinet/in.h>
#endif



vrpn_File_Connection::vrpn_File_Connection (const char * file_name) :
    vrpn_Connection (file_name),
    d_controllerId (register_sender("vrpn File Controller")),
    d_set_replay_rate_type
                 (register_message_type("vrpn File set replay rate")),
    d_reset_type (register_message_type("vrpn File reset")),
    d_rate (1.0f),
    d_file (NULL),
    d_logHead (NULL),
    d_logTail (NULL),
    d_currentLogEntry (NULL)
{
  const char * bare_file_name;

  register_handler(d_set_replay_rate_type, handle_set_replay_rate,
                      this, d_controllerId);
  register_handler(d_reset_type, handle_reset, this, d_controllerId);

  bare_file_name = vrpn_copy_file_name(file_name);
  if (!bare_file_name) {
    fprintf(stderr, "vrpn_File_Connection:  Out of memory!\n");
    return;
  }

  d_file = fopen(bare_file_name, "rb");
  if (!d_file)
    fprintf(stderr, "vrpn_File_Connection:  "
                    "Could not open file \"%s\".\n", bare_file_name);

  if (bare_file_name)
    delete [] (char *) bare_file_name;

  // PRELOAD
  // TCH 16 Sept 1998

  //fprintf(stderr, "Beginning preload...\n");

  while (!read_entry());
  d_currentLogEntry = d_logHead;  // BUG BUG BUG never plays first entry

  //fprintf(stderr, "Completed preload;  starting time.\n");

  d_start_time.tv_sec = d_start_time.tv_usec = 0L;
  d_next_time.tv_sec = d_next_time.tv_usec = 0L;  // simulated elapsed time
  d_now_time.tv_sec = d_now_time.tv_usec = 0L;
    // necessary for last_time to initialize properly in mainloop()
}

// virtual
vrpn_File_Connection::~vrpn_File_Connection (void) {
  vrpn_LOGLIST * np;

  close_file();

  while (d_logHead) {
    np = d_logHead->next;
    if (d_logHead->data.buffer)
      delete [] (char *) d_logHead->data.buffer;
    delete d_logHead;
    d_logHead = np;
  }

}

// virtual
int vrpn_File_Connection::mainloop (void) {

  struct timeval last_time;
  struct timeval skip_time;
  int retval;

  if (!d_file)
    return 0;

  // compute time elapsed since last call to mainloop()

  last_time = d_now_time;
  gettimeofday(&d_now_time, NULL);
  if ((last_time.tv_sec == 0L) &&
      (last_time.tv_usec == 0L))
    last_time = d_now_time;

  // scale elapsed time by d_rate (rate of replay);
  // this gives us the time to advance (skip_time)
  // our clock (d_next_time).

  skip_time = vrpn_TimevalDiff(d_now_time, last_time);
  skip_time = vrpn_TimevalScale(skip_time, d_rate);
  d_next_time = vrpn_TimevalSum(d_next_time, skip_time);

  // are we ready for the next message?
  // XXX This should really look ahead at the next message
  // (if there is one) and see if it is too soon to play it,
  // rather than reading and sending the message then seeing
  // that it is too late.

  if (vrpn_TimevalGreater(d_runtime, d_next_time))
    return 0;

  // give the user the next message;  fetch from disk if need be

  if (d_currentLogEntry)
    d_currentLogEntry = d_currentLogEntry->next;

  if (!d_currentLogEntry) {
    retval = read_entry();
    if (retval < 0)
      return -1;  // error reading from file
    if (retval > 0)
      return 0;  // end of file;  nothing to replay
    d_currentLogEntry = d_logTail;  // better be non-NULL
  } 

  vrpn_HANDLERPARAM & header = d_currentLogEntry->data;

  // keep track of the time

  d_time.tv_sec = header.msg_time.tv_sec;
  d_time.tv_usec = header.msg_time.tv_usec;

  // call handlers

  if (header.type >= 0) {

    if (other_types[header.type].local_id >= 0)

      if (do_callbacks_for(other_types[header.type].local_id,
                           other_senders[header.sender].local_id,
                           header.msg_time, header.payload_len,
                           header.buffer))
        return -1;

  } else {  // system handler

    if (system_messages[-header.type](this, header)) {
      fprintf(stderr, "vrpn_Connection::handle_udp_messages:  "
                      "Nonzero system return.\n");
      return -1;
    }
  }

  // keep track of the time
  // Set the start time if it hasn't been set yet and then figure
  // out how long in file time we have been running.

  if (!d_start_time.tv_sec && !d_start_time.tv_usec)
    d_start_time = d_time;

  d_runtime = vrpn_TimevalDiff(d_time, d_start_time);

  return 0;
}

// Returns the time since the connection opened.
// Some subclasses may redefine time.

// virtual
int vrpn_File_Connection::time_since_connection_open
                                (struct timeval * elapsed_time) {
  elapsed_time->tv_sec = d_runtime.tv_sec;
  elapsed_time->tv_usec = d_runtime.tv_usec;

  return 0;
}

// virtual
int vrpn_File_Connection::read_entry (void) {

  vrpn_LOGLIST * newEntry;
  int retval;

  newEntry = new vrpn_LOGLIST;
  if (!newEntry) {
    fprintf(stderr, "vrpn_File_Connection::read_entry:  "
                    "Out of memory.\n");
    return -1;
  }

  // get the header of the next message

  vrpn_HANDLERPARAM & header = newEntry->data;
  retval = fread(&header, sizeof(header), 1, d_file);

  // return 1 if nothing to read OR end-of-file;
  // the latter isn't an error state
  if (retval <= 0) {
    // Don't close the file because we might get a reset message...
    // close_file();
    return 1;
  }

  header.type = ntohl(header.type);
  header.sender = ntohl(header.sender);
  header.msg_time.tv_sec = ntohl(header.msg_time.tv_sec);
  header.msg_time.tv_usec = ntohl(header.msg_time.tv_usec);
  header.payload_len = ntohl(header.payload_len);

  // get the body of the next message

  header.buffer = new char [header.payload_len];
  if (!header.buffer) {
    fprintf(stderr, "vrpn_File_Connection::read_entry:  "
                    "Out of memory.\n");
    return -1;
  }

  retval = fread((char *) header.buffer, 1, header.payload_len, d_file);

  // return 1 if nothing to read OR end-of-file;
  // the latter isn't an error state
  if (retval <= 0) {
    // Don't close the file because we might get a reset message...
    //close_file();
    return 1;
  }

  // doubly-linked list maintenance

  newEntry->next = NULL;
  newEntry->prev = d_logTail;
  if (d_logTail)
    d_logTail->next = newEntry;
  d_logTail = newEntry;
  if (!d_logHead)
    d_logHead = d_logTail;

  return 0;
}

// virtual
int vrpn_File_Connection::close_file (void) {

  if (d_file)
    fclose(d_file);

  d_file = NULL;

  return 0;
}

// static
int vrpn_File_Connection::handle_set_replay_rate
         (void * userdata, vrpn_HANDLERPARAM p) {
  vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;

#if (defined(sgi) || defined(hpux) || defined(sparc))

  me->d_rate = *((float *) (p.buffer));

#else

  int value = ntohl(*(int *) (p.buffer));
  me->d_rate = *((float *) &value);

#endif

  return 0;
}

// static
int vrpn_File_Connection::handle_reset
         (void * userdata, vrpn_HANDLERPARAM) {
  vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;

fprintf(stderr, "In vrpn_File_Connection::handle_reset().\n");

  me->d_start_time.tv_sec = me->d_start_time.tv_usec = 0L;
  me->d_time.tv_sec = me->d_time.tv_usec = 0L;

  // elapsed file time
  me->d_runtime.tv_sec = me->d_runtime.tv_usec = 0L;

  // elapsed wallclock time
  me->d_next_time.tv_sec = me->d_next_time.tv_usec = 0L;

/*
  if (me->d_file)
    fseek(me->d_file, 0L, SEEK_SET);
*/

  // BUG BUG BUG
  // Doesn't replay the first log entry.
  // This doesn't bite us because the first log entry is (should?)
  // always be a TCP connection.

  me->d_currentLogEntry = me->d_logHead;

  return 0;
}



