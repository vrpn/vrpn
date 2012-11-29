#include <fcntl.h>                      // for open, O_RDONLY
#ifndef _WIN32
#include <netinet/in.h>                 // for ntohl
#include <unistd.h>                     // for close, read
#endif
#include <stdio.h>                      // for printf, fprintf, stderr
#include <stdlib.h>                     // for exit
#include <vrpn_Connection.h>            // for vrpn_HANDLERPARAM, etc

#include "vrpn_Shared.h"                // for timeval

const int buflen = 8000;

void Usage (const char * name) {
  fprintf(stderr, "Usage:  %s <filename>\n", name);
}


int main (int argc, char ** argv) {

  char buffer [buflen];
  vrpn_HANDLERPARAM header;
  int file;
  int retval;

  if (argc != 2) {
    Usage(argv[0]);
    exit(0);
  }

  file = open(argv[1], O_RDONLY);
  if (file == -1) {
    fprintf(stderr, "Couldn't open \"%s\".\n", argv[1]);
    exit(0);
  }

  while (1) {

    int len;
    struct timeval time;
    long sender;
    long type;
    int len2;

    retval = read(file, &header, sizeof(header));
    if (retval < 0) { printf("ERROR\n"); close(file); exit(0); }
    if (!retval) { printf("EOF\n"); close(file); exit(0); }

    len = ntohl(header.payload_len);
    time.tv_sec = ntohl(header.msg_time.tv_sec);
    time.tv_usec = ntohl(header.msg_time.tv_usec);
    sender = ntohl(header.sender);
    type = ntohl(header.type);

    //printf("Message type %d, sender %d, payload length %d\n",
            //type, sender, len);

    retval = read(file, buffer, len);
    if (retval < 0) { printf("ERROR\n"); close(file); exit(0); }
    if (!retval) { printf("EOF\n"); close(file); exit(0); }

    //printf(" <%d bytes>\n", retval);

    switch (type) {

      case vrpn_CONNECTION_SENDER_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	printf(" The name of sender #%ld is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_TYPE_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	//printf(" The name of type #%d is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_UDP_DESCRIPTION:
        buffer[len] = 0;
        //printf(" UDP host is \"%s\", port %d.\n", buffer, sender);
        break;

      case vrpn_CONNECTION_LOG_DESCRIPTION:
        buffer[len] = 0;
        //printf(" Log to file \"%s\".\n", buffer);
        break;

    }
  }
}

