/**
	@file
	@brief Implementation

	@date 2013

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "vrpn_Socket.h"

#include "vrpn_Shared.h"
#include "vrpn_Connection.h"

// Library/third-party includes
// - none

// Standard includes
#include <stdexcept>

#ifndef	_WIN32_WCE
#include <errno.h>                      // for errno, EINTR
#endif

#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, stderr, NULL, etc
#include <string.h>                     // for strlen, strcpy, memcpy, etc


#ifdef VRPN_USE_WINSOCK_SOCKETS
#else
#include <arpa/inet.h>                  // for inet_addr
#include <netinet/in.h>                 // for sockaddr_in, ntohl, in_addr, etc
#include <sys/socket.h>                 // for getsockname, send, AF_INET, etc
#include <unistd.h>                     // for close, read, fork, etc
#ifdef _AIX
#define _USE_IRS
#endif
#include <netdb.h>                      // for hostent, gethostbyname, etc
#endif

#ifdef	sparc
#include <arpa/inet.h>                  // for inet_addr

#define INADDR_NONE -1
#endif

#ifdef hpux
#include <arpa/nameser.h>
#include <resolv.h>  // for herror() - but it isn't there?
#endif

#ifndef VRPN_USE_WINSOCK_SOCKETS
#include <sys/wait.h>                   // for wait, wait3, WNOHANG
#ifndef __CYGWIN__
#include <netinet/tcp.h>                // for TCP_NODELAY
#endif /* __CYGWIN__ */
#endif /* VRPN_USE_WINSOCK_SOCKETS */

vrpn_Socket::vrpn_Socket() : sock_(INVALID_SOCKET) {
#ifdef VRPN_USE_WINSOCK_SOCKETS
	{
		WORD vreq;
		WSADATA wsa;

		vreq = MAKEWORD(2, 2);

		if(WSAStartup(vreq, &wsa) != 0){
			throw std::runtime_error("Could not initialize Winsock");
		}
	}
#endif
}
vrpn_Socket::~vrpn_Socket() {
	close();
#ifdef VRPN_USE_WINSOCK_SOCKETS
	WSACleanup();
#endif
}

bool vrpn_Socket::valid() const {
	return sock_ != INVALID_SOCKET;
}
namespace {

	/// @brief Map host name to IP address, allowing for dotted decimal
	void fillAddress(struct sockaddr_in & name, unsigned short portno, const char * IPaddress) {
		memset(static_cast<void*>(&name), 0, sizeof(name));
		name.sin_family = AF_INET;
		if (portno) {
			name.sin_port = htons(portno);
		} else {
			name.sin_port = htons(0);
		}

		/// @todo use getaddrinfo

		// gethostbyname() fails on SOME Windows NT boxes, but not all,
		// if given an IP octet string rather than a true name.
		// MS Documentation says it will always fail and inet_addr should
		// be called first. Avoids a 30+ second wait for
		// gethostbyname() to fail.
		if (!IPaddress) {
			name.sin_addr.s_addr = INADDR_ANY;
		} else {
			name.sin_addr.s_addr = inet_addr(IPaddress);
			if (name.sin_addr.s_addr == INADDR_NONE) {

				struct hostent * host = gethostbyname(IPaddress);
				if (host != NULL) {
#ifdef CRAY
					int i;
					u_long foo_mark = 0L;
					for  (i = 0; i < 4; i++) {
					  u_long one_char = host->h_addr_list[0][i];
					  foo_mark = (foo_mark << 8) | one_char;
					}
					udp_name.sin_addr.s_addr = foo_mark;
#else
					memcpy(static_cast<void *>(&name.sin_addr), static_cast<const void *>(host->h_addr), host->h_length);
#endif
				} else {
					fprintf(stderr, "open_socket:  can't get %s host entry\n", IPaddress);
					throw std::runtime_error("Cannot get host entry!");
				}
			}
		}
	}

}

void vrpn_Socket::open(int type, unsigned short localportno, const char * localIPaddress) {
	socket_(type);

	// bind to local address
	bind_(localportno, localIPaddress);
}

void vrpn_Socket::openUDP(unsigned short localportno, const char * localIPaddress) {
	open(SOCK_DGRAM, localportno, localIPaddress);
}

void vrpn_Socket::openTCP(unsigned short localportno, const char * localIPaddress) {
	open(SOCK_STREAM, localportno, localIPaddress);
}

void vrpn_Socket::close() {
	if (valid()) {
#ifdef VRPN_USE_WINSOCK_SOCKETS
		::closesocket(sock_);
#else
		::close(sock_);
#endif
		sock_ = INVALID_SOCKET;
	}
}
void vrpn_Socket::connect(unsigned short remortportno, const char * remoteIPaddress) {
	struct sockaddr_in name;
	fillAddress(name, remortportno, remoteIPaddress);
	int err = ::connect(sock_, reinterpret_cast<struct sockaddr *>(&name), sizeof(name));
	if (err) {
		throw std::runtime_error("Can't connect socket!");
	}
}

