#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <strings.h>
#ifndef hpux
#include <sysent.h>
#endif
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


#ifdef	sgi
#include <bstring.h>
#endif

#include "vrpn_Connection.h"
#include "vrpn_Shared.h"		// defines gettimeofday for WIN32

//#define VERBOSE
//#define	VERBOSE3
//#define	VERBOSE2
//#define	PRINT_READ_HISTOGRAM

// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#ifndef	_WIN32
#define	INVALID_SOCKET	-1
#endif

extern "C" int sdi_disconnect_from_device(int device);
extern "C" int sdi_connect_to_client(char *machine, int port);
#ifndef _WIN32
extern "C" int sdi_connect_to_device(char *s);
extern "C" int sdi_noint_block_read(int file, char *buf, int len);
extern "C" int sdi_noint_block_write(int file, const char *buf, int len);
extern "C" int sdi_noint_block_read_timeout(int filedes, char *buffer,
					int len, struct timeval *timeout);
#else
extern "C" int sdi_noint_block_read(SOCKET outsock, char *buffer, int length);
extern "C" int sdi_noint_block_write(SOCKET insock, char *buffer, int length);
extern "C" int sdi_noint_block_read_timeout(SOCKET tcp_sock, char *buffer,
					int len, struct timeval *timeout);
#endif

const	char	MAGIC[] = "vrpn: ver. 01.00";
const	int	MAGICLEN = 16;	// Must be a multiple of 4 bytes!

// This is the list of states that a connection can be in
// (possible values for status).
#define LISTEN			(1)
#define CONNECTED		(0)
#define CONNECTION_FAIL		(-1)
#define BROKEN			(-2)
#define DROPPED			(-3)

//---------------------------------------------------------------------------
// This routine opens a UDP socket with the requested port number.
// The socket is not bound to any particular outgoing address.
// The routine returns -1 on failure and the file descriptor on success.
// The portno parameter is filled in with the actual port that is opened
// (this is useful when the address INADDR_ANY is used, since it tells
// what port was opened).

static	int open_udp_socket(unsigned short *portno)
{
   int sock;
   struct sockaddr_in server_addr;
   struct sockaddr_in udp_name;
   int	udp_namelen = sizeof(udp_name);

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)  {
	perror("vrpn: vrpn_Connection: can't open socket");
	return -1;
   }

   // bind to local address
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(*portno);
   server_addr.sin_addr.s_addr = htonl((u_long)0); // don't care which client
   if (bind(sock, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0){
	perror("vrpn: vrpn_Connection: can't bind address");
	return -1;
   }

   // Find out which port was actually bound
   if (getsockname(sock,(struct sockaddr*)&udp_name,&udp_namelen)) {
	fprintf(stderr, "vrpn: open_udp_socket: cannot get socket name.\n");
	return -1;
   }
   *portno = ntohs(udp_name.sin_port);

   return sock;
}

//---------------------------------------------------------------------------
//  This routine opens a UDP socket and then connects it to a the specified
// port on the specified machine.
//  The routine returns -1 on failure and the file descriptor on success.

#ifdef _WIN32
static	SOCKET connect_udp_to(char *machine, int portno)
{
   SOCKET sock; 
#else
static	int connect_udp_to(char *machine, int portno)
{
   int sock;
#endif
   struct	sockaddr_in client;     /* The name of the client */
   struct	hostent *host;          /* The host to connect to */

   // create an internet datagram socket
   if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)  {
	perror("vrpn: vrpn_Connection: can't open socket");
	return INVALID_SOCKET;
   }

   // Connect it to the specified port on the specified machine
	client.sin_family = AF_INET;
	if ( (host=gethostbyname(machine)) == NULL ) {
		close(sock);
		fprintf(stderr,
			 "vrpn: connect_udp_to: error finding host by name\n");
		return(INVALID_SOCKET);
	}
#ifdef CRAY
        { int i;
          u_long foo_mark = 0;
          for  (i = 0; i < 4; i++) {
              u_long one_char = host->h_addr_list[0][i];
              foo_mark = (foo_mark << 8) | one_char;
          }
          client.sin_addr.s_addr = foo_mark;
        }
#elif defined(_WIN32)
	memcpy((char*)&(client.sin_addr.s_addr), host->h_addr, host->h_length);
#else
	bcopy(host->h_addr, (char*)&(client.sin_addr.s_addr), host->h_length);
#endif
	client.sin_port = htons(portno);
	if (connect(sock,(struct sockaddr*)&client,sizeof(client)) < 0 ){
		perror("vrpn: connect_udp_to: Could not connect to client");
		close(sock);
		return(INVALID_SOCKET);
	}

   return sock;
}


//---------------------------------------------------------------------------
//  This routine opens a TCP socket and connects it to the machine and port
// that are passed in the msg parameter.  This is a string that contains
// the machine name, a space, then the port number.
//  The routine returns -1 on failure and the file descriptor on success.

int vrpn_Connection::connect_tcp_to(char *msg)
{	char	machine[5000];
	int	port;

	// Find the machine name and port number
	if (sscanf(msg, "%s %d", machine, &port) != 2) {
		return -1;
	}
	// Save the remote machine name in case we need it later
	strncpy(rmachine_name, machine, sizeof(rmachine_name));
	rmachine_name[sizeof(rmachine_name)-1] = '\0';

	// Open the socket
	return sdi_connect_to_client(machine, port);
}

//---------------------------------------------------------------------------
//  This routine checks for a request for attention from a remote machine.
//  The requests come as datagrams to the well-known port number on this
// machine.
//  The datagram includes the machine name and port number on the remote
// machine to which this one should establish a connection.
//  This routine establishes the TCP connection and then sends a magic
// cookie describing this as a VRPN connection and telling which port to
// use for incoming UDP packets of data associated  with the current TCP
// session.  It reads the associated information from the other side and
// sets up the local UDP sender.
//  It then sends descriptions for all of the known packet types.

