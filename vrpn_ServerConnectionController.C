
//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: connection setup
//
//**************************************************************************
//**************************************************************************

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
void vrpn_ServerConnectionController::
listen_for_incoming_connections(const struct timeval * pTimeout){
   vrpn_int32	request;
   char	msg[200];	// Message received on the request channel
   timeval timeout;
   if (pTimeout) {
     timeout = *pTimeout;
   } else {
     timeout.tv_sec = 0;
     timeout.tv_usec = 0;
   }

   // Do a zero-time select() to see if there is an incoming packet on
   // the UDP socket.

   fd_set f;
   FD_ZERO (&f);
   FD_SET ( listen_udp_sock, &f);
   request = select(32, &f, NULL, NULL, &timeout);
   if (request == -1 ) {	// Error in the select()
	perror("vrpn: vrpn_ServerConnectionController: select failed");
	status = BROKEN;
	return;
   } 
   else if (request != 0) {	// Some data to read!  Go get it.
	   struct sockaddr from;
	   vrpn_int32 fromlen = sizeof(from);
	   if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0,
					&from, GSN_CAST &fromlen) == -1) {
		   fprintf(stderr,
				   "vrpn: Error on recvfrom: Bad connection attempt\n");
		   return;
	   }

	   
	   // create new NetConnection
	   BaseConnection* new_connection = new NetConnection;
	   
	   // add it to the connection list
	   
	   // NetConnection will handle rest

	   // either unmarshall machine name and port or
	   // just pass whole message to function 
	   new_connection->connect_to_client(machine,port);
	   new_connection->connect_to_client(msg);
   }
   return;
}



//**************************************************************************
//**************************************************************************
//
// vrpn_ServerConnectionController: public: sending and receiving
//
//**************************************************************************
//**************************************************************************

vrpn_int32 mainloop( const timeval * timeout){

	