void vrpn_Socket::listen(int n) {
	int err = ::listen(sock_, n);
	if (err) {
		throw std::runtime_error("Can't listen on socket!");
	}
}


int vrpn_Socket::select_(void * readfds, void * writefds, void * exceptfds, double timeoutSeconds) {
	struct	timeval t;
	t.tv_sec = long(timeoutSeconds);
	t.tv_usec = long( (timeoutSeconds - t.tv_sec) * 1000000L );

	int ret = vrpn_noint_select(FD_SETSIZE, static_cast<fd_set *>(readfds), static_cast<fd_set *>(writefds), static_cast<fd_set *>(exceptfds), &t);
	if (ret == -1) {
		throw std::runtime_error("select() failed!");
	}
	return ret;
}

bool vrpn_Socket::selectForRead(double timeoutSeconds) {
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sock_, &rfds);	/* Check for read (connect) */
	return (select_(&rfds, NULL, NULL, timeoutSeconds) > 0 && FD_ISSET(sock_, &rfds));
}

bool vrpn_Socket::pollForAccept(vrpn_Socket & acceptSock, double timeoutSeconds) {
	/* Check for read (connect) */
	if (selectForRead(timeoutSeconds)) {
		socket_t accepted = ::accept(sock_, 0, 0);
		if (accepted == INVALID_SOCKET) {
			throw std::runtime_error("accept() failed even though select() indicated it should succeed!");
		}

		acceptSock.acquire_(accepted);

#if	!defined(_WIN32_WCE) && !defined(__ANDROID__)
		{
			struct	protoent	*p_entry;
			int	nonzero = 1;

			if ( (p_entry = getprotobyname("TCP")) == NULL ) {
				fprintf(stderr, 	"vrpn_poll_for_accept: getprotobyname() failed.\n");
				throw std::runtime_error("getprotobyname() failed when working to set TCP_NODELAY");
			}

			if (setsockopt(accepted, p_entry->p_proto,
				TCP_NODELAY, static_cast<socket_t *>(&nonzero), sizeof(nonzero))==-1) {
				acceptSock.close();
				throw std::runtime_error("pollForAccept: setsockopt() failed");
			}
		}
#endif
		return true;
	}
	return false;
}

int vrpn_Socket::receive(unsigned char * buffer, int maxlen) {
	int nbytes = ::recv(sock_, (char *)buffer, maxlen, 0);
	if (nbytes < 0) {
		throw std::runtime_error("Error in receive");
	}
	return nbytes;
}

std::string vrpn_Socket::receive() {
	const int BUFSIZE = 1024;
	unsigned char buf[BUFSIZE];

	int numBytes = receive(buf, BUFSIZE);
	return std::string(&(buf[0]), &(buf[0]) + numBytes);
}

std::string vrpn_Socket::receiveAvailable(double timeoutSeconds, int numPackets) {
	const int BUFSIZE = 1024;
	unsigned char buf[BUFSIZE];
	std::string ret;
	int packetCount = 0;
	for (int packets = 0; packets < numPackets || numPackets == -1; packets++) {
		if (!selectForRead(timeoutSeconds)) {
			break;
		}
		int numBytes = receive(buf, BUFSIZE);
		ret.append(&(buf[0]), &(buf[0]) + numBytes);
	}
	return ret;
}
void vrpn_Socket::socket_(int type) {
	// create an Internet socket of the appropriate type
	sock_ = socket(AF_INET, type, 0);
	if (sock_ == INVALID_SOCKET)  {
		fprintf(stderr, "open_socket: can't open socket.\n");
#ifndef _WIN32_WCE
		fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
#endif
		throw std::runtime_error("Cannot open socket!");
	}

	// Added by Eric Boren to address socket reconnectivity on the Android
	#ifdef __ANDROID__
	vrpn_int32 optval = 1;
	vrpn_int32 sockoptsuccess = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	fprintf(stderr, "setsockopt returned %i, optval: %i\n", sockoptsuccess, optval);
	#endif
}

void vrpn_Socket::bind_(unsigned short localportno, const char * localIPaddress) {

	struct sockaddr_in name;
	fillAddress(name, localportno, localIPaddress);

	if (::bind(sock_, reinterpret_cast<struct sockaddr *>(&name), sizeof(name)) < 0){
		fprintf(stderr, "open_socket:  can't bind address %d", localportno);
#ifndef _WIN32_WCE
		fprintf(stderr, "  --  %d  --  %s\n", errno, strerror(errno));
#endif
		fprintf(stderr,"  (This probably means that another application has the port open already)\n");
		throw std::runtime_error("Cannot bind address!");
	}
}
void vrpn_Socket::send(std::string const& message) {
	send_(message.c_str(), message.length());
}

void vrpn_Socket::send_(const char * msg, size_t len) {
	int err = ::send(sock_, msg, len, 0);
	if (err == -1) {
		throw std::runtime_error("Error in send()!");
	}
}

void vrpn_Socket::acquire_(socket_t s) {
	close();
	sock_ = s;
}