void vrpn_Connection::check_connection(void)
{  int	request;
   char	msg[200];	// Message received on the request channel
   timeval timeout;
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   // Do a zero-time select() to see if there is an incoming packet on
   // the UDP socket.

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( listen_udp_sock, &f);
   request = select(32, &f, NULL, NULL, &timeout);
   if (request == -1 ) {	// Error in the select()
	perror("vrpn: vrpn_Connection: select failed");
	status = BROKEN;
	return;
   } else if (request != 0) {	// Some data to read!  Go get it.
	if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0, NULL, NULL) == -1) {
		fprintf(stderr,
			"vrpn: Error on recvfrom: Bad connection attempt\n");
		return;
	}

	printf("vrpn: Connection request received: %s\n", msg);
	tcp_sock = connect_tcp_to(msg);
	if (tcp_sock != -1) {
		status = CONNECTED;
	}
   }

   if (status != CONNECTED) {	// Something broke
   	return;
   }

   // Set TCP_NODELAY on the socket
/*XXX It looks like this means something different to Linux than what I
	expect.  I expect it means to send acknowlegements without waiting
	to piggyback on reply.  This causes the serial port to fail.  As
	long as the other end has this set, things seem to work fine.  From
	this end, if we set it it fails (unless we're talking to another
	PC).  Russ Taylor

   {    struct  protoent        *p_entry;
	static  int     nonzero = 1;

	if ( (p_entry = getprotobyname("TCP")) == NULL ) {
		fprintf(stderr,
		  "vrpn: vrpn_Connection: getprotobyname() failed.\n");
		close(tcp_sock); tcp_sock = -1;
		status = BROKEN;
		return;
	}

	if (setsockopt(tcp_sock, p_entry->p_proto,
		TCP_NODELAY, &nonzero, sizeof(nonzero))==-1) {
		perror("vrpn: vrpn_Connection: setsockopt() failed");
		close(tcp_sock); tcp_sock = -1;
		status = BROKEN;
		return;
	}
   }
*/

   // Ignore SIGPIPE, so that the program does not crash when a
   // remote client shuts down its TCP connection.  We'll find out
   // about it as an exception on the socket when we select() for
   // read.
#ifndef _WIN32
   signal( SIGPIPE, SIG_IGN );
#endif

   // Set up the things that need to happen when a new connection is
   // started.
   if (setup_for_new_connection()) {
	fprintf(stderr,"vrpn_Connection: Can't set up new connection!\n");
	drop_connection();
	return;
   }

}

int vrpn_Connection::setup_for_new_connection(void)
{
	char	buf[17];
	int	i;
	unsigned short udp_portnum;

	// No senders or types yet defined by the other side.
	num_other_senders = 0;
	num_other_types = 0;

	// No characters to send on any port
	tcp_num_out = 0;
	udp_num_out = 0;

	// No UDP outbound is defined
	udp_outbound = INVALID_SOCKET;

	// Set all of the local IDs to zero, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).
	for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
		other_senders[i].local_id = -1;
	}
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		other_types[i].local_id = -1;
	}

	// Write the magic cookie header to the server
	if (sdi_noint_block_write(tcp_sock,MAGIC,MAGICLEN) != MAGICLEN) {
	  perror(
	    "vrpn_Connection::setup_for_new_connection: Can't write cookie");
	  return -1;
	}

	// Read the magic cookie from the server
	if (sdi_noint_block_read(tcp_sock,buf,MAGICLEN) != MAGICLEN) {
	  perror(
	    "vrpn_Connection::setup_for_new_connection: Can't read cookie");
	  return -1;
	}
	buf[MAGICLEN] = '\0';
	if (strncmp(buf,MAGIC,MAGICLEN) != 0) {
	  fprintf(stderr,"vrpn_Connection::setup_for_new_connection: bad cookie (wanted '%s', got '%s'\n", MAGIC, buf);
	  return -1;
	}

	// Open the UDP port to accept time-critical messages on.
	udp_portnum = (unsigned short)INADDR_ANY;
	if ((udp_inbound = open_udp_socket(&udp_portnum)) == -1)  {
	  fprintf(stderr,
	  "vrpn_Connection::setup_for_new_connection: can't open UDP socket\n");
	  return -1;
	}

	// Tell the other side what port number to send its UDP messages to.
	if (pack_udp_description(udp_portnum) == -1) {
	  fprintf(stderr,
	    "vrpn_Connection::setup_for_new_connection: Can't pack UDP msg\n");
	  return -1;
	}

	// Pack messages that describe the types of messages and sender
	// ID mappings that have been described to this connection.  These
	// messages use special IDs (negative ones).
	for (i = 0; i < num_my_senders; i++) {
		pack_sender_description(i);
	}
	for (i = 0; i < num_my_types; i++) {
		pack_type_description(i);
	}

	// Send the messages
	if (send_pending_reports() == -1) {
	  fprintf(stderr,
	    "vrpn_Connection::setup_for_new_connection: Can't send UDP msg\n");
	  return -1;
	}

	return 0;
}


int vrpn_Connection::pack_type_description(int which)
{
   struct timeval now;
   long	len = strlen(my_types[which].name);
   char	buffer[sizeof(len)+sizeof(cName)];

   // Pack a message with type vrpn_CONNECTION_TYPE_DESCRIPTION
   // whose sender ID is the ID of the type that is being
   // described and whose body contains the length of the name
   // and then the name of the type.

#ifdef	VERBOSE
   printf("  vrpn_Connection: Packing type '%s'\n",my_types[which].name);
#endif
   memcpy(buffer, &len, sizeof(len));
   memcpy(&buffer[sizeof(len)], my_types[which].name, (int)len);
   gettimeofday(&now,NULL);
   return pack_message((int)(len+sizeof(len)), now,
   	vrpn_CONNECTION_TYPE_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE);
}

