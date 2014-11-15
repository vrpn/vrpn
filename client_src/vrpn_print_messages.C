// vrpn_print_message.C: Print out information about all the messages
// that come from a VRPN file or connection. Useful for seeing what
// is at a server or in a file.
// It uses the logging feature to get called even for messages that
// it has not registered senders for.

// XXX Does not work on files; nothing gets printed...

// XXX Bad side effect: It produces a file called vrpn_temp.deleteme
// in the current directory.
#include <stdio.h>  // for fprintf, NULL, stderr, etc
#include <stdlib.h> // for exit

#include <vrpn_Configure.h>  // for VRPN_CALLBACK
#include <vrpn_Shared.h>     // for timeval
#include <vrpn_Connection.h> // for vrpn_HANDLERPARAM, etc

vrpn_Connection *conn; // Connection pointer

int Usage(char *s)
{
    fprintf(stderr, "Usage: %s vrpn_connection_name\n", s);
    fprintf(stderr, "      (Note: file:filename can be connection name)\n");
    return -1;
}

// This function does the work. It prints out the message type,
// sender, size and time for each message that comes in to the
// connection.

int VRPN_CALLBACK msg_handler(void *, vrpn_HANDLERPARAM p)
{
    const char *sender_name = conn->sender_name(p.sender);
    const char *type_name = conn->message_type_name(p.type);

    // We'll need to adjust the sender and type if it was
    // unknown.
    if (sender_name == NULL) {
        sender_name = "UNKNOWN_SENDER";
    }
    if (type_name == NULL) {
        type_name = "UNKNOWN_TYPE";
    }
    printf("Time: %ld:%ld, Sender: %s, Type %s, Length %d\n",
           static_cast<long>(p.msg_time.tv_sec),
           static_cast<long>(p.msg_time.tv_usec), sender_name, type_name,
           p.payload_len);

    return -1; // Do not log the message
}

int main(int argc, char *argv[])
{
    char *conn_name; // Name of the connection or file

    // Parse the command line
    if (argc != 2) {
        return Usage(argv[0]);
    }
    else {
        conn_name = argv[1];
    }

    // Open the connection, with a file for incoming log required for some
    // reason.
    // (I think it's so that there is a log that we can filter by displaying it)
    conn = vrpn_get_connection_by_name(conn_name, "vrpn_temp.deleteme");
    if (conn == NULL) {
        fprintf(stderr, "ERROR: Can't get connection %s\n", conn_name);
        return -1;
    }

    // Set up the callback for all message types
    conn->register_log_filter(msg_handler, NULL);

    // Mainloop until they kill us.
    while (1) {
        conn->mainloop();
    }

    return 0; // Sure!
}
