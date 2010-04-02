// vrpn_print_message.C: Print out information about all the messages
// that come from a VRPN file or connection. Useful for seeing what
// is at a server or in a file.
// It uses the logging feature to get called even for messages that
// it has not registered senders for.

// XXX Does not work on files; nothing gets printed...

// XXX Bad side effect: It produces a file called vrpn_temp.deleteme
// in the current directory.

#include	<math.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<vrpn_Connection.h>

vrpn_Connection		*conn;	// Connection pointer
vrpn_OneConnection	*endp;	// Endpoint pointer

void	Usage(char *s)
{
	fprintf(stderr,"Usage: %s vrpn_connection_name\n", s);
	fprintf(stderr,"      (Note: file:filename can be connection name)\n");
	exit(-1);
}

// This function does the work. It prints out the message type,
// sender, size and time for each message that comes in to the
// connection.

int	msg_handler(void *, vrpn_HANDLERPARAM p)
{
	const char	*sender_name = conn->sender_name(p.sender);
	const char	*type_name = conn->message_type_name(p.type);

	// We'll need to adjust the sender and type if it was
	// unknown.
	if (sender_name == NULL) { sender_name = "UNKNOWN_SENDER"; }
	if (type_name == NULL) { type_name = "UNKNOWN_TYPE"; }
	printf("Time: %ld:%ld, Sender: %s, Type %s, Length %ld\n",
		p.msg_time.tv_sec, p.msg_time.tv_usec,
		sender_name,
		type_name,
		p.payload_len);

	return -1;	// Do not log the message
}

int	main(int argc, char *argv[])
{
	char	*conn_name;	// Name of the connection or file
	int	num_registered_senders = 0;
	int	num_registered_types = 0;

	// Parse the command line
	if (argc != 2) {
		Usage(argv[0]);
	} else {
		conn_name = argv[1];
	}

	// Open the connection
	conn = vrpn_get_connection_by_name(conn_name, "vrpn_temp.deleteme",
		vrpn_LOG_INCOMING);
	if (conn == NULL) {
		fprintf(stderr,"ERROR: Can't get connection %s\n",conn_name);
		return -1;
	}
	endp = conn->endpointPtr();

	// Set up the callback for all message types
	conn->register_log_filter(msg_handler, NULL);

	// Mainloop until they kill us. When new senders or types
	// are registered on the other side, register them locally
	// so that we can print them out reasonably
	while (1) {
		int	i;

		// Register any new senders
		for (i = num_registered_senders; i < endp->num_other_senders;
		     i++) {
			conn->register_sender(endp->other_senders[i].name);
		}
		num_registered_senders = endp->num_other_senders;

		// Register any new types
		for (i = num_registered_types; i < endp->num_other_types;
		     i++) {
			conn->register_message_type(endp->other_types[i].name);
		}
		num_registered_types = endp->num_other_types;

		conn->mainloop();
	}

	return 0;	// Sure!
}