int vrpn_Connection::pack_sender_description(int which)
{
   struct timeval now;
   long	len = strlen(my_senders[which]);
   char	buffer[sizeof(len)+sizeof(cName)];

   // Pack a message with type vrpn_CONNECTION_SENDER_DESCRIPTION
   // whose sender ID is the ID of the sender that is being
   // described and whose body contains the length of the name
   // and then the name of the sender.

#ifdef	VERBOSE
	printf(
		"  vrpn_Connection: Packing sender '%s'\n",my_senders[which]);
#endif
   memcpy(buffer, &len, sizeof(len));
   memcpy(&buffer[sizeof(len)], my_senders[which], (int)len);
   gettimeofday(&now,NULL);
   return pack_message((int)(len+sizeof(len)), now,
   	vrpn_CONNECTION_SENDER_DESCRIPTION, which, buffer,
	vrpn_CONNECTION_RELIABLE);
}

int vrpn_Connection::pack_udp_description(int portno)
{
   struct timeval now;
   long	portparam = portno;
   char hostname[1000];

   // Find the local host name
   if (gethostname(hostname, sizeof(hostname))) {
	perror("vrpn_Connection::pack_udp_description: can't get host name");
	return -1;
   }

   // Pack a message with type vrpn_CONNECTION_UDP_DESCRIPTION
   // whose sender ID is the ID of the port that is to be
   // used and whose body holds the zero-terminated string
   // name of the host to contact.

#ifdef	VERBOSE
	printf("  vrpn_Connection: Packing UDP %s:%d\n", hostname,
		portno);
#endif
   gettimeofday(&now,NULL);
   return pack_message(strlen(hostname)+1, now, vrpn_CONNECTION_UDP_DESCRIPTION,
	portparam, hostname, vrpn_CONNECTION_RELIABLE);
}

// Get the UDP port description from the other side and connect the
// outgoing UDP socket to that port so we can send lossy but time-
// critical (tracker) messages that way.

int vrpn_Connection::handle_UDP_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	char	rhostname[1000];	// name of remote host
	vrpn_Connection *me = (vrpn_Connection *)userdata;

	// Get the name of the remote host from the buffer (ensure terminated)
	strncpy(rhostname, p.buffer, sizeof(rhostname));
	rhostname[sizeof(rhostname)-1] = '\0';

	// Open the UDP outbound port and connect it to the port on the
	// remote machine.
	// (remember that the sender field holds the UDP port number)
	me->udp_outbound = connect_udp_to(rhostname, (int)p.sender);
	if (me->udp_outbound == -1) {
	    perror("vrpn: vrpn_Connection: Couldn't open outbound UDP link");
	    return -1;
	}
#ifdef	VERBOSE
	printf("  Opened UDP channel to %s:%d\n", rhostname, p.sender);
#endif
	return 0;
}

int vrpn_Connection::pack_message(int len, struct timeval time,
		long type, long sender, char *buffer,
		unsigned long class_of_service)
{
	int	ret;

	// Make sure type is either a system type (-) or a legal user type
	if ( type >= num_my_types ) {
	    printf("vrpn_Connection::pack_message: bad type (%ld)\n",
		type);
	    return -1;
	}

	// If this is not a system message, make sure the sender is legal.
	if (type >= 0) {
	  if ( (sender < 0) || (sender >= num_my_senders) ) {
	    printf("vrpn_Connection::pack_message: bad sender (%ld)\n",
		sender);
	    return -1;
	  }
	}

	// See if there are any local handlers for this message type from
	// this sender.  If so, yank the callbacks.
	if (do_callbacks_for(type, sender, time, len, buffer)) {
		return -1;
	}

	// If we're not connected, nowhere to send the message so just
	// toss it out.
	if (!connected()) { return 0; }

	// If we don't have a UDP outbound channel, send everything TCP
	if (udp_outbound == -1) {
	    ret = marshall_message(tcp_outbuf, sizeof(tcp_outbuf), tcp_num_out,
				   len, time, type, sender, buffer);
	    tcp_num_out += ret;
	    return -(ret==0);
	}

	// Determine the class of service and pass it off to the
	// appropriate service (TCP for reliable, UDP for everything else).
	if (class_of_service & vrpn_CONNECTION_RELIABLE) {
	    ret = marshall_message(tcp_outbuf, sizeof(tcp_outbuf), tcp_num_out,
				   len, time, type, sender, buffer);
	    tcp_num_out += ret;
	    return -(ret==0);
	} else {
	    ret = marshall_message(udp_outbuf, sizeof(udp_outbuf), udp_num_out,
				   len, time, type, sender, buffer);
	    udp_num_out += ret;
	    return -(ret==0);
	}
}

// Marshal the message into the buffer if it will fit.  Return the number
// of characters sent.

