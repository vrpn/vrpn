/** @file
	@brief Header

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

#ifndef INCLUDED_vrpn_Socket_h_GUID_bc213b91_a1c1_4071_8118_62454e579e56
#define INCLUDED_vrpn_Socket_h_GUID_bc213b91_a1c1_4071_8118_62454e579e56

// Internal Includes
#include "vrpn_Configure.h"

// Library/third-party includes
// - none

// Standard includes
#include <string>

/// @brief A thin wrapper class around a (IPv4) socket, that automatically
/// closes the socket in the destructor.
///
/// The purpose of this class is to put an object-oriented, RAII front on sockets,
/// hiding messy platform differences, workarounds, and gnarly unchecked C behaviors.
///
/// It also serves to centralize socket-handling code to reduce duplication
/// in the codebase, at least in theory.
class VRPN_API vrpn_Socket {
	public:
		/// @brief Constructor - initializes to an invalid socket.
		vrpn_Socket();

		/// @brief Destructor - closes the socket if applicable.
		~vrpn_Socket();

#ifdef _WIN32
		typedef SOCKET socket_t
#else
		typedef int socket_t;
#endif
		/// @brief Returns if the socket is valid
		bool valid() const;

		/// @name Open methods
		/// @brief These methods all "open" a socket by calling "socket()" then "bind()"
		///
		/// If no port is specified (default 0), one will be automatically selected.
		/// If no local IP address is specified, one will automatically be selected (or all interfaces will be used)
		/// @{
		/// @brief Open a socket specifying "type" manually.
		void open(int type, unsigned short localportno = 0, const char * localIPaddress = 0);


		void openUDP(unsigned short localportno = 0, const char * localIPaddress = 0);


		void openTCP(unsigned short localportno = 0, const char * localIPaddress = 0);
		/// @}

		/// @brief Closes the socket. No-op if the socket is invalid.
		void close();

		/// @brief Connects an open socket to a remote IP and port.
		void connect(unsigned short remortportno, const char * remoteIPaddress);

		/// @brief On a TCP socket, starts listening, with a queue n long.
		void listen(int n = 1);

		/// @brief Block (for timeout > 0) for up to a timeout until this
		/// socket is ready for read (or has a connection request)
		bool selectForRead(double timeoutSeconds = 0.0);

		/// @brief If this is a listening socket, check to see if there has been a connection
		/// request. If so, it will accept a connection on the accept socket and
		/// set TCP_NODELAY on that socket. Returns true if a connection request is handled.
		bool pollForAccept(vrpn_Socket & acceptSock, double timeoutSeconds = 0.0);

		/// @brief Relatively bare wrapper around recv(): one of the string
		/// versions is recommended to avoid thinking about buffer issues.
		int receive(unsigned char * buffer, int maxlen);

		/// @brief Same result as recv() with results returned in a string.
		std::string receive();

		/// @brief Receive up to the indicated number of packets that are immediately available,
		/// or all such packets if numPackets = -1, waiting up to the timeout for each packet.
		std::string receiveAvailable(double timeoutSeconds = 0.0, int numPackets = -1);

		/// @brief Send a string literal (or other statically-allocated char string) safely.
		template<size_t LEN>
		void send(const char message[LEN]) {
			send_(message, LEN);
		}

		/// @brief Send a string.
		void send(std::string const& message);

		/// @brief Advanced: take over a socket from a raw file descriptor, managing its lifetime.
		void acquire(socket_t s);

		/// @brief Advanced: Return the raw file descriptor of this socket, emptying
		/// this object and making _you_ responsible for lifetime management.
		socket_t release();

	private:
		/// @name noncopyable
		/// @{
		vrpn_Socket(vrpn_Socket const&);
		vrpn_Socket const& operator=(vrpn_Socket const&);
		/// @}

		/// @brief Internal wrapper for vrpn_noint_select which enhances select()
		int select_(void * readfds, void * writefds = NULL, void * exceptfds = NULL, double timeoutSeconds = 0.0);

		/// @brief Internal wrapper for socket()
		void socket_(int type);

		/// @brief Internal wrapper for bind()
		void bind_(unsigned short localportno, const char * localIPaddress);

		/// @brief Internal wrapper for send()
		void send_(const char * msg, size_t len);

		socket_t sock_;
};

#endif // INCLUDED_vrpn_Socket_h_GUID_bc213b91_a1c1_4071_8118_62454e579e56
