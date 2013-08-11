// test_logging.C
//    This is a VRPN test program that has both clients and servers
// running within the same thread. It is intended to test the logging of
// messages and the replay of those messages from the saved log files
//    The program uses VRPN text send/receive to make the logged messages
// flow from both the client to the server and the server to the client.

#include <stdio.h>                      // for NULL, fprintf, printf, etc
#include <string.h>                     // for strlen

#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday
#include "vrpn_Text.h"                  // for vrpn_Text_Receiver, etc
#ifndef _WIN32
#include <unistd.h>                     // for unlink
#endif

const char  *CLIENT_TEXT_NAME = "Text0";
const char  *SERVER_TEXT_NAME = "Text1";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

// The names of the files to log to.  First the ones that will be
// open the whole time, then the ones that get created on request
// by the client connection.
const char  *SERVER_BASE_INCOMING_LOG = "svr_incoming";
const char  *SERVER_BASE_OUTGOING_LOG = "svr_outgoing";

const char *CLIENT_CLIENT_INCOMING_LOG = "cli_cli_incoming";
const char *CLIENT_CLIENT_OUTGOING_LOG = "cli_cli_outgoing";
const char *CLIENT_SERVER_INCOMING_LOG = "cli_svr_incoming";
const char *CLIENT_SERVER_OUTGOING_LOG = "cli_svr_outgoing";

// The server and client connections
vrpn_Connection		*server_connection = NULL;
vrpn_Connection         *client_connection = NULL;

// The text sender and receiver whose receiver is on the client side.
vrpn_Text_Sender	*server_text_sender = NULL;
vrpn_Text_Receiver	*client_text_receiver = NULL;

// The text sender and receiver whose receiver is on the server side.
vrpn_Text_Sender	*client_text_sender = NULL;
vrpn_Text_Receiver	*server_text_receiver = NULL;

bool  got_text_message = false;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_text (void *, const vrpn_TEXTCB info)
{
  printf("Got text type %d level %d: '%s'\n", info.type, info.level, info.message);
  got_text_message = true;
}

/*****************************************************************************
 *
   Routines to create remotes and link their callback handlers.
 *
 *****************************************************************************/

void	create_and_link_text_remote(void)
{
  client_text_receiver = new vrpn_Text_Receiver (CLIENT_TEXT_NAME, client_connection);
  client_text_receiver->register_message_handler(NULL, handle_text);

  client_text_sender = new vrpn_Text_Sender (SERVER_TEXT_NAME, client_connection);
}

/*****************************************************************************
 *
   Server routines for those devices that rely on application intervention
 *
 *****************************************************************************/

void	send_text_once_per_second(void)
{
  static	long	secs = 0;
  struct	timeval	now;

  vrpn_gettimeofday(&now, NULL);
  if (secs == 0) {	// First time through
    secs = now.tv_sec;
  }
  if ( now.tv_sec - secs >= 1 ) {
    secs = now.tv_sec;
    client_text_sender->send_message("Client message");
    server_text_sender->send_message("Server message");
  }
}

//---------------------------------------------------------------------
// Open a client connection, asking for the specified logging files
// from the server.  Loop for a while to make sure messages get delivered.
// Then delete the client-side objects and connection.

int open_client_connection_and_loop(const char *local_in, const char *local_out,
                                    const char *remote_in, const char *remote_out)
{
  //---------------------------------------------------------------------
  // Open a client connection to the server, asking for the logging files
  // requested in the parameters.
  char name[512];
  sprintf(name, "%s@localhost:%d", CLIENT_TEXT_NAME, CONNECTION_PORT);
  client_connection = vrpn_get_connection_by_name(name, local_in, local_out,
    remote_in, remote_out);
  if (client_connection == NULL) {
    fprintf(stderr,"Cannot open client connection\n");
    return -2;
  }

  //---------------------------------------------------------------------
  // Open the client-side text sender and receivers.
  create_and_link_text_remote();

  //---------------------------------------------------------------------
  // Go for a few seconds to let messages get sent back and forth.

  struct timeval now, start;
  vrpn_gettimeofday(&now, NULL);
  start = now;
  while ( now.tv_sec - start.tv_sec < 3 ) {
    send_text_once_per_second();
    server_connection->mainloop();
    client_connection->mainloop();
    vrpn_gettimeofday(&now, NULL);
  }

  //---------------------------------------------------------------------
  // Delete the client-side objects and get rid of the client connection
  // by reducing its reference count.
  delete client_text_receiver;
  delete client_text_sender;
  client_connection->removeReference();

  return 0;
}