int vrpn_Connection::marshall_message(
	char *outbuf,		// Base pointer to the output buffer
	int outbuf_size,	// Total size of the output buffer
	int initial_out,	// How many characters are already in outbuf
	int len,		// Length of the message payload
	struct timeval time,	// Time the message was generated
	long type,		// Type of the message
	long sender,		// Sender of the message
	char *buffer)		// Message payload
{
   int	ceil_len, total_len;
   int	curr_out = initial_out;	// How many out total so far

   // Compute the length of the message plus its padding to make it
   // an even multiple of 4 bytes.
   // Compute the total message length and put it into the
   // message buffer (if we have room for the whole message)
   ceil_len = len; if (len%4) {ceil_len += 4 - len%4;}
   total_len = 5*sizeof(long) + ceil_len;
   if (curr_out + total_len > outbuf_size) {
   	return 0;
   }
   *(unsigned long*)(void*)(&outbuf[curr_out]) = htonl(total_len);
   curr_out+= sizeof(unsigned long);

   // Pack the time (using gettimeofday() format) into the buffer
   // and do network byte ordering.
   *(unsigned long*)(void*)(&outbuf[curr_out]) = htonl(time.tv_sec);
   curr_out += sizeof(unsigned long);
   *(unsigned long*)(void*)(&outbuf[curr_out]) = htonl(time.tv_usec);
   curr_out += sizeof(unsigned long);

   // Pack the sender and type and do network byte-ordering
   *(unsigned long*)(void*)(&outbuf[curr_out]) = htonl(sender);
   curr_out += sizeof(unsigned long);
   *(unsigned long*)(void*)(&outbuf[curr_out]) = htonl(type);
   curr_out += sizeof(unsigned long);

   // Pack the message from the buffer.  Then skip as many characters
   // as needed to make the end of the buffer fall on an even alignment
   // of 4 bytes (the size of floats/longs).
   if (buffer != NULL) {
	memcpy(&outbuf[curr_out], buffer, len);
   }
   curr_out += ceil_len;
#ifdef	VERBOSE
   printf("Marshalled: len %d, ceil_len %d: '",len,ceil_len);
   printf("'\n");
#endif
   return curr_out - initial_out;	// How many extra bytes we sent
}

void vrpn_Connection::drop_connection(void)
{
	//XXX Store name when opening, say which dropped here
	printf("vrpn: Connection dropped\n");
	if (tcp_sock != INVALID_SOCKET) {
		close(tcp_sock);
		tcp_sock = INVALID_SOCKET;
	}
	if (udp_outbound != INVALID_SOCKET) {
		close(udp_outbound);
		udp_outbound = INVALID_SOCKET;
	}
	if (udp_inbound != INVALID_SOCKET) {
		close(udp_inbound);
		udp_inbound = INVALID_SOCKET;
	}
	if (listen_udp_sock != INVALID_SOCKET) {
		status = LISTEN;	// We're able to accept more
	} else {
		status = BROKEN;	// We're unable to accept more
	}
}

int vrpn_Connection::send_pending_reports(void)
{
   int	ret, sent = 0;

   // Do nothing if not connected.
   if (!connected()) {
   	return 0;
   }

   // Check for an exception on the socket.  If there is one, shut it
   // down and go back to listening.
   timeval timeout;
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( tcp_sock, &f);

   int connection = select(32, NULL, NULL, &f, &timeout);
   if (connection != 0) {
	drop_connection();
	return -1;
   }

   // Send all of the messages that have built
   // up in the TCP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
   while (sent < tcp_num_out) {
	ret = send(tcp_sock, &tcp_outbuf[sent], tcp_num_out-sent, 0);
#ifdef	VERBOSE
   printf("TCP Sent %d bytes\n",ret);
#endif
      if (ret == -1) {
	printf("vrpn: TCP send failed\n");
	drop_connection();
	return -1;
      }
      sent += ret;
   }

   // Send all of the messages that have built
   // up in the UDP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.
   if (udp_num_out > 0) {
	ret = send(udp_outbound, udp_outbuf, udp_num_out, 0);
#ifdef	VERBOSE
   printf("UDP Sent %d bytes\n",ret);
#endif 
      if (ret == -1) {
	printf("vrpn: UDP send failed\n");
	drop_connection();
	return -1;
      }
   }

   tcp_num_out = 0;	// Clear the buffer for the next time
   udp_num_out = 0;	// Clear the buffer for the next time
   return 0;
}

int vrpn_Connection::mainloop(void)
{
	int	tcp_messages_read;
	int	udp_messages_read;

#ifdef	VERBOSE2
	printf("vrpn_Connection::mainloop() called (status %d)\n",status);
#endif

   switch (status) {
      case LISTEN:
      	check_connection();
      	break;

      case CONNECTED:

	// Send all pending reports on the way out
      	send_pending_reports();

	// Read incoming messages from the UDP channel
	if (udp_inbound != -1) {
	  if ( (udp_messages_read = handle_udp_messages(udp_inbound)) == -1) {
		fprintf(stderr,
			"vrpn: UDP handling failed, dropping connection\n");
		drop_connection();
		if (status != LISTEN) {
			status = DROPPED;
			return -1;
		}
		break;
	  }
#ifdef VERBOSE3
	  if(udp_messages_read != 0 )
	    printf("udp message read = %d\n",udp_messages_read);
#endif
	}

	// Read incoming messages from the TCP channel
	if ( (tcp_messages_read = handle_tcp_messages(tcp_sock)) == -1) {
		printf(
			"vrpn: TCP handling failed, dropping connection\n");
		drop_connection();
		if (status != LISTEN) {
			status = DROPPED;
			return -1;
		}
		break;
	}
#ifdef VERBOSE3
	else {
	  if(tcp_messages_read!=0)
		printf("tcp_message_read %d bytes\n",tcp_messages_read);
	}
#endif
#ifdef	PRINT_READ_HISTOGRAM
#define      HISTSIZE 25
   {
        static long count = 0;
        static int tcp_histogram[HISTSIZE+1];
        static int udp_histogram[HISTSIZE+1];
        count++;

        if (tcp_messages_read > HISTSIZE) {tcp_histogram[HISTSIZE]++;}
        else {tcp_histogram[tcp_messages_read]++;};

        if (udp_messages_read > HISTSIZE) {udp_histogram[HISTSIZE]++;}
        else {udp_histogram[udp_messages_read]++;};

        if (count == 3000L) {
		int i;
                count = 0;
		printf("\nHisto (tcp): ");
                for (i = 0; i < HISTSIZE+1; i++) {
                        printf("%d ",tcp_histogram[i]);
                        tcp_histogram[i] = 0;
                }
                printf("\n");
		printf("      (udp): ");
                for (i = 0; i < HISTSIZE+1; i++) {
                        printf("%d ",udp_histogram[i]);
                        udp_histogram[i] = 0;
                }
                printf("\n");
        }
   }
#endif
      	break;

      case CONNECTION_FAIL:
      case BROKEN:
      	fprintf(stderr, "vrpn: Socket failure.  Giving up!\n");
	status = DROPPED;
      	return -1;

      case DROPPED:
      	break;
   }

   return 0;
}

