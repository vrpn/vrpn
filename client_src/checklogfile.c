#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <vrpn_Connection.h>  // for vrpn_ALIGN

const int buflen = 8000;

void Usage (const char * name) {
  fprintf(stderr, "Usage:  %s [-n] <filename>\n", name);
  fprintf(stderr,"    -n:  Print names instead of numbers.\n");
}


int main (int argc, char ** argv) {

  char * filename;
  int name_mode = 0;

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
  }

  file = open(filename, O_RDONLY);
  if (file == -1) {
    fprintf(stderr, "Couldn't open \"%s\".\n", filename);
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

    if (name_mode)
      printf("%s from %s, payload length %d\n",
              type, sender, len);
    else
      printf("Message type %ld, sender %ld, payload length %d\n",
            type, sender, len);

    retval = read(file, buffer, len);
    if (retval < 0) { printf("ERROR\n"); close(file); exit(0); }
    if (!retval) { printf("EOF\n"); close(file); exit(0); }

    printf(" <%d bytes> at %ld:%ld\n", retval, time.tv_sec, time.tv_usec);

    switch (type) {

      case vrpn_CONNECTION_SENDER_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	printf(" The name of sender #%d is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_TYPE_DESCRIPTION:
        len2 = ntohl(* ((int *) buffer));
        buffer[len2 + sizeof(int)] = 0;
	printf(" The name of type #%d is \"%s\".\n", sender, buffer + sizeof(int));
        break;

      case vrpn_CONNECTION_UDP_DESCRIPTION:
        buffer[len] = 0;
        printf(" UDP host is \"%s\", port %d.\n", buffer, sender);
        break;

      case vrpn_CONNECTION_LOG_DESCRIPTION:
        buffer[len] = 0;
        printf(" Log to file \"%s\".\n", buffer);
        break;

    }
  }
}

