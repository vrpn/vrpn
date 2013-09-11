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
// - none

// Library/third-party includes
// - none

// Standard includes
#include <string>

class vrpn_Socket {
	public:
		vrpn_Socket();

		~vrpn_Socket();

		bool valid() const;

		void open(int type, unsigned short localportno, const char * localIPaddress = 0);
		void openUDP(unsigned short localportno = 0, const char * localIPaddress = 0);
		void openTCP(unsigned short localportno = 0, const char * localIPaddress = 0);
		void close();

		void connect(unsigned short remortportno, const char * remoteIPaddress);
		void listen(int n = 1);

		bool selectForRead(double timeoutSeconds = 0.0);

		/// @brief If this is a listening socket, check to see if there has been a connection
		/// request. If so, it will accept a connection on the accept socket and
		/// set TCP_NODELAY on that socket. Returns true if a connection request is handled.
		bool pollForAccept(vrpn_Socket & acceptSock, double timeoutSeconds = 0.0);

		int receive(unsigned char * buffer, int maxlen);
		std::string receive();
		/// @brief Receive up to the indicated number of packets that are immediately available,
		/// or all such packets if numPackets = -1
		std::string receiveAvailable(double timeoutSeconds = 0.0, int numPackets = -1);

		template<size_t LEN>
		void send(const char message[LEN]) {
			send_(message, LEN);
		}

		void send(std::string const& message);

	private:
		/// @name noncopyable
		/// @{
		vrpn_Socket(vrpn_Socket const&);
		vrpn_Socket const& operator=(vrpn_Socket const&);
		/// @}
		int select_(void * readfds, void * writefds = NULL, void * exceptfds = NULL, double timeoutSeconds = 0.0);
		void socket_(int type);
		void bind_(unsigned short localportno, const char * localIPaddress);
		void send_(const char * msg, size_t len);
#ifdef _WIN32
		typedef SOCKET socket_t
#else
		typedef int socket_t;
#endif

		/// @brief Take over a socket from a raw file descriptor, managing lifetime.
		void acquire_(socket_t s);
		socket_t sock_;
};

#endif // INCLUDED_vrpn_Socket_h_GUID_bc213b91_a1c1_4071_8118_62454e579e56