void	vrpn_Connection::init(void)
{
	int	i;

	num_my_senders = 0;
	num_my_types = 0;

	num_other_senders = 0;
	num_other_types = 0;

	tcp_num_out = 0;
	udp_num_out = 0;

	listen_udp_sock = INVALID_SOCKET;
	tcp_sock = INVALID_SOCKET;
	udp_inbound = INVALID_SOCKET;
	udp_outbound = INVALID_SOCKET;

	// Set up the default handlers for the different types of user
	// incoming messages.  Initially, they are all set to the
	// NULL handler, which tosses them out.
	// Note that system messages always have negative indices.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		my_types[i].who_cares = NULL;
	}

	// Set up the handlers for the system messages.  Skip any ones
	// that we don't know how to handle.
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		system_messages[i] = NULL;
	}
	system_messages[-vrpn_CONNECTION_SENDER_DESCRIPTION] =
		handle_sender_message;
	system_messages[-vrpn_CONNECTION_TYPE_DESCRIPTION] =
		handle_type_message;
	system_messages[-vrpn_CONNECTION_UDP_DESCRIPTION] =
		handle_UDP_message;
}

vrpn_Connection::vrpn_Connection(unsigned short listen_port_no)
{
   // Initialize the things that must be for any constructor
   init();

   if ( (listen_udp_sock=open_udp_socket(&listen_port_no)) == INVALID_SOCKET) {
      status = BROKEN;
   } else {
      status = LISTEN;
   printf("vrpn: Listening for requests on port %d\n",listen_port_no);
   }
}

#ifndef _WIN32
vrpn_Connection::vrpn_Connection(char *station_name)
{
	// Initialize the things that must be for any constructor
	init();

	// Open a connection to the station using SDI
	tcp_sock = sdi_connect_to_device(station_name);
	if (tcp_sock < 0) {
	    fprintf(stderr,
		"vrpn: vrpn_Connection::vrpn_Connection(): Can't open %s\n",
		station_name);
		status = BROKEN;
		return;
	}
	status = 0;

	// Set up the things that need to happen when a new connection is
	// established.
	if (setup_for_new_connection()) {
	    fprintf(stderr,"vrpn_Connection: Can't setup new connection!\n");
	    drop_connection();
	    return;
	}

}
#endif

long vrpn_Connection::register_sender(char *name)
{
   int	i;

   // See if the name is already in the list.  If so, return it.
   for (i = 0; i < num_my_senders; i++) {
      if (strcmp(my_senders[i],name) == 0) {
      	return i;
      }
   }

   // See if there are too many on the list.  If so, return -1.
   if (num_my_senders == vrpn_CONNECTION_MAX_SENDERS) {
   	fprintf(stderr,"vrpn: vrpn_Connection::register_sender: Too many!\n");
   	return -1;
   }

   // Add this one into the list
   strncpy(my_senders[num_my_senders], name, sizeof(cName)-1);
   num_my_senders++;

   // If we're connected, pack the sender description
   if (connected()) {
   	pack_sender_description(num_my_senders-1);
   }

   // If the other side has declared this sender, establish the
   // mapping for it.
   for (i = 0; i < num_other_senders; i++) {
	if (strcmp(other_senders[i].name,name) == 0) {
#ifdef	VERBOSE
	printf("  ...mapping from other-side sender %d\n", i);
#endif
		other_senders[i].local_id = num_my_senders-1;
		break;
	}
   }

   // One more in place -- return its index
   return num_my_senders-1;
}

long vrpn_Connection::register_message_type(char *name)
{
	int	i;

	// See if the name is already in the list.  If so, return it.
	for (i = 0; i < num_my_types; i++) {
		if (strcmp(my_types[i].name,name) == 0) {
			return i;
		}
	}

	// See if there are too many on the list.  If so, return -1.
	if (num_my_types == vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,
		  "vrpn: vrpn_Connection::register_message_type: Too many!\n");
		return -1;
	}

	// Add this one into the list and return its index
	strncpy(my_types[num_my_types].name, name, sizeof(cName)-1);
	my_types[num_my_types].who_cares = NULL;
	num_my_types++;

	// If we're connected, pack the type description
	if (connected()) {
		pack_type_description(num_my_types-1);
	}

	// If the other side has declared this type, establish the
	// mapping for it.  If not, make sure there isn't one.
	for (i = 0; i < num_other_types; i++) {
		if (strcmp(other_types[i].name,name) == 0) {
			other_types[i].local_id = num_my_types-1;
#ifdef	VERBOSE
	printf("  ...mapping from other-side type %d\n", i);
#endif
			break;
		}
	}

	// One more in place -- return its index
	return num_my_types-1;
}

int	vrpn_Connection::connected(void) { return status == CONNECTED; };

// Yank the callback chain for a message type.  Call all handlers that
// are interested in messages from this sender.  Return 0 if they all
// return 0, -1 otherwise.

int	vrpn_Connection::do_callbacks_for(long type, long sender,
		struct timeval time, int payload_len, char *buf)
{
	// Make sure we have a non-negative type.  System messages are
	// handled differently.
	if (type < 0) {
		return 0;
	}

	// Find the head for the list of callbacks to call
	vrpnMsgCallbackEntry *who = my_types[type].who_cares;