int check_for_messages_in(const char *device_name, const char *file_name)
{
  //---------------------------------------------------------------------
  // Open a text receiver to connect to the specified input file.
  char name[512];
  sprintf(name, "%s@file://%s", device_name, file_name);
  printf("Checking for logs in %s\n", name);
  vrpn_Text_Receiver *r = new vrpn_Text_Receiver(name);
  r->register_message_handler(NULL, handle_text);

  //---------------------------------------------------------------------
  // Clear the text-received flag and then watch the connection for a
  // few seconds to see if we get a message.
  got_text_message = false;
  struct timeval now, start;
  vrpn_gettimeofday(&now, NULL);
  start = now;
  while ( now.tv_sec - start.tv_sec < 3 ) {
    r->mainloop();
    vrpn_gettimeofday(&now, NULL);
  }

  //---------------------------------------------------------------------
  // Delete the receiver, which should also get rid of the connection.
  delete r;

  //---------------------------------------------------------------------
  // Make sure we got a message.
  if (got_text_message) {
    return 0;
  } else {
    return -1;
  }
}

// Server incoming connections get saved to files with a number
// appended, per connection: if the base name is "foo", then the
// files are foo-1 foo-2 fo-3...
char *make_server_incoming_name(const char *base, int which)
{
  char *name = new char[strlen(base) + 30];
  if (name == NULL) {
    return NULL;
  }
  sprintf(name, "%s-%d", base, which);
  return name;
}

