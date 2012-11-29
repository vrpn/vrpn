#include <fcntl.h>                      // for open, O_RDONLY
#include <stdio.h>                      // for printf, fprintf, stderr
#include <stdlib.h>                     // for exit
#include <string.h>                     // for strcmp
#ifndef _WIN32
#include <netinet/in.h>                 // for ntohl
#include <unistd.h>                     // for close, read
#else
    #include <io.h>
#endif

#include <vrpn_Connection.h>            // for vrpn_HANDLERPARAM, etc

#include "vrpn_Shared.h"                // for timeval, vrpn_TimevalMsecs


const int buflen = 8000;

void Usage (const char * name) {
  fprintf(stderr, "Usage:  %s [-n|-s] <filename>\n", name);
  fprintf(stderr,"    -n:  Print names instead of numbers.\n");
  fprintf(stderr,"    -s:  Summary only, start/end/duration\n");
}


int main (int argc, char ** argv) {

  char * filename;
  int name_mode = 0, summary_mode = 0;

  char buffer [buflen];
  vrpn_HANDLERPARAM header;
  int file;
  int retval;

  if (argc < 2) {
    Usage(argv[0]);
    exit(0);
  }

  filename = argv[1];
  if (!strcmp(argv[1], "-n")) {
    filename = argv[2];
    name_mode = 1;
    fprintf(stderr, "FATAL ERROR:  Name mode not implemented.\n");
    exit(0);
  } else if (!strcmp(argv[1], "-s")) {
    filename = argv[2];
    summary_mode = 1;
  }

  #ifdef _WIN32
      // blech
    const int oflag = O_RDONLY | O_BINARY;
  #else
    const int oflag = O_RDONLY;
  #endif
  file = open(filename, oflag);
  if (file == -1) {
    fprintf(stderr, "Couldn't open \"%s\".\n", filename);
    exit(0);
  }

  struct timeval tvFirst, time;
  int cEntries = 0;

  while (1) {
    int len;
    long sender;
    long type;
    int len2;

    retval = read(file, &header, sizeof(header));
    if (retval < 0) { printf("ERROR\n"); close(file); exit(0); }
    if (!retval) {
        if (summary_mode) {
            printf("Last timestamp in file: %ld:%ld\n", time.tv_sec, static_cast<long>(time.tv_usec));
            timeval tvDuration = vrpn_TimevalDiff(time, tvFirst);
            double dDuration = vrpn_TimevalMsecs(tvDuration) / 1000.0;
            printf("Duration: %ld:%ld\n", tvDuration.tv_sec, static_cast<long>(tvDuration.tv_usec));
            printf("%d enties over %gs = %.3fHz\n",
                cEntries, dDuration, cEntries/dDuration);
        } else {
            printf("EOF\n");
        }
        close(file);
        break;
    }
    cEntries++;

    len = ntohl(header.payload_len);
    time.tv_sec = ntohl(header.msg_time.tv_sec);
    time.tv_usec = ntohl(header.msg_time.tv_usec);
    sender = ntohl(header.sender);
    type = ntohl(header.type);
    
    if (summary_mode) {
        static int first = 1;
        if (first) {
            printf("First timestamp in file: %ld:%ld\n", time.tv_sec, static_cast<long>(time.tv_usec));
            tvFirst = time;
            first = 0;
        }
    }

    if (!summary_mode) {
        if (name_mode)
          printf("%ld from %ld, payload length %d\n",
                  type, sender, len);
        else
          printf("Message type %ld, sender %ld, payload length %d\n",
                type, sender, len);
    }

    retval = read(file, buffer, len);
    if (retval < 0) { printf("ERROR\n"); close(file); exit(0); }
    if (!retval) { printf("EOF\n"); close(file); exit(0); }

    if (summary_mode) {
        continue;
    }

    printf(" <%d bytes> at %ld:%ld\n", retval, time.tv_sec, static_cast<long>(time.tv_usec));

    switch (type) {

      case vrpn_CONNECTION_SENDER_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	    printf(" The name of sender #%ld is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_TYPE_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	    printf(" The name of type #%ld is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_UDP_DESCRIPTION:
        buffer[len] = 0;
        printf(" UDP host is \"%s\", port %ld.\n", buffer, sender);
        break;

      case vrpn_CONNECTION_LOG_DESCRIPTION:
        buffer[len] = 0;
        printf(" Log to file \"%s\".\n", buffer);
        break;
    }
  }

  return 0;
}