	// Fill in the parameter to be passed to the routines
	vrpn_HANDLERPARAM p;
	p.type = type;
	p.sender = sender;
	p.msg_time = time;
	p.payload_len = payload_len;
	p.buffer = buf;

	while (who != NULL) {	// For each callback entry

		// Verify that the sender is ANY or matches
		if ( (who->sender == vrpn_ANY_SENDER) ||
		     (who->sender == sender) ) {
			if (who->handler(who->userdata, p)) {
				fprintf(stderr, "vrpn: vrpn_Connection::do_callbacks_for: Nonzero user handler return\n");
				return -1;
			}
		}

		// Next callback in list
		who = who->next;
	}

	return 0;
}

// Read all messages available on the given file descriptor (a TCP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.

int	vrpn_Connection::handle_tcp_messages(int fd)
{	int	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval timeout;
	int	num_messages_read = 0;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_tcp_messages() called\n");
#endif
	// Read incoming messages until there are no more characters to
	// read from the other side.  For each message, determine what
	// type it is and then pass it off to the appropriate handler
	// routine.

	do {
	  // Select to see if ready to hear from other side, or exception
	  FD_ZERO(&readfds);              /* Clear the descriptor sets */
	  FD_ZERO(&exceptfds);
	  FD_SET(fd, &readfds);     /* Check for read */
	  FD_SET(fd, &exceptfds);   /* Check for exceptions */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	  sel_ret = select(32,&readfds,NULL,&exceptfds, &timeout);
	  if (sel_ret == -1) {
	    if (errno == EINTR) { /* Ignore interrupt */
		continue;
	    } else {
		perror(
		  "vrpn: vrpn_Connection::handle_tcp_messages: select failed");
		return(-1);
	    }
	  }

	  // See if exceptional condition on socket
	  if (FD_ISSET(fd, &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Exception on socket\n");
		return(-1);
	  }

	  // If there is anything to read, get the next message
	  if (FD_ISSET(fd, &readfds)) {
		int	header[5];
		struct timeval time;
		int	sender, type;
		int	len, payload_len, ceil_len;
		static	int	buflen = 0;
		static	char	*buf = NULL;
#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_tcp_messages() something to read\n");
#endif

		// Read and parse the header
		if (sdi_noint_block_read(fd,(char*)header,sizeof(header)) !=
			sizeof(header)) {
		  //perror("vrpn: vrpn_Connection::handle_tcp_messages: Can't read header");
		  printf("vrpn:vrpn_connection: handle_tcp_messages: Can't read header");
			return -1;
		}
		len = htonl(header[0]);
		time.tv_sec = htonl(header[1]);
		time.tv_usec = htonl(header[2]);
		sender = htonl(header[3]);
		type = htonl(header[4]);

		// Figure out how long the message body is, and how long it
		// is including any padding to make sure that it is a
		// multiple of four bytes long.
		payload_len = len - sizeof(header);
		ceil_len = payload_len;
		if (ceil_len % 4) { ceil_len += 4 - ceil_len%4; }

		// Make sure the buffer is long enough to hold the whole
		// message body.
		if (buflen < ceil_len) {
		  if (buf) { buf = (char*)realloc(buf,ceil_len); }
		  else { buf = (char*)malloc(ceil_len); }
		  if (buf == NULL) {
		     fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Out of memory\n");
		     return -1;
		  }
		  buflen = ceil_len;
		}

		// Read the body of the message 
		if (sdi_noint_block_read(fd,buf,ceil_len) != ceil_len) {
		 perror("vrpn: vrpn_Connection::handle_tcp_messages: Can't read body");
		 return -1;
		}

		// Call the handler(s) for this message type
		// If one returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id
		 if (other_types[type].local_id >= 0) {	// Has one been set?
		  if (do_callbacks_for(other_types[type].local_id,
				       other_senders[sender].local_id,
				       time, payload_len, buf)) {
			return -1;
		  }
		 }
		} else {	// Call system handler if there is one

		 if (system_messages[-type] != NULL) {

		  // Fill in the parameter to be passed to the routines
		  vrpn_HANDLERPARAM p;
		  p.type = type;
		  p.sender = sender;
		  p.msg_time = time;
		  p.payload_len = payload_len;
		  p.buffer = buf;

		  if (system_messages[-type](this, p)) {
			    fprintf(stderr, "vrpn: vrpn_Connection::handle_tcp_messages: Nonzero system handler return\n");
			    return -1;
		  }
		 }

		}

		// Got one more message
		num_messages_read++;
	  }

	} while (sel_ret);

	return num_messages_read;
}

// Read all messages available on the given file descriptor (a UDP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.
// XXX At some point, the common parts of this routine and the TCP read
// routine should be merged.  Using read() on the UDP socket fails, so
// we need the UDP version.  If we use the UDP version for the TCP code,
// it hangs when we the client drops its connection, so we need the
// TCP code as well.