int main (int argc, char * argv [])
{
  // Return value is good so far...
  int ret = 0;

  if (argc != 1) {
    fprintf(stderr, "Usage: %s\n", argv[0]);
    return -1;
  }

  //---------------------------------------------------------------------
  // explicitly open the server connection, telling it to log its
  // incoming and outgoing messages even when there is not a logging
  // client connection.
  server_connection = vrpn_create_server_connection(CONNECTION_PORT,
    SERVER_BASE_INCOMING_LOG, SERVER_BASE_OUTGOING_LOG);
  if (server_connection == NULL) {
    fprintf(stderr,"Cannot open server connection\n");
    return -1;
  }

  //---------------------------------------------------------------------
  // Open the server-side text sender and receivers.
  server_text_sender = new vrpn_Text_Sender(CLIENT_TEXT_NAME, server_connection);
  server_text_receiver = new vrpn_Text_Receiver(SERVER_TEXT_NAME, server_connection);
  if ( (server_text_sender == NULL) || (server_text_receiver == NULL) ) {
    fprintf(stderr,"Cannot create text server or client\n");
    return -3;
  }

  //---------------------------------------------------------------------
  // Establish the no-logging connection and make sure it works.
  // (This will put entries into the server-side log files.)
  printf("Sending messages to and from server with base logging only\n");
  if (0 != open_client_connection_and_loop(NULL, NULL, NULL, NULL)) {
    fprintf(stderr,"Could not test no-logging connection\n");
    ret = -4;
  }

  //---------------------------------------------------------------------
  // Establish the client-in-logging connection and make sure it works.
  // (This will put entries into the server-side log files.)
  printf("Sending messages to and from server with client-in\n");
  if (0 != open_client_connection_and_loop(CLIENT_CLIENT_INCOMING_LOG, NULL, NULL, NULL)) {
    fprintf(stderr,"Could not test CLIENT_CLIENT_INCOMING_LOG connection\n");
    ret = -4;
  }

  //---------------------------------------------------------------------
  // Establish the client-out-logging connection and make sure it works.
  // (This will put entries into the server-side log files.)
  printf("Sending messages to and from server with client-out\n");
  if (0 != open_client_connection_and_loop(NULL, CLIENT_CLIENT_OUTGOING_LOG, NULL, NULL)) {
    fprintf(stderr,"Could not test CLIENT_CLIENT_OUTGOING_LOG connection\n");
    ret = -4;
  }

  //---------------------------------------------------------------------
  // Establish the server-in-logging connection and make sure it works.
  // Actually, this has to be done below in a separate step because the
  // server has been set to automatically log incoming connections.

  //---------------------------------------------------------------------
  // Establish the server-out-logging connection and make sure it works.
  // (This will put entries into the server-side log files.)
  printf("Sending messages to and from server with server-out\n");
  if (0 != open_client_connection_and_loop(NULL, NULL, NULL, CLIENT_SERVER_OUTGOING_LOG)) {
    fprintf(stderr,"Could not test CLIENT_SERVER_OUTGOING_LOG connection\n");
    ret = -4;
  }

  //---------------------------------------------------------------------
  // Mainloop the sever connection to make sure we close all open links
  printf("Waiting for connections to close\n");
  struct timeval now, start;
  vrpn_gettimeofday(&now, NULL);
  start = now;
  while ( now.tv_sec - start.tv_sec < 3 ) {
    server_connection->mainloop();
    vrpn_gettimeofday(&now, NULL);
  }

  //---------------------------------------------------------------------
  // Delete the server-side objects and get rid of the server connection
  // by reducing its reference count.
  delete server_text_receiver;
  delete server_text_sender;
  server_connection->removeReference();

  //---------------------------------------------------------------------
  // To test the case of logging server-side incoming messages when the
  // server was not constructed to do so automatically, we need to open
  // the server again with no logging requested and then connect to it
  // with a client that requests it.
  server_connection = vrpn_create_server_connection(CONNECTION_PORT);
  if (server_connection == NULL) {
    fprintf(stderr,"Cannot open server connection\n");
    return -1;
  }

  //---------------------------------------------------------------------
  // Open the server-side text sender and receivers.
  server_text_sender = new vrpn_Text_Sender(CLIENT_TEXT_NAME, server_connection);
  server_text_receiver = new vrpn_Text_Receiver(SERVER_TEXT_NAME, server_connection);
  if ( (server_text_sender == NULL) || (server_text_receiver == NULL) ) {
    fprintf(stderr,"Cannot create text server or client\n");
    return -3;
  }

  //---------------------------------------------------------------------
  // Establish the server-in-logging connection and make sure it works.
  // (This will put entries into the server-side log files.)
  printf("Sending messages to and from server with server-out\n");
  if (0 != open_client_connection_and_loop(NULL, NULL, CLIENT_SERVER_INCOMING_LOG, NULL )) {
    fprintf(stderr,"Could not test CLIENT_SERVER_INCOMING_LOG connection\n");
    ret = -4;
  }

  //---------------------------------------------------------------------
  // Try re-writing the same log file a couple of times.  This turned up a
  // crash case before.  We do it twice because it may save an emergency
  // log file in temp the first time.
  printf("Testing for crash when attempt to rewrite file with client-out\n");
  open_client_connection_and_loop(NULL, CLIENT_CLIENT_OUTGOING_LOG, NULL, NULL);
  open_client_connection_and_loop(NULL, CLIENT_CLIENT_OUTGOING_LOG, NULL, NULL);

  //---------------------------------------------------------------------
  // Mainloop the server connection to make sure we close all open links
  printf("Waiting for connections to close\n");
  vrpn_gettimeofday(&now, NULL);
  start = now;
  while ( now.tv_sec - start.tv_sec < 3 ) {
    server_connection->mainloop();
    vrpn_gettimeofday(&now, NULL);
  }

  //---------------------------------------------------------------------
  // Delete the server-side objects and get rid of the server connection
  // by reducing its reference count.
  delete server_text_receiver;
  delete server_text_sender;
  server_connection->removeReference();

  //---------------------------------------------------------------------
  // Try reading from each of the log files in turn to make sure each
  // contains at least one text message of the desired type.
  if (0 != check_for_messages_in(SERVER_TEXT_NAME, make_server_incoming_name(SERVER_BASE_INCOMING_LOG,1))) {
    fprintf(stderr,"Failure when reading from server base incoming log file\n");
    ret = -5;
  }
  if (0 != check_for_messages_in(CLIENT_TEXT_NAME, SERVER_BASE_OUTGOING_LOG)) {
    fprintf(stderr,"Failure when reading from server base outgoing log file\n");
    ret = -5;
  }
  if (0 != check_for_messages_in(CLIENT_TEXT_NAME, CLIENT_CLIENT_INCOMING_LOG)) {
    fprintf(stderr,"Failure when reading from client-side incoming log file\n");
    ret = -5;
  }
  if (0 != check_for_messages_in(SERVER_TEXT_NAME, CLIENT_CLIENT_OUTGOING_LOG)) {
    fprintf(stderr,"Failure when reading from client-side outgoing log file\n");
    ret = -5;
  }
  if (0 != check_for_messages_in(SERVER_TEXT_NAME, CLIENT_SERVER_INCOMING_LOG)) {
    fprintf(stderr,"Failure when reading from server-side incoming log file\n");
    ret = -5;
  }
  if (0 != check_for_messages_in(CLIENT_TEXT_NAME, CLIENT_SERVER_OUTGOING_LOG)) {
    fprintf(stderr,"Failure when reading from server-side outgoing log file\n");
    ret = -5;
  }

  // Clean up after ourselves by deleting the log files.
  printf("Deleting log files\n");
  // Don't complain about using "unlink"
#ifdef _MSC_VER
#pragma warning ( disable : 4996 )
#endif

  char * name;
  name = make_server_incoming_name(SERVER_BASE_INCOMING_LOG,1);
  unlink(name);
  delete[] name;

  name = make_server_incoming_name(SERVER_BASE_INCOMING_LOG,2);
  unlink(name);
  delete[] name;

  name = make_server_incoming_name(SERVER_BASE_INCOMING_LOG,3);
  unlink(name);
  delete[] name;

  name = make_server_incoming_name(SERVER_BASE_INCOMING_LOG,4);
  unlink(name);
  delete[] name;

  unlink(SERVER_BASE_OUTGOING_LOG);
  unlink(CLIENT_CLIENT_INCOMING_LOG);
  unlink(CLIENT_CLIENT_OUTGOING_LOG);
  unlink(CLIENT_SERVER_INCOMING_LOG);
  unlink(CLIENT_SERVER_OUTGOING_LOG);

  unlink("/tmp/vrpn_emergency_log");

  if (ret == 0) {
    printf("Success!\n");
  } else {
    printf("Failure\n");
  }
  return ret;

}   /* main */
