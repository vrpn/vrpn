
void vrpn_BaseConnection::init(void)
{
	vrpn_int32 i;

	tcp_sock = INVALID_SOCKET;
	udp_outbound = INVALID_SOCKET;

	// Set all of the local IDs to -1, in case the other side
	// sends a message of a type that it has not yet defined.
	// (for example, arriving on the UDP line ahead of its TCP
	// definition).
	num_other_senders = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
		other_senders[i].local_id = -1;
		other_senders[i].name = NULL;
	}

	num_other_types = 0;
	for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
		other_types[i].local_id = -1;
		other_types[i].name = NULL;
	}

}

//**************************************************************************
//**************************************************************************
//
// vrpn_BaseConnection: public: sending and receiving
//
//**************************************************************************
//**************************************************************************