int	vrpn_Connection::handle_udp_messages(int fd)
{	int	sel_ret;
        fd_set  readfds, exceptfds;
        struct  timeval timeout;
	int	num_messages_read = 0;
	static	char	inbuf[vrpn_CONNECTION_UDP_BUFLEN];
	int	inbuf_len;
	char	*inbuf_ptr;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_udp_messages() called\n");
#endif

	// Read incoming messages until there are no more packets to
	// read from the other side.  Each packet may have more than one
	// message in it.  For each message, determine what
	// type it is and then pass it off to the appropriate handler
	// routine.

	do {
	  // Select to see if ready to hear from server, or exception
	  FD_ZERO(&readfds);              /* Clear the descriptor sets */
	  FD_ZERO(&exceptfds);
	  FD_SET(fd, &readfds);     /* Check for read */
	  FD_SET(fd, &exceptfds);   /* Check for exceptions */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	  sel_ret = select(32,&readfds,NULL,&exceptfds, &timeout);
	  if (sel_ret == -1) {
	    if (errno == EINTR) { /* Ignore interrupt */
		continue;
	    } else {
		perror("vrpn: vrpn_Connection::handle_udp_messages: select failed()");
		return(-1);
	    }
	  }

	  // See if exceptional condition on socket
	  if (FD_ISSET(fd, &exceptfds)) {
		fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Exception on socket\n");
		return(-1);
	  }

	  // If there is anything to read, get the next message
	  if (FD_ISSET(fd, &readfds)) {
		int	header[5];
		struct timeval time;
		int	sender, type;
		int	len, payload_len, ceil_len;

#ifdef	VERBOSE2
	printf("vrpn_Connection::handle_udp_messages() something to read\n");
#endif
		// Read the buffer and reset the buffer pointer
		inbuf_ptr = inbuf;
		if ( (inbuf_len = recv(fd, inbuf, sizeof(inbuf), 0)) == -1) {
		   perror("vrpn: vrpn_Connection::handle_udp_messages: recv() failed");
		   return -1;
		}

	      // Parse each message in the buffer
	      while ( (inbuf_ptr - inbuf) != inbuf_len) {

		// Read and parse the header
		if ( ((inbuf_ptr - inbuf) + sizeof(header)) > (unsigned)inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Can't read header");
		   return -1;
		}
		memcpy(header, inbuf_ptr, sizeof(header));
		inbuf_ptr += sizeof(header);
		len = htonl(header[0]);
		time.tv_sec = htonl(header[1]);
		time.tv_usec = htonl(header[2]);
		sender = htonl(header[3]);
		type = htonl(header[4]);

#ifdef VERBOSE
		printf("Message type %ld (local type %ld), sender %ld received\n",
			type,other_types[type].local_id,sender);
#endif
		
		// Figure out how long the message body is, and how long it
		// is including any padding to make sure that it is a
		// multiple of four bytes long.
		payload_len = len - sizeof(header);
		ceil_len = payload_len;
		if (ceil_len % 4) { ceil_len += 4 - ceil_len%4; }

		// Make sure we received enough to cover the entire payload
		if ( ((inbuf_ptr - inbuf) + ceil_len) > inbuf_len) {
		   fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Can't read payload");
		   return -1;
		}

		// Call the handler for this message type
		// If it returns nonzero, return an error.
		if (type >= 0) {	// User handler, map to local id
		 if (other_types[type].local_id >= 0) {	// Has one been set?
		  if (do_callbacks_for(other_types[type].local_id,
				       other_senders[sender].local_id,
				       time, payload_len, inbuf_ptr)) {
			return -1;
		  }
		 }
		} else {	// System handler

		  // Fill in the parameter to be passed to the routines
		  vrpn_HANDLERPARAM p;
		  p.type = type;
		  p.sender = sender;
		  p.msg_time = time;
		  p.payload_len = payload_len;
		  p.buffer = inbuf_ptr;

		  if (system_messages[-type](this, p)) {
		    fprintf(stderr, "vrpn: vrpn_Connection::handle_udp_messages: Nonzero system return\n");
		    return -1;
		  }
		}
		inbuf_ptr += ceil_len;

		// Got one more message
		num_messages_read++;
	    }
	  }

	} while (sel_ret);

	return num_messages_read;
}

int	vrpn_Connection::handle_type_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_Connection *me = (vrpn_Connection*)userdata;
	cName	type_name;
	int	i;

	if (p.payload_len > sizeof(type_name)) {
		fprintf(stderr,"vrpn: vrpn_Connection::Type name too long\n");
		return -1;
	}

	// Find out the name of the type (skip the length)
	strncpy(type_name, p.buffer+4, p.payload_len-4);
	type_name[p.payload_len-4] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side type: '%s'\n", type_name);
#endif

	// The assumption is that the type has not been defined before.
	// Add this as a new type to the list of those defined
	if (me->num_other_types >= vrpn_CONNECTION_MAX_TYPES) {
		fprintf(stderr,"vrpn: Too many types from other side\n");
		return -1;
	}
#ifdef _WIN32
	memcpy(me->other_types[me->num_other_types].name, type_name, 
		 sizeof(type_name));
#else
	bcopy(type_name, me->other_types[me->num_other_types].name,
		sizeof(type_name));
#endif

	// If there is a corresponding local type defined, set the mapping.
	// If not, clear the mapping.
	me->other_types[me->num_other_types].local_id = -1; // None yet
	for (i = 0; i < me->num_my_types; i++) {
		if (strcmp(type_name, me->my_types[i].name) == 0) {
			me->other_types[me->num_other_types].local_id = i;
#ifdef	VERBOSE
	printf("  ...mapping to this-side type %d\n", i);
#endif
			break;
		}
	}

	// One more local type
	me->num_other_types++;

	return 0;
}

int	vrpn_Connection::handle_sender_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
	vrpn_Connection *me = (vrpn_Connection*)userdata;
	cName	sender_name;
	int	i;

	if (p.payload_len > sizeof(sender_name)) {
		fprintf(stderr,"vrpn: vrpn_Connection::Type name too long\n");
		return -1;
	}

	// Find out the name of the sender (skip the length)
	strncpy(sender_name, p.buffer+4, p.payload_len-4);
	sender_name[p.payload_len-4] = '\0';

#ifdef	VERBOSE
	printf("Registering other-side sender: '%s'\n", sender_name);
#endif

	// The assumption is that the sender has not been defined before.
	// Add this as a new sender to the list of those defined
	if (me->num_other_senders >= vrpn_CONNECTION_MAX_SENDERS) {
		fprintf(stderr,"vrpn: Too many senders from other side\n");
		return -1;
	}
