#include "vrpn_FileConnection.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>


#define time_greater(t1,t2)     ( (t1.tv_sec > t2.tv_sec) || \
                                 ((t1.tv_sec == t2.tv_sec) && \
                                  (t1.tv_usec > t2.tv_usec)) )
#define time_add(t1,t2,tr)     { (tr).tv_sec = (t1).tv_sec + (t2).tv_sec ; \
                                  (tr).tv_usec = (t1).tv_usec + (t2).tv_usec ; \
                                  if ((tr).tv_usec >= 1000000L) { \
                                        (tr).tv_sec++; \
                                        (tr).tv_usec -= 1000000L; \
                                  } }
#define time_subtract(t1,t2,tr) \
  { \
    (tr).tv_sec = (t1).tv_sec - (t2).tv_sec ; \
    (tr).tv_usec = (t1).tv_usec - (t2).tv_usec ;\
    if ((tr).tv_usec < 0) { \
      (tr).tv_sec--; \
      (tr).tv_usec += 1000000L; \
    } \
  }

#define time_multiply(t1,f,tr) \
  { \
    (tr).tv_sec = (t1).tv_sec * f; \
    (tr).tv_usec = (t1).tv_usec * f; \
    if ((tr).tv_usec > 1000000L) { \
      (tr).tv_sec += (tr).tv_usec / 1000000L; \
      (tr).tv_usec %= 1000000L; \
    } \
  }

vrpn_File_Connection::vrpn_File_Connection (const char * file_name) :
    vrpn_Connection (file_name),
    d_controllerId (register_sender("vrpn File Controller")),
    d_set_replay_rate_type
                 (register_message_type("vrpn File set replay rate")),
    d_reset_type (register_message_type("vrpn File reset")),
    d_rate (1.0f),
    d_file_handle (-1)
{
  const char * bare_file_name;

  register_handler(d_set_replay_rate_type, handle_set_replay_rate,
                      this, d_controllerId);
  register_handler(d_reset_type, handle_reset, this, d_controllerId);

  d_start_time.tv_sec = d_start_time.tv_usec = 0L;
  d_next_time.tv_sec = d_next_time.tv_usec = 0L;  // simulated elapsed time

  bare_file_name = vrpn_copy_file_name(file_name);
  if (!bare_file_name) {
    fprintf(stderr, "vrpn_File_Connection:  Out of memory!\n");
    return;
  }

  d_file_handle = open(bare_file_name, O_RDONLY);
  if (d_file_handle == -1)
    fprintf(stderr, "vrpn_File_Connection:  "
                    "Could not open file \"%s\".\n", bare_file_name);

  if (bare_file_name)
    delete [] (char *) bare_file_name;
}

// virtual
vrpn_File_Connection::~vrpn_File_Connection (void) {

  close_file();

}

// virtual
int vrpn_File_Connection::mainloop (void) {

  vrpn_HANDLERPARAM header;
  struct timeval last_time;
  struct timeval skip_time;
  char buffer [8000];
  int retval;

  if (d_file_handle == -1)
    return 0;

  // compute elapsed time and when to read

  last_time = d_now_time;
  gettimeofday(&d_now_time, NULL);
  if ((last_time.tv_sec == 0L) &&
      (last_time.tv_usec == 0L))
    last_time = d_now_time;

  // scale elapsed time by d_rate

  time_subtract(d_now_time, last_time, skip_time);
  time_multiply(skip_time, d_rate, skip_time);
  time_add(d_next_time, skip_time, d_next_time);

  // are we ready for the next message?

  if (time_greater(d_runtime, d_next_time))
    return 0;

  // get the header of the next message

  retval = read(d_file_handle, (char *) &header, sizeof(header));

  // return 0 if nothing to read OR end-of-file;
  // the latter isn't an error state
  if (retval <= 0) {
    close_file();
    return 0;
  }

  header.type = ntohl(header.type);
  header.sender = ntohl(header.sender);
  header.msg_time.tv_sec = ntohl(header.msg_time.tv_sec);
  header.msg_time.tv_usec = ntohl(header.msg_time.tv_usec);
  header.payload_len = ntohl(header.payload_len);

  // keep track of the time

  d_time.tv_sec = header.msg_time.tv_sec;
  d_time.tv_usec = header.msg_time.tv_usec;

  // get the body of the next message

  retval = read(d_file_handle, buffer, header.payload_len);

  // return 0 if nothing to read OR end-of-file;
  // the latter isn't an error state
  if (retval <= 0) {
    close_file();
    return 0;
  }

  // call handlers

  if (header.type >= 0) {

    if (other_types[header.type].local_id >= 0)

      if (do_callbacks_for(other_types[header.type].local_id,
                           other_senders[header.sender].local_id,
                           header.msg_time, header.payload_len,
                           buffer))
        return -1;

  } else {  // system handler

    header.buffer = buffer;
    if (system_messages[-header.type](this, header)) {
      fprintf(stderr, "vrpn_Connection::handle_udp_messages:  "
                      "Nonzero system return.\n");
      return -1;
    }
  }

  // keep track of the time

  if (!d_start_time.tv_sec && !d_start_time.tv_usec)
    d_start_time = d_time;

  time_subtract(d_time, d_start_time, d_runtime);

  return 0;
}

// virtual
int vrpn_File_Connection::close_file (void) {

  if (d_file_handle != -1)
    close(d_file_handle);

  d_file_handle = -1;

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
  me->d_rate = *((float *) value);

#endif

  return 0;
}

// static
int vrpn_File_Connection::handle_reset
         (void * userdata, vrpn_HANDLERPARAM) {
  vrpn_File_Connection * me = (vrpn_File_Connection *) userdata;

  me->d_time = me->d_start_time;

  // elapsed file time
  me->d_runtime.tv_sec = me->d_runtime.tv_usec = 0L;

  // elapsed wallclock time
  me->d_next_time.tv_sec = me->d_next_time.tv_usec = 0L;

  return 0;
}