#ifdef _WIN32
	memcpy(me->other_senders[me->num_other_senders].name, sender_name, 
		sizeof(sender_name));
#else
	bcopy(sender_name, me->other_senders[me->num_other_senders].name,
		sizeof(sender_name));
#endif

	// If there is a corresponding local sender defined, set the mapping.
	// If not, set it to -1.
	me->other_senders[me->num_other_senders].local_id = -1; // None yet
	for (i = 0; i < me->num_my_senders; i++) {
		if (strcmp(sender_name, me->my_senders[i]) == 0) {
			me->other_senders[me->num_other_senders].local_id = i;
#ifdef	VERBOSE
	printf("  ...mapping to this-side sender %d\n", i);
#endif
			break;
		}
	}

	// One more local sender
	me->num_other_senders++;

	return 0;
}

int vrpn_Connection::register_handler(long type, vrpn_MESSAGEHANDLER handler,
                        void *userdata, long sender)
{
	vrpnMsgCallbackEntry	*new_entry;

	// Ensure that the type is a valid one (one that has been defined)
	if ( (type < 0) || (type >= num_my_types) ) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: No such type\n");
		return -1;
	}

	// Ensure that the sender is a valid one (or "any")
	if ( (sender != vrpn_ANY_SENDER) &&
	     ( (sender < 0) || (sender >= num_my_senders) ) ) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: No such sender\n");
		return -1;
	}

	// Ensure that the handler is non-NULL
	if (handler == NULL) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: NULL handler\n");
		return -1;
	}

	// Allocate and initialize the new entry
	if ( (new_entry = new vrpnMsgCallbackEntry) == NULL) {
		fprintf(stderr,
			"vrpn_Connection::register_handler: Out of memory\n");
		return -1;
	}
	new_entry->handler = handler;
	new_entry->userdata = userdata;
	new_entry->sender = sender;

	// Add this handler to the chain at the beginning (don't check to see
	// if it is already there, since duplication is okay).
#ifdef	VERBOSE
	printf("Adding user handler for type %ld, sender %ld\n",type,sender);
#endif
	new_entry->next = my_types[type].who_cares;
	my_types[type].who_cares = new_entry;

	return 0;
}

int vrpn_Connection::unregister_handler(long type, vrpn_MESSAGEHANDLER handler,
                        void *userdata, long sender)
{
	// The pointer at *snitch points to victim
	vrpnMsgCallbackEntry	*victim, **snitch;

	// Ensure that the type is a valid one (one that has been defined)
	if ( (type < 0) || (type >= num_my_types) ) {
		fprintf(stderr,
			"vrpn_Connection::unregister_handler: No such type\n");
		return -1;
	}

	// Find a handler with this registry in the list (any one will do,
	// since all duplicates are the same).
	snitch = &(my_types[type].who_cares);
	victim = *snitch;
	while ( (victim != NULL) &&
		( (victim->handler != handler) ||
		  (victim->userdata != userdata) ||
		  (victim->sender != sender) )){
	    snitch = &( (*snitch)->next );
	    victim = victim->next;
	}

	// Make sure we found one
	if (victim == NULL) {
		fprintf(stderr,
		    "vrpn_Connection::unregister_handler: No such handler\n");
		return -1;
	}

	// Remove the entry from the list
	*snitch = victim->next;
	delete victim;

	return 0;
}

//------------------------------------------------------------------------
//	This section holds data structures and functions to open
// connections by name.
//	The intention of this section is that it can open connections for
// objects that are in different libraries (trackers, buttons and sound),
// even if they all refer to the same connection.

// List of connections that are already open
typedef	struct vrpn_KNOWN_CONS_STRUCT {
	char	name[1000];	// The name of the connection
	vrpn_Connection	*c;	// The connection
	struct vrpn_KNOWN_CONS_STRUCT *next;	// Next on the list
} vrpn_KNOWN_CONNECTION;
static vrpn_KNOWN_CONNECTION *known = NULL;

//	This routine will return a pointer to the connection whose name
// is passed in.  If the routine is called multiple times with the same
// name, it will return the same pointer, rather than trying to make
// multiple of the same connection.  If the connection is in a bad way,
// it returns NULL.
//	This routine will strip off any part of the string before and
// including the '@' character, considering this to be the local part
// of a device name, rather than part of the connection name.  This allows
// the opening of a connection to "Tracker0@ioglab" for example, which will
// open a connection to ioglab.
#ifndef _WIN32
vrpn_Connection *vrpn_get_connection_by_name(char *cname)
{
	vrpn_KNOWN_CONNECTION	*curr = known;
	char			*where_at;	// Part of name past last '@'

	if (cname == NULL) {
		fprintf(stderr,"vrpn_get_connection_by_name(): NULL name\n");
		return NULL;
	}

	// Find the relevant part of the name (skip past last '@'
	// if there is one)
	if ( (where_at = strrchr(cname, '@')) != NULL) {
		cname = where_at+1;	// Chop off the front of the name
	}

	// See if the connection is one that is already open.
	// Leave curr pointing to it if so, or to NULL if not.
	while ( (curr != NULL) && (strcmp(cname, curr->name) != 0)) {
		curr = curr->next;
	}

	// If its not already open, open it and add it to the list.
	if (curr == NULL) {
		if ( (curr = new(vrpn_KNOWN_CONNECTION)) == NULL) {
			fprintf(stderr,"vrpn_get_connection: Out of memory\n");
			return NULL;
		}
		strncpy(curr->name, cname, sizeof(curr->name));
		curr->c = new vrpn_Connection(cname);
		curr->next = known;
		known = curr;
	}

	// See if the connection is okay.  If so, return it.  If not, NULL
	if (curr->c->doing_okay()) {
		return curr->c;
	} else {
		return NULL;
	}
}
#endif
