// Must be done before stdio.h to avoid conflict for SEEK_SET, at least for MPICH2 on
// the Windows platform..  Note that this means we cannot include it in vrpn_Connection.h,
// because user code often includes stdio.h before and VRPN includes.
#ifdef  vrpn_USE_MPI
#include <mpi.h>
#endif

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#ifndef	_WIN32_WCE
#include <errno.h>
#endif
#include <string.h>  // for strerror()
#include <stdio.h>
#ifndef _WIN32_WCE
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// malloc.h is deprecated;  all the functionality *should*
// be in stdlib.h
#include <stdlib.h>

#include "vrpn_Connection.h"

#ifdef VRPN_USE_WINSOCK_SOCKETS
// a socket in windows can not be closed like it can in unix-land
#define vrpn_closeSocket closesocket
#else
#define vrpn_closeSocket close
#include <unistd.h>
#ifndef _WIN32_WCE
#include <sys/types.h>
#endif
  // gethostname() and getdtablesize() should be here on SGIs,
  // but apparently aren't under g++
#include <strings.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // for inet_addr()?
#ifdef _AIX
#define _USE_IRS
#endif
#include <netdb.h>
#endif

#ifdef	sparc
#include <arpa/inet.h>
#define INADDR_NONE -1
#endif

#ifdef	sgi
#include <bstring.h>
#endif

#ifdef hpux
#include <arpa/nameser.h>
#include <resolv.h>  // for herror() - but it isn't there?
#endif

#ifndef VRPN_USE_WINSOCK_SOCKETS
#include <sys/wait.h>
#include <sys/resource.h>  // for wait3() on sparc_solaris
#ifndef __CYGWIN__
#include <netinet/tcp.h>
#endif /* __CYGWIN__ */
#endif /* VRPN_USE_WINSOCK_SOCKETS */

// cast fourth argument to setsockopt()
#ifdef VRPN_USE_WINSOCK_SOCKETS
  #define SOCK_CAST (char *)
#ifdef _WIN32_WCE
  #define errno WSAGetLastError()
  #define EINTR WSAEINTR
#endif
#else
  #ifdef sparc
    #define SOCK_CAST (const char *)
  #else
    #define SOCK_CAST
  #endif
#endif

#if defined(_AIX) || defined(__APPLE__)
 #define GSN_CAST (socklen_t *)
#else
 #if defined(linux) || defined(FreeBSD)
  #define GSN_CAST (unsigned int *)
 #else
  #define GSN_CAST
 #endif
#endif

//  NOT SUPPORTED ON SPARC_SOLARIS
//  gethostname() doesn't seem to want to link out of stdlib
#ifdef sparc
extern "C" {
int gethostname (char *, int);
}
#endif

#include "vrpn_Log.h"

#include "vrpn_FileConnection.h"  // for vrpn_get_connection_by_name

//#define VERBOSE
//#define VERBOSE2
//#define VERBOSE3
//#define PRINT_READ_HISTOGRAM

//   Warning:  PRINT_READ_HISTOGRAM is not thread-safe.

// On Win32, this constant is defined as ~0 (sockets are unsigned ints)
#ifndef	VRPN_USE_WINSOCK_SOCKETS
#define	INVALID_SOCKET	-1
#endif

// Don't tell us about FD_SET() "conditional expression is constant"
#ifdef	_WIN32
#pragma	warning ( disable : 4127)
#endif

// Syntax of vrpn_MAGIC:
//
// o The minor version number must follow the last period ('.') and be the
// only significant thing following the last period in the vrpn_MAGIC string.
// 
// o Minor versions should interoperate.  So, when establishing a connection,
// vrpn_MAGIC is checked through the last period.  If everything up to, and
// including, the last period matches, a connection will be made.
// 
// o If everything up to the last period matches, then a second check is
// preformed on everything after the last period (the minor version number).
// If the minor version numbers differ, a connection is still made, but a
// warning is printed to stderr.  There is currently no way to supress this
// warning message if the minor versions differ between the server and the
// client..
//
// o The version checking described above is performed by the function
// check_vrpn_cookie, found in this file.  check_vrpn_cookie returns a
// different value for each of these three cases.
//                                                    -[juliano 2000/08]
// 
// [juliano 2000/08] Suggestion:
//
// We have the situation today that vrpn5 can read stream files that were
// produced by vrpn-4.x.  However, the way the library is written, vrpn
// doesn't know this.  Further, it is difficult to change this quickly,
// because vrpn_check_cookie doesn't know if it's being called for a network
// or a file connection.  The purpose of this comment is to suggest a
// solution.
// 
// Our temporary solution is to change the cookie, rebulid the library, and
// relink into our app.  Then, our app can read stream files produced by
// vrpn4.x.
// 
// Vrpn currently knows that live network connections between vrpn-4.x and
// vrpn-5.x apps are not possible.  But, ideally, it should also know that
// it's ok for a vrpn-5.x app to read a vrpn-4.x streamfile.  Unfortunately,
// coding this is difficult in the current framework.
//
// I suggest that check_vrpn_cookie should not be a global function, but
// should instead be a protected member function of vrpn_Connection.  The
// default implementation would do what is currently done by the global
// check_vrpn_cookie.  However, vrpn_FileConnection would override it and
// perform a more permissive check.
//
// This strategy would scale in the future when we move to vrpn-6.x and
// higher.  It would also be useful if we ever realize after a release that
// VRPN-major.minor is actually network incompatible (but not streamfile
// incompatible) with VRPN-major.(minor-1).  Then, the vrpn_check_cookie
// implementation in VRPN-major.(minor-1) could test for this incompatibility
// and print an appropriate diagnostic.  Similar solution exists if release
// n+1 is file-compatible but later found to be network-incompatible with
// release n.
// 
// Again, in our current framework, we cannot distinguish between
// file-compatible and network-compatible.  In the future, we may also have
// shared-memory-access-compatible as well as other types of connection.  The
// proposed strategy handles both partial major version compatibility as well
// as accidental partial minor version incompatibility.
//
const char * vrpn_MAGIC = (const char *) "vrpn: ver. 07.20";
const char * vrpn_FILE_MAGIC = (const char *) "vrpn: ver. 04.00";
const int vrpn_MAGICLEN = 16;  // Must be a multiple of vrpn_ALIGN bytes!

const char *vrpn_got_first_connection	= "VRPN_Connection_Got_First_Connection";
const char *vrpn_got_connection		= "VRPN_Connection_Got_Connection";
const char *vrpn_dropped_connection	= "VRPN_Connection_Dropped_Connection";
const char *vrpn_dropped_last_connection= "VRPN_Connection_Dropped_Last_Connection";

const char *vrpn_CONTROL = "VRPN Control";

//**********************************************************************
//**  This section has been pulled from the "SDI" library and had its
//**  functions renamed to vrpn_ from sdi_.  This removes our dependence
//**  on libsdi.a for VRPN.

#ifdef sparc

// On capefear and swift, getdtablesize() isn't declared in unistd.h
// even though the man page says it should be.  Similarly, wait3()
// isn't declared in sys/{wait,time,resource}.h.
extern "C" {
    extern int getdtablesize (void);
    pid_t wait3 (int * statusp, int options, struct rusage * rusage);
}
#endif

/* On HP's, this defines how many possible open file descriptors can be
 * open at once.  This is what is returned by the getdtablesize() function
 * on other architectures. */
#ifdef  hpux
#define getdtablesize() MAXFUPLIM
#endif

#ifdef __hpux
#define getdtablesize() MAXFUPLIM
#endif

/* The version of rsh in /usr/local/bin is the AFS version that passes tokens
 * to the remote machine.  This will allow remote execution of anything you
 * can execute locally.  This is the default location from which to get rsh.
 * If the VRPN_RSH environment variable is set, that will be used as the full
 * path instead.  */
#ifdef  linux
#define RSH             (char *) "/usr/local/bin/ssh"
#else
#define RSH             (char *) "/usr/local/bin/rsh"
#endif

/* How long to wait for a UDP packet to cause a callback connection,
 * and how many times to retry. */
#define	UDP_CALL_TIMEOUT	(2)
#define	UDP_CALL_RETRIES	(5)

/* How long to wait for the server to connect, and how many times to wait
 * this long.  The death of the child is checked for between each of the
 * waits, in order to allow faster exit if the child quits before calling
 * back. */
#define SERVCOUNT       (20)
#define SERVWAIT        (120/SERVCOUNT)


// From vrpn_CONNECTION_MAX_SENDERS and vrpn_CONNECTION_MAX_TYPES
// in vrpn_Connection.h.

#define vrpn_CONNECTION_MAX_XLATION_TABLE_SIZE 2000


/*
  Major refactoring 18-20 April 2000 T. Hudson

  Broke two classes (vrpn_Connection, vrpn_OneConnection) into five:

    vrpn_TranslationTable
    vrpn_TypeDispatcher
    vrpn_Log
    vrpn_Endpoint
    vrpn_Connection

  Each Connection manages one Endpoint per Connection that it is
  communicating with.  Each Endpoint has a Log, and at a future date
  some Connections may have their own logs.  Each Endpoint has two
  TranslationTables to map remote senders and types to their local
  identifiers;  these tables are also used by Logs.  The entire system
  shares a single TypeDispatcher to track local types, senders, and callbacks.

  This decomposition let me get rid of the circular references between
  Connection and Endpoint and between Endpoint and Log.  It lets us
  have logs attached to both Endpoints and Connections.  It better
  isolates and identifies some of the functionality we're using.

  I've always thought the central component of VRPN is the TypeDispatcher,
  which I've also seen in the (late '80s) commercial database middleware
  I've hacked the internals of.  There isn't an example of it in the
  Gang-Of-4 "Design Patterns" book, but I'm doing a small Patterns literature
  search to try to find it.

  This module's interface still only contains Connection and Endpoint.
  The only reason Endpoint is visible is so that it can be used by
  vrpn_FileConnection;  unfortunately, it isn't easy to factor it out
  of there.  I'd suggest moving Connection and FileConnection into their
  own directory;  we can then extract all the classes out of this
  file into their own C files.

  TypeDispatcher could certainly use a better name.
*/


/**
 * @class vrpn_TranslationTable
 * Handles translation of type and sender names between local and
 * network peer equivalents.
 * Used by Endpoints, Logs, and diagnostic code.
 */

struct cRemoteMapping {
  char * name;
  vrpn_int32 remote_id;
  vrpn_int32 local_id;
};

class vrpn_TranslationTable {

  public:

    vrpn_TranslationTable (void);
    ~vrpn_TranslationTable (void);

    // ACCESSORS

    vrpn_int32 numEntries (void) const;
    vrpn_int32 mapToLocalID (vrpn_int32 remote_id) const;

    // MANIPULATORS

    void clear (void);
      ///< Deletes every entry in the table.

    vrpn_int32 addRemoteEntry (cName name, vrpn_int32 remote_id,
                               vrpn_int32 local_id);
      ///< Adds a name and local ID to the table, returning its
      ///< remote ID.  This exposes an UGLY hack in the VRPN internals -
      ///< that ID is implicitly carried as the index into this array,
      ///< and there isn't much in the way of checking (?).
    vrpn_bool addLocalID (const char * name, vrpn_int32 local_id);
      ///< Adds a local ID to a name that was already in the table;
      ///< returns TRUE on success, FALSE if not found.

  private:

    vrpn_int32 d_numEntries;
    cRemoteMapping d_entry [vrpn_CONNECTION_MAX_XLATION_TABLE_SIZE];

};

vrpn_TranslationTable::vrpn_TranslationTable (void) :
    d_numEntries (0) {
  int i;

  for (i = 0; i < vrpn_CONNECTION_MAX_XLATION_TABLE_SIZE; i++) {
    d_entry[i].name = NULL;
    d_entry[i].remote_id = -1;
    d_entry[i].local_id = -1;
  }
}

vrpn_TranslationTable::~vrpn_TranslationTable (void) {
  clear();
}

vrpn_int32 vrpn_TranslationTable::numEntries (void) const {
  return d_numEntries;
}

vrpn_int32 vrpn_TranslationTable::mapToLocalID (vrpn_int32 remote_id) const {
  if ((remote_id < 0) || (remote_id > d_numEntries)) {

#ifdef VERBOSE2
    // This isn't an error!?  It happens regularly!?
    fprintf(stderr, "vrpn_TranslationTable::mapToLocalID:  "
                    "Remote ID %d is illegal!\n", remote_id);
#endif

    return -1;
  }

#ifdef VERBOSE
  fprintf(stderr, "Remote ID %d maps to local ID %d (%s).\n", remote_id,
  d_entry[remote_id].local_id, d_entry[remote_id].name);
#endif

  return d_entry[remote_id].local_id;
}

vrpn_int32 vrpn_TranslationTable::addRemoteEntry (cName name,
                                                  vrpn_int32 remote_id,
                                                  vrpn_int32 local_id) {
  vrpn_int32 useEntry;

  useEntry = remote_id;

  if (useEntry >= vrpn_CONNECTION_MAX_XLATION_TABLE_SIZE) {
    fprintf(stderr, "vrpn_TranslationTable::addRemoteEntry:  " 
                    "Too many entries in table (%d).\n", d_numEntries);
    return -1;
  }

  // We do not check to see if this entry is already filled in.  Such
  // a check caused problems with vrpn_Control when reading from log files.
  // Also, it will cause problems with multi-logging, where the connection
  // may be requested to send all of its IDs again for a log file is opeened
  // at a time other than connection set-up.

  if (!d_entry[useEntry].name) {
    d_entry[useEntry].name = new cName;
    if (!d_entry[useEntry].name) {
      fprintf(stderr, "vrpn_TranslationTable::addRemoteEntry:  "
                      "Out of memory.\n");
      return -1;
    }
  }

  memcpy(d_entry[useEntry].name, name, sizeof(cName));
  d_entry[useEntry].remote_id = remote_id;
  d_entry[useEntry].local_id = local_id;

#ifdef VERBOSE
  fprintf(stderr, "Set up remote ID %d named %s with local equivalent %d.\n",
  remote_id, name, local_id);
#endif

  if (d_numEntries <= useEntry) {
    d_numEntries = useEntry + 1;
  }

  return useEntry;
}


vrpn_bool vrpn_TranslationTable::addLocalID (const char * name,
                                             vrpn_int32 local_id) {
  int i;

  for (i = 0; i < d_numEntries; i++) {
    if (d_entry[i].name && !strcmp(d_entry[i].name, name)) {
      d_entry[i].local_id = local_id;
      return VRPN_TRUE;
    }
  }
  return VRPN_FALSE;
}

void vrpn_TranslationTable::clear (void) {
  int i;

  for (i = 0; i < d_numEntries; i++) {
    if (d_entry[i].name) {
      delete [] d_entry[i].name;
      d_entry[i].name = NULL;
    }
    d_entry[i].local_id = -1;
    d_entry[i].remote_id = -1;
  }
  d_numEntries = 0;
}



vrpn_Log::vrpn_Log (vrpn_TranslationTable * senders,
                    vrpn_TranslationTable * types) :
    d_logFileName (NULL),
    d_logmode (vrpn_LOG_NONE),
    d_logTail (NULL),
    d_firstEntry (NULL),
    d_file (NULL),
    d_magicCookie (NULL),
    d_wroteMagicCookie(vrpn_FALSE),
    d_filters (NULL),
    d_senders (senders),
    d_types (types)
{

  d_lastLogTime.tv_sec = 0;
  d_lastLogTime.tv_usec = 0;

  // Set up default value for the cookie received from the server
  // because if we are using a file connection and want to
  // write a log, we never receive a cookie from the server.
  d_magicCookie = new char [vrpn_cookie_size() + 1];
  if (!d_magicCookie) {
    fprintf(stderr, "vrpn_Log:  Out of memory.\n");
    return;
  }
  write_vrpn_cookie(d_magicCookie, vrpn_cookie_size() + 1,
                    vrpn_LOG_NONE);

}

vrpn_Log::~vrpn_Log (void) {
  if (d_file) {
    close();
  }

  if (d_filters) {
    vrpnLogFilterEntry * next;
    while (d_filters) {
      next = d_filters->next;
      delete d_filters;
      d_filters = next;
    }
  }

  if (d_magicCookie) {
    delete [] d_magicCookie;
  }
}


char* vrpn_Log::getName()
{
	if( this->d_logFileName == NULL )  return NULL;
	else
	{
		char* s = new char[ strlen( this->d_logFileName )  + 1 ];
		strcpy( s, this->d_logFileName );
		return s;
	}
}


int vrpn_Log::open (void) {

  if (!d_logFileName) {
    fprintf(stderr, "vrpn_Log::open:  Log file has no name.\n");
    return -1;
  }
  if (d_file) {
    fprintf(stderr, "vrpn_Log::open:  Log file is already open.\n");
    return 0;  // not a catastrophic failure
  }

  // If we can open the file for reading, then it already exists.  If
  // so, we don't want to overwrite it.
  d_file = fopen(d_logFileName, "r");
  if (d_file) {
    fprintf(stderr, "vrpn_Log::open:  "
                    "Log file \"%s\" already exists.\n", d_logFileName);
    fclose(d_file);
    d_file = NULL;
  } else {
    d_file = fopen(d_logFileName, "wb");
    if( d_file == NULL ) { // unable to open the file
      fprintf(stderr, "vrpn_Log::open:  "
              "Couldn't open log file \"%s\":  ", d_logFileName);
      perror( NULL /* no additional string */ );
    }
  }

  if (!d_file) { // Try to write to "/tmp/vrpn_emergency_log", unless it exists!
    d_file = fopen("/tmp/vrpn_emergency_log", "r");
    if (d_file) {
      fclose(d_file);
      d_file = NULL;
      perror( "vrpn_Log::open_log:  "
              "Emergency log file \"/tmp/vrpn_emergency_log\" "
              "already exists.\n");
    } else {
      d_file = fopen("/tmp/vrpn_emergency_log", "wb");
      if( d_file == NULL ) {
        perror( "vrpn_Log::open:  "
                "Couldn't open emergency log file "
                "\"/tmp/vrpn_emergency_log\":  ");
      }
    }

    if (!d_file) {
      return -1;
    } else {
      fprintf(stderr, "Writing to /tmp/vrpn_emergency_log instead.\n");
    }
  }

  return 0;
}

int vrpn_Log::close (void) {
  int final_retval = 0;
  final_retval = saveLogSoFar();

  if ( fclose(d_file)) {
    fprintf(stderr, "vrpn_Log::close:  "
                    "close of log file failed!\n");
    final_retval = -1;
  }
  d_file = NULL;

  if (d_logFileName) {
    delete [] d_logFileName;
    d_logFileName = NULL;
  }


  return final_retval;
}

int vrpn_Log::saveLogSoFar(void) {
  vrpn_LOGLIST * lp;
  int host_len;
  int final_retval = 0;
  int retval;

  // If we aren't supposed to be logging, return with no error.
  if(!logMode()) return 0;

  // Make sure the file is open. If not, then error.
  if (!d_file) {
    fprintf(stderr, "vrpn_Log::saveLogSoFar:  "
                    "Log file is not open!\n");

    // Abort writing out log without destroying data needed to
    // clean up memory.

    d_firstEntry = NULL;
    final_retval = -1;
  }

  if(!d_wroteMagicCookie) {
    // Write out the log header (magic cookie)
    // TCH 20 May 1999
    
    // There's at least one hack here:
    //   What logging mode should a client that plays back the log at a
    // later time be forced into?  I believe NONE, but there might be
    // arguments the other way? So, you may want to adjust the cookie
    // to make the log mode 0.
    
    retval = fwrite(d_magicCookie, 1, vrpn_cookie_size(), d_file);
    if (retval != vrpn_cookie_size()) {
      fprintf(stderr, "vrpn_Log::saveLogSoFar:  "
              "Couldn't write magic cookie to log file "
              "(got %d, expected %d).\n",
              retval, vrpn_cookie_size());
      lp = d_logTail;
      final_retval = -1;
    }
    d_wroteMagicCookie = vrpn_TRUE;
  }

  // Write out the messages in the log,
  // starting at d_firstEntry and working backwards
  for (lp = d_firstEntry; lp && !final_retval; lp = lp->prev) {

    // This used to be a horrible hack that wrote the size of the
    // structure (which included a pointer) to the file.  this broke on
    // 64-bit machines, but could also have broken on any architecture
    // that packed structures differently from the common packing.
    // Here, we pull out the entries in a way that avoids doing any
    // sign changes and then write the array of values to disk.
    // Unfortunately, to remain backward-compatible with earlier log
    // files, we need to write the empty pointer.
    // XXX This involves one more copy of these six values than would
    // be strictly needed, but this should be very small relative to the
    // disk-write time.  To fix this would require changing the structure
    // that stored the data.
    vrpn_int32  values[6];
    vrpn_int32  zero = 0;
    memcpy(&(values[0]), &lp->data.type, sizeof(vrpn_int32));
    memcpy(&(values[1]), &lp->data.sender, sizeof(vrpn_int32));
    memcpy(&(values[2]), &lp->data.msg_time.tv_sec, sizeof(vrpn_int32));
    memcpy(&(values[3]), &lp->data.msg_time.tv_usec, sizeof(vrpn_int32));
    memcpy(&(values[4]), &lp->data.payload_len, sizeof(vrpn_int32));
    memcpy(&(values[5]), &zero, sizeof(vrpn_int32));   // Bogus pointer.
    retval = fwrite(values, sizeof(vrpn_int32), 6, d_file);
    
    if (retval != 6) {
      fprintf(stderr, "vrpn_Log::saveLogSoFar:  "
                      "Couldn't write log file (got %d, expected %d).\n",
              retval, sizeof(lp->data));
      lp = d_logTail;
      final_retval = -1;
      continue;
    }

    host_len = ntohl(lp->data.payload_len);

//fprintf(stderr, "type %d, sender %d, payload length %d\n",
//htonl(lp->data.type), htonl(lp->data.sender), host_len);

    retval = fwrite(lp->data.buffer, 1, host_len, d_file);

    if (retval != host_len) {
      fprintf(stderr, "vrpn_Log::saveLogSoFar:  "
                      "Couldn't write log file.\n");
      lp = d_logTail;
      final_retval = -1;
      continue;
    }
  }

  // clean up the linked list
  while (d_logTail) {
    lp = d_logTail->next;
    if (d_logTail->data.buffer) {
      delete [] (char *) d_logTail->data.buffer;  // ugly cast
    }
    delete d_logTail;
    d_logTail = lp;
  }

  d_firstEntry = NULL;

  return final_retval;
}


int vrpn_Log::logIncomingMessage
                   (vrpn_int32 payloadLen, struct timeval time,
                    vrpn_int32 type, vrpn_int32 sender, const char * buffer) {

  // Log it the same way, whether it's a User or System message.
  // (We used to throw away system messages that we didn't have a handler
  // for, but I believe that was incorrect.)

  if (logMode() & vrpn_LOG_INCOMING) {
//fprintf(stderr, "Logging incoming message of type %d.\n", type);
      return logMessage(payloadLen, time,
                        type, sender, buffer, vrpn_TRUE);
  }
//fprintf(stderr, "Not logging incoming messages (type %d)...\n", type);

  return 0;
}

int vrpn_Log::logOutgoingMessage
                   (vrpn_int32 payloadLen, struct timeval time,
                    vrpn_int32 type, vrpn_int32 sender, const char * buffer) {
  if (logMode() & vrpn_LOG_OUTGOING) {
//fprintf(stderr, "Logging outgoing message of type %d.\n", type);
    return logMessage(payloadLen, time, type, sender, buffer);
  }
//fprintf(stderr, "Not logging outgoing messages (type %d)...\n", type);
  return 0;
}


int vrpn_Log::logMessage (vrpn_int32 payloadLen, struct timeval time,
                          vrpn_int32 type, vrpn_int32 sender,
                          const char * buffer, vrpn_bool isRemote) {
  vrpn_LOGLIST * lp;
  vrpn_int32 effectiveType;
  vrpn_int32 effectiveSender;

  if (isRemote) {
    effectiveType = d_types->mapToLocalID(type);
    effectiveSender = d_senders->mapToLocalID(sender);
  } else {
    effectiveType = type;
    effectiveSender = sender;
  }

  // Filter user messages
  if (type >= 0) {
    if (checkFilters(payloadLen, time, effectiveType,
                          effectiveSender, buffer)) {
      // This is NOT a failure - do not return nonzero!
      return 0;
    }
  }

  // Make a log structure for the new message
  lp = new vrpn_LOGLIST;
  if (!lp) {
    fprintf(stderr, "vrpn_Log::logMessage:  "
                    "Out of memory!\n");
    return -1;
  }
  lp->data.type = htonl(type);
  lp->data.sender = htonl(sender);

  lp->data.msg_time.tv_sec = htonl(time.tv_sec);
  lp->data.msg_time.tv_usec = htonl(time.tv_usec);

  d_lastLogTime.tv_sec = time.tv_sec;
  d_lastLogTime.tv_usec = time.tv_usec;

  lp->data.payload_len = htonl(payloadLen);
  lp->data.buffer = NULL;

  if (payloadLen > 0) {
    lp->data.buffer = new char [payloadLen];
    if (!lp->data.buffer) {
      fprintf(stderr, "vrpn_Log::logMessage:  "
                      "Out of memory!\n");
      delete lp;
      return -1;
    }

    // need to explicitly override the const
    memcpy((char *) lp->data.buffer, buffer, payloadLen);
  }

  // Insert the new message into the log
  lp->next = d_logTail;
  lp->prev = NULL;
  if (d_logTail) {
    d_logTail->prev = lp;
  }
  d_logTail = lp;
  if (!d_firstEntry) {
    d_firstEntry = lp;
  }

  return 0;
}


int vrpn_Log::setCompoundName (const char * name, int index) {
  char newName [2048];  // HACK
  const char * dot;
  int len;

  // Change foo.bar, 5 to foo-5.bar
  //   and foo, 5 to foo-5

  dot = strrchr(name, '.');

  if (dot) {
    strncpy(newName, name, dot - name);
    newName[dot - name] = 0;
  } else {
    strcpy(newName, name);
  }
  len = strlen(newName);
  sprintf(newName + len, "-%d", index);
  if (dot) {
    strcat(newName, dot);
  }

  return setName (newName);
}

int vrpn_Log::setName (const char * name) {
  return setName(name, strlen(name));
}

int vrpn_Log::setName (const char * name, int len) {
  if (d_logFileName) {
    delete [] d_logFileName;
  }

  d_logFileName = new char [1 + len];
  if (!d_logFileName) {
    fprintf(stderr, "vrpn_Log::setName:  Out of memory!\n");
    return -1;
  }
  strncpy(d_logFileName, name, len);
  d_logFileName[len] = '\0';

  return 0;
}

int vrpn_Log::setCookie (const char * cookieBuffer) {

  if (d_magicCookie) {
    delete [] d_magicCookie;
  }
  d_magicCookie = new char [1 + vrpn_cookie_size()];
  if (!d_magicCookie) {
    fprintf(stderr, "vrpn_Log::setCookie:  Out of memory.\n");
    return -1;
  }
  strncpy(d_magicCookie, cookieBuffer, vrpn_cookie_size());

  return 0;
}

long & vrpn_Log::logMode (void) {
  return d_logmode;
}

int vrpn_Log::addFilter (vrpn_LOGFILTER filter, void * userdata) {
  vrpnLogFilterEntry * newEntry;

  newEntry = new vrpnLogFilterEntry;
  if (!newEntry) {
    fprintf(stderr, "vrpn_Log::addFilter:  Out of memory.\n");
    return -1;
  }

  newEntry->filter = filter;
  newEntry->userdata = userdata;
  newEntry->next = d_filters;
  d_filters = newEntry;

  return 0;
}

timeval vrpn_Log::lastLogTime ()
{
  return d_lastLogTime;
}

int vrpn_Log::checkFilters (vrpn_int32 payloadLen, struct timeval time,
                            vrpn_int32 type, vrpn_int32 sender,
                            const char * buffer) {
  vrpnLogFilterEntry * next;

  vrpn_HANDLERPARAM p;
  p.type = type;
  p.sender = sender;
  p.msg_time.tv_sec = time.tv_sec;
  p.msg_time.tv_usec = time.tv_usec;
  p.payload_len = payloadLen;
  p.buffer = buffer;

  for (next = d_filters; next; next = next->next) {
    if ((*next->filter)(next->userdata, p)) {
      // Don't log
      return 1;
    }
  }

  return 0;
}


/**
 * @class vrpn_TypeDispatcher
 * Handles types, senders, and callbacks.
 * Also handles system messages through the same non-orthogonal technique
 * as always.
 */

class vrpn_TypeDispatcher {

  public:

    vrpn_TypeDispatcher (void);
    ~vrpn_TypeDispatcher (void);


    // ACCESSORS


    int numTypes (void) const;
    const char * typeName (int which) const;

    vrpn_int32 getTypeID (const char * name);
      ///< Returns -1 if not found.

    int numSenders (void) const;
    const char * senderName (int which) const;

    vrpn_int32 getSenderID (const char * name);
      ///< Returns -1 if not found.


    // MANIPULATORS


    vrpn_int32 addType (const char * name);
    vrpn_int32 addSender (const char * name);

    vrpn_int32 registerType (const char * name);
      ///< Calls addType() iff getTypeID() returns -1.
      ///< vrpn_Connection doesn't call this because it needs to know
      ///< whether or not to send a type message.

    vrpn_int32 registerSender (const char * name);
      ///< Calls addSender() iff getSenderID() returns -1.
      ///< vrpn_Connection doesn't call this because it needs to know
      ///< whether or not to send a sender message.

    int addHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler,
                           void * userdata, vrpn_int32 sender);
    int removeHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler,
                              void * userdata, vrpn_int32 sender);
    void setSystemHandler (vrpn_int32 type, vrpn_MESSAGEHANDLER handler);


    // It'd make a certain amount of sense to unify these next few, but
    // there are some places in the code that depend on the side effect of
    // do_callbacks_for() NOT dispatching system messages.

    int doCallbacksFor (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer);
    int doSystemCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer,
                        void * userdata);
    int doSystemCallbacksFor
                       (vrpn_HANDLERPARAM p,
                        void * userdata);

    void clear (void);

  protected:

    struct vrpnLocalMapping {
      char                      * name;         // Name of type
      vrpnMsgCallbackEntry      * who_cares;    // Callbacks
      vrpn_int32                cCares;         // TCH 28 Oct 97
    };

    int d_numTypes;
    vrpnLocalMapping d_types [vrpn_CONNECTION_MAX_TYPES];

    int d_numSenders;
    char * d_senders [vrpn_CONNECTION_MAX_SENDERS];

    vrpn_MESSAGEHANDLER d_systemMessages [vrpn_CONNECTION_MAX_TYPES];

    vrpnMsgCallbackEntry * d_genericCallbacks;
};



vrpn_TypeDispatcher::vrpn_TypeDispatcher (void) :
    d_numTypes (0),
    d_numSenders (0),
    d_genericCallbacks (NULL)
{
  int i;
  // Make all of the names NULL pointers so they get allocated later
  for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
    d_senders[i] = NULL;
  }

  // Clear out any entries in the table.
  clear();
}

vrpn_TypeDispatcher::~vrpn_TypeDispatcher (void) {
  vrpnMsgCallbackEntry * pVMCB, * pVMCB_Del;
  int i;

  for (i = 0; i < d_numTypes; i++) {
    if (d_types[i].name) {
      delete [] d_types[i].name;
    }
    pVMCB = d_types[i].who_cares;
    while (pVMCB) {
      pVMCB_Del = pVMCB;
      pVMCB = pVMCB_Del->next;
      delete pVMCB_Del;
    }
  }

  pVMCB = d_genericCallbacks;
   
  while (pVMCB) {
    pVMCB_Del = pVMCB;
    pVMCB = pVMCB_Del->next;
    delete pVMCB_Del;
  }

  // Clear out any entries in the table.
  clear();
}

int vrpn_TypeDispatcher::numTypes (void) const {
  return d_numTypes;
}

const char * vrpn_TypeDispatcher::typeName (int i) const {
  if ((i < 0) || (i >= d_numTypes)) {
    return NULL;
  }
  return d_types[i].name;
}

vrpn_int32 vrpn_TypeDispatcher::getTypeID (const char * name) {
  vrpn_int32 i;

  for (i = 0; i < d_numTypes; i++) {
    if (!strcmp(name, d_types[i].name)) {
      return i;
    }
  }

  return -1;
}

int vrpn_TypeDispatcher::numSenders (void) const {
  return d_numSenders;
}

const char * vrpn_TypeDispatcher::senderName (int i) const {
  if ((i < 0) || (i >= d_numSenders)) {
    return NULL;
  }
  return d_senders[i];
}

vrpn_int32 vrpn_TypeDispatcher::getSenderID (const char * name) {
  vrpn_int32 i;

  for (i = 0; i < d_numSenders; i++) {
    if (!strcmp(name, d_senders[i])) {
      return i;
    }
  }

  return -1;
}

vrpn_int32 vrpn_TypeDispatcher::addType (const char * name) {

  // See if there are too many on the list.  If so, return -1.
  if (d_numTypes >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::addType:  "
                    "Too many! (%d)\n", d_numTypes);
    return -1;
  }

  if (!d_types[d_numTypes].name) {
    d_types[d_numTypes].name = new cName;
       if (!d_types[d_numTypes].name) {
         fprintf(stderr, "vrpn_TypeDispatcher::addType:  "
                         "Can't allocate memory for new record.\n");
       return -1;
    }
  }

  // Add this one into the list and return its index
  strncpy(d_types[d_numTypes].name, name, sizeof(cName) - 1);
  d_types[d_numTypes].who_cares = NULL;
  d_types[d_numTypes].cCares = 0;
  d_numTypes++;

  return d_numTypes - 1;
}

vrpn_int32 vrpn_TypeDispatcher::addSender (const char * name) {

  // See if there are too many on the list.  If so, return -1.
  if (d_numSenders >= vrpn_CONNECTION_MAX_SENDERS) {
    fprintf(stderr, "vrpn_TypeDispatcher::addSender:  "
                    "Too many! (%d).\n", d_numSenders);
    return -1;
  }

  if (!d_senders[d_numSenders]) {

    //  fprintf(stderr, "Allocating a new name entry\n");

    d_senders[d_numSenders] = new cName;
    if (!d_senders[d_numSenders]) {
      fprintf(stderr, "vrpn_TypeDispatcher::addSender:  "
                      "Can't allocate memory for new record\n");
      return -1;
    }
  }

  // Add this one into the list
  strncpy(d_senders[d_numSenders], name, sizeof(cName) - 1);
  d_numSenders++;

  // One more in place -- return its index
  return d_numSenders - 1;
}

vrpn_int32 vrpn_TypeDispatcher::registerType (const char * name) {
  vrpn_int32 retval;

  // See if the name is already in the list.  If so, return it.
  retval = getTypeID(name);
  if (retval != -1) {
    return retval;
  }

  return addType(name);
}

vrpn_int32 vrpn_TypeDispatcher::registerSender (const char * name) {
  vrpn_int32 retval;

  // See if the name is already in the list.  If so, return it.
  retval = getSenderID(name);
  if (retval != -1) {
    return retval;
  }

  return addSender(name);
}



int vrpn_TypeDispatcher::addHandler (vrpn_int32 type,
                                     vrpn_MESSAGEHANDLER handler,
                                     void * userdata, vrpn_int32 sender) {
  vrpnMsgCallbackEntry * new_entry;
  vrpnMsgCallbackEntry ** ptr;

  // Ensure that the type is a valid one (one that has been defined)
  //   OR that it is "any"
  if (((type < 0) || (type >= d_numTypes)) &&
      (type != vrpn_ANY_TYPE)) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  No such type\n");
          return -1;
  }

  // Ensure that the sender is a valid one (or "any")
  if ((sender != vrpn_ANY_SENDER) &&
      ((sender < 0) || (sender >= d_numSenders))) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  No such sender\n");
          return -1;
  }

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
          fprintf(stderr,
                  "vrpn_TypeDispatcher::addHandler:  NULL handler\n");
          return -1;
  }

  // Allocate and initialize the new entry
  new_entry = new vrpnMsgCallbackEntry ();
  if (new_entry == NULL) {
    fprintf(stderr, "vrpn_TypeDispatcher::addHandler:  Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;
  new_entry->sender = sender;

#ifdef  VERBOSE
  printf("Adding user handler for type %ld, sender %ld\n",type,sender);
#endif

  // TCH June 2000 - rewrote to insert at end of list instead of beginning,
  // to make sure multiple callbacks on the same type are triggered
  // in the order registered.  Note that multiple entries with the same
  // info is okay.

  if (type == vrpn_ANY_TYPE) {
    ptr = &d_genericCallbacks;
  } else {
    ptr = &d_types[type].who_cares;
  }

  while (*ptr) {
    ptr = &((*ptr)->next);
  }
  *ptr = new_entry;
  new_entry->next = NULL;

  return 0;
}

int vrpn_TypeDispatcher::removeHandler (vrpn_int32 type,
                                        vrpn_MESSAGEHANDLER handler,
                                        void * userdata, vrpn_int32 sender) {
  // The pointer at *snitch points to victim
  vrpnMsgCallbackEntry * victim, ** snitch;

  // Ensure that the type is a valid one (one that has been defined)
  //   OR that it is "any"
  if (((type < 0) || (type >= d_numTypes)) &&
      (type != vrpn_ANY_TYPE)) {
          fprintf(stderr, "vrpn_TypeDispatcher::removeHandler: No such type\n");
          return -1;
  }

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  if (type == vrpn_ANY_TYPE) {
    snitch = &d_genericCallbacks;
  } else {
    snitch = &(d_types[type].who_cares);
  }
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
              "vrpn_TypeDispatcher::removeHandler: No such handler\n");
          return -1;
  }

  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;

  return 0;
}

void vrpn_TypeDispatcher::setSystemHandler (vrpn_int32 type,
                                           vrpn_MESSAGEHANDLER handler) {
  d_systemMessages[-type] = handler;
}



int vrpn_TypeDispatcher::doCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer) {
  vrpnMsgCallbackEntry * who;
  vrpn_HANDLERPARAM p;

  // We don't dispatch system messages (kluge?).
  if (type < 0) {
    return 0;
  }

  if (type >= d_numTypes) {
    return -1;
  }

  // Fill in the parameter to be passed to the routines
  p.type = type;
  p.sender = sender;
  p.msg_time = time;
  p.payload_len = len;
  p.buffer = buffer;

  // Do generic callbacks (vrpn_ANY_TYPE)
  who = d_genericCallbacks;

  while (who) {   // For each callback entry
    // Verify that the sender is ANY or matches
    if ((who->sender == vrpn_ANY_SENDER) ||
        (who->sender == sender) ) {
      if (who->handler(who->userdata, p)) {
        fprintf(stderr, "vrpn_TypeDispatcher::doCallbacksFor:  "
                        "Nonzero user generic handler return.\n");
              return -1;
      }
    }

    // Next callback in list
    who = who->next;
  }

  // Find the head for the list of callbacks to call
  who = d_types[type].who_cares;
  while (who) {   // For each callback entry
    // Verify that the sender is ANY or matches
    if ((who->sender == vrpn_ANY_SENDER) ||
        (who->sender == sender) ) {
      if (who->handler(who->userdata, p)) {
        fprintf(stderr, "vrpn_TypeDispatcher::doCallbacksFor:  "
                        "Nonzero user handler return.\n");
              return -1;
      }
    }

    // Next callback in list
    who = who->next;
  }

  return 0;
}

int vrpn_TypeDispatcher::doSystemCallbacksFor
                       (vrpn_int32 type, vrpn_int32 sender,
                        timeval time, vrpn_uint32 len,
                        const char * buffer,
                        void * userdata) {
  vrpn_HANDLERPARAM p;

  if (type >= 0) {
    return 0;
  }
  if (-type >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Illegal type %d.\n", type);
    return -1;
  }

  if (!d_systemMessages[-type]) {
    return 0;
  }

  // Fill in the parameter to be passed to the routines
  p.type = type;
  p.sender = sender;
  p.msg_time = time;
  p.payload_len = len;
  p.buffer = buffer;

  return doSystemCallbacksFor(p, userdata);
}

int vrpn_TypeDispatcher::doSystemCallbacksFor
                       (vrpn_HANDLERPARAM p,
                        void * userdata) {
  int retval;

  if (p.type >= 0) {
    return 0;
  }
  if (-p.type >= vrpn_CONNECTION_MAX_TYPES) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Illegal type %d.\n", p.type);
    return -1;
  }

  if (!d_systemMessages[-p.type]) {
    return 0;
  }

  retval = d_systemMessages[-p.type](userdata, p);
  if (retval) {
    fprintf(stderr, "vrpn_TypeDispatcher::doSystemCallbacksFor:  "
                    "Nonzero system handler return.\n");
    return -1;
  }
  return 0;
}

void vrpn_TypeDispatcher::clear (void) {
  int i;

  for (i = 0; i < vrpn_CONNECTION_MAX_TYPES; i++) {
    d_types[i].who_cares = NULL;
    d_types[i].cCares = 0;
    d_types[i].name = NULL;

    d_systemMessages[i] = NULL;
  }

  for (i = 0; i < vrpn_CONNECTION_MAX_SENDERS; i++) {
    if (d_senders[i] != NULL) { delete [] d_senders[i]; }
    d_senders[i] = NULL;
  }
}


vrpn_ConnectionManager::~vrpn_ConnectionManager (void) {

  //fprintf(stderr, "In ~vrpn_ConnectionManager:  tearing down the list.\n");

  // Call the destructor of every known connection.
  // That destructor will call vrpn_ConnectionManager::deleteConnection()
  // to remove itself from d_kcList.
  while (d_kcList) {
    delete d_kcList->connection;
  }
  while (d_anonList) {
    delete d_anonList->connection;
  }

}

// static
vrpn_ConnectionManager & vrpn_ConnectionManager::instance (void) {
  static vrpn_ConnectionManager manager;
  return manager;
}

void vrpn_ConnectionManager::addConnection (vrpn_Connection * c,
                                            const char * name)
{
  knownConnection * p;

  p = new knownConnection;
  p->connection = c;

  if (name) {
    strncpy(p->name, name, 1000);
    p->next = d_kcList;
    d_kcList = p;
  } else {
    p->name[0] = 0;
    p->next = d_anonList;
    d_anonList = p;
  }
}

void vrpn_ConnectionManager::deleteConnection (vrpn_Connection * c)
{
  deleteConnection(c, &d_kcList);
  deleteConnection(c, &d_anonList);
}

void vrpn_ConnectionManager::deleteConnection (vrpn_Connection * c,
                                               knownConnection ** snitch) {
  knownConnection * victim = *snitch;

  while (victim && (victim->connection != c)) {
    snitch = &((*snitch)->next);
    victim = *snitch;
  }

  if (!victim) {
    // No warning, because this connection might be on the *other* list.
  } else {
    *snitch = victim->next;
    delete victim;
  }
}

vrpn_Connection * vrpn_ConnectionManager::getByName (const char * name) {
  knownConnection * p;
  for (p = d_kcList; p && strcmp(p->name, name); p = p->next) {
    // do nothing
  }
  if (!p) {
    return NULL;
  }
  return p->connection;
}

vrpn_ConnectionManager::vrpn_ConnectionManager (void) :
    d_kcList (NULL),
    d_anonList (NULL)
{

}


/**
 *   This function returns the host IP address in string form.  For example,
 * the machine "ioglab.cs.unc.edu" becomes "152.2.130.90."  This is done
 * so that the remote host can connect back even if it can't resolve the
 * DNS name of this host.  This is especially useful at conferences, where
 * DNS may not even be running.
 *   If the NIC_IP name is passed in as NULL but the SOCKET passed in is
 * valid, then look up the address associated with that socket; this is so
 * that when a machine has multiple NICs, it will send the outgoing request
 * for UDP connections to the same place that its TCP connection is on.
 */

static int	vrpn_getmyIP (char * myIPchar, unsigned maxlen,
                              const char * NIC_IP = NULL,
                              SOCKET incoming_socket = INVALID_SOCKET)
{	
  char myname [100];		// Host name of this host
  struct hostent * host;        // Encoded host IP address, etc.
  char myIPstring [100];	// Hold "152.2.130.90" or whatever

  if (myIPchar == NULL) {
    fprintf(stderr,"vrpn_getmyIP: NULL pointer passed in\n");
    return -1;
  }

  // If we have a specified NIC_IP address, fill it in and return it.
  if (NIC_IP) {
    if (strlen(NIC_IP) > maxlen) {
      fprintf(stderr,"vrpn_getmyIP: Name too long to return\n");
      return -1;
    }
#ifdef VERBOSE
    fprintf(stderr, "Was given IP address of %s so returning that.\n",
            NIC_IP);
#endif
    strncpy(myIPchar, NIC_IP, maxlen);
    return 0;
  }

  // If we have a valid specified SOCKET, then look up its address and
  // return it.
  if (incoming_socket != INVALID_SOCKET) {
    struct sockaddr_in socket_name;
    int socket_namelen = sizeof(socket_name);

    if (getsockname(incoming_socket, (struct sockaddr *) &socket_name,
                    GSN_CAST &socket_namelen)) {
      fprintf(stderr, "vrpn_getmyIP: cannot get socket name.\n");
      return -1;
    }

    sprintf(myIPstring, "%u.%u.%u.%u",
  	  ntohl(socket_name.sin_addr.s_addr) >> 24,
          (ntohl(socket_name.sin_addr.s_addr) >> 16) & 0xff,
          (ntohl(socket_name.sin_addr.s_addr) >> 8) & 0xff,
          ntohl(socket_name.sin_addr.s_addr) & 0xff);

    // Copy this to the output
    if ((int)strlen(myIPstring) > maxlen) {
      fprintf(stderr,"vrpn_getmyIP: Name too long to return\n");
      return -1;
    }

    strcpy(myIPchar, myIPstring);

#ifdef VERBOSE
    fprintf(stderr, "Decided on IP address of %s.\n", myIPchar);
#endif
    return 0;
  }

  // Find out what my name is
  // gethostname() is guaranteed to produce something gethostbyname() can parse. 
  if (gethostname(myname, sizeof(myname))) {
    fprintf(stderr, "vrpn_getmyIP: Error finding local hostname\n");
    return -1;
  }

  // Find out what my IP address is
  host = gethostbyname(myname);
  if (host == NULL) {
    fprintf(stderr, "vrpn_getmyIP: error finding host by name (%s)\n",myname);
    return -1;
  }

  // Convert this back into a string
#ifndef CRAY
  if (host->h_length != 4) {
    fprintf(stderr, "vrpn_getmyIP: Host length not 4\n");
    return -1;
  }
#endif
  sprintf(myIPstring, "%u.%u.%u.%u",
  	(unsigned int)(unsigned char)host->h_addr_list[0][0],
  	(unsigned int)(unsigned char)host->h_addr_list[0][1],
  	(unsigned int)(unsigned char)host->h_addr_list[0][2],
  	(unsigned int)(unsigned char)host->h_addr_list[0][3]);

  // Copy this to the output
  if ((int)strlen(myIPstring) > maxlen) {
    fprintf(stderr,"vrpn_getmyIP: Name too long to return\n");
    return -1;
  }

  strcpy(myIPchar, myIPstring);
#ifdef VERBOSE
  fprintf(stderr, "Decided on IP address of %s.\n", myIPchar);
#endif
  return 0;
}


/**
 *	This routine will perform like a normal select() call, but it will
 * restart if it quit because of an interrupt.  This makes it more robust
 * in its function, and allows this code to perform properly on pxpl5, which
 * sends USER1 interrupts while rendering an image.
 */
int vrpn_noint_select(int width, fd_set *readfds, fd_set *writefds, 
		     fd_set *exceptfds, struct timeval * timeout)
{
	fd_set	tmpread, tmpwrite, tmpexcept;
	int	ret;
	int	done = 0;
        struct	timeval timeout2;
        struct	timeval *timeout2ptr;
	struct	timeval	start, stop, now;

	/* If the timeout parameter is non-NULL and non-zero, then we
	 * may have to adjust it due to an interrupt.  In these cases,
	 * we will copy the timeout to timeout2, which will be used
	 * to keep track.  Also, the stop time is calculated so that
         * we can know when it is time to bail. */
	if ( (timeout != NULL) && 
	     ((timeout->tv_sec != 0) || (timeout->tv_usec != 0)) ) {
		timeout2 = *timeout;
		timeout2ptr = &timeout2;
		vrpn_gettimeofday(&start, NULL);	/* Find start time */
		stop = vrpn_TimevalSum(start, *timeout);/* Find stop time */
	} else {
		timeout2ptr = timeout;
                stop.tv_sec = 0;
                stop.tv_usec = 0;
	}

	/* Repeat selects until it returns for a reason other than interrupt */
	do {
		/* Set the temp file descriptor sets to match parameters each time through. */
		if (readfds != NULL) {
			tmpread = *readfds;
		} else {
			FD_ZERO(&tmpread);
		}
		if (writefds != NULL) {
			tmpwrite = *writefds;
		} else {
			FD_ZERO(&tmpwrite);
		}
		if (exceptfds != NULL) {
			tmpexcept = *exceptfds;
		} else {
			FD_ZERO(&tmpexcept);
		}

		/* Do the select on the temporary sets of descriptors */
		ret = select(width, &tmpread,&tmpwrite,&tmpexcept, timeout2ptr);
		if (ret >= 0) {	/* We are done if timeout or found some */
			done = 1;
		} else if (errno != EINTR) {	/* Done if non-intr error */
			done = 1;
		} else if ( (timeout != NULL) &&
		  ((timeout->tv_sec != 0) || (timeout->tv_usec != 0))) {

		    /* Interrupt happened.  Find new time timeout value */
		    vrpn_gettimeofday(&now, NULL);
		    if (vrpn_TimevalGreater(now, stop)) {/* Past stop time */
			done = 1;
		    } else {	/* Still time to go. */
			unsigned long	usec_left;
			usec_left = (stop.tv_sec - now.tv_sec) * 1000000L;
			usec_left += stop.tv_usec - now.tv_usec;
			timeout2.tv_sec = usec_left / 1000000L;
			timeout2.tv_usec = usec_left % 1000000L;
		    }
		}
	} while (!done);

	/* Copy the temporary sets back to the parameter sets */
        if (readfds != NULL) { *readfds = tmpread; }
	if (writefds != NULL) {	*writefds = tmpwrite; }
        if (exceptfds != NULL) { *exceptfds = tmpexcept; }

	return(ret);
}


/**
 *      This routine will write a block to a file descriptor.  It acts just
 * like the write() system call does on files, but it will keep sending to
 * a socket until an error or all of the data has gone.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the write when they occur.  It will also work when
 * sending large blocks of data through socket connections, since it will
 * send all of the data before returning.
 *	This routine will either write the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data is sent).
 */

#ifndef VRPN_USE_WINSOCK_SOCKETS

int vrpn_noint_block_write (int outfile, const char buffer[], int length)
{
        register int    sofar;          /* How many characters sent so far */
        register int    ret;            /* Return value from write() */

        sofar = 0;
        do {
                /* Try to write the remaining data */
                ret = write(outfile, buffer+sofar, length-sofar);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }

        } while ( (ret > 0) && (sofar < length) );

        if (ret == -1) return(-1);	/* Error during write */
	if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes written */
}


/**
 *      This routine will read in a block from the file descriptor.
 * It acts just like the read() routine does on normal files, so that
 * it hides the fact that the descriptor may point to a socket.
 *      This will also take care of problems caused by interrupted system
 * calls, retrying the read when they occur.
 *	This routine will either read the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data arrives).
 */

int vrpn_noint_block_read(int infile, char buffer[], int length)
{
        register int    sofar;          /* How many we read so far */
        register int    ret;            /* Return value from the read() */

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }
        sofar = 0;
        do {
		/* Try to read all remaining data */
                ret = read(infile, buffer+sofar, length-sofar);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }
        } while ((ret > 0) && (sofar < length));

        if (ret == -1) return(-1);	/* Error during read */
	if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes read */
}

#else /* winsock sockets */

int vrpn_noint_block_write(SOCKET outsock, char *buffer, int length)
{
	int nwritten, sofar = 0;
	do {
		    /* Try to write the remaining data */
		nwritten = send(outsock, buffer+sofar, length-sofar, 0);

		if (nwritten == SOCKET_ERROR) {
			return -1;
        }

		sofar += nwritten;		
	} while ( sofar < length );
	
	return(sofar);			/* All bytes written */
}

int vrpn_noint_block_read(SOCKET insock, char *buffer, int length)
{
    int nread, sofar = 0;  

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }

    do {
            /* Try to read all remaining data */
        nread = recv(insock, buffer+sofar, length-sofar, 0);

		if (nread == SOCKET_ERROR) {
            return -1;
        }
		if (nread == 0) {   /* socket closed */
			return 0;
		}
        
        sofar += nread;        
    } while (sofar < length);
	    
    return(sofar);			/* All bytes read */
}


#endif /* VRPN_USE_WINSOCK_SOCKETS */


/**
 *   This routine will read in a block from the file descriptor.
 * It acts just like the read() routine on normal files, except that
 * it will time out if the read takes too long.
 *   This will also take care of problems caused by interrupted system
 * calls, retrying the read when they occur.
 *   This routine will either read the requested number of bytes and
 * return that or return -1 (in the case of an error) or 0 (in the case
 * of EOF being reached before all the data arrives), or return the number
 * of characters read before timeout (in the case of a timeout).
 */

#ifdef VRPN_USE_WINSOCK_SOCKETS
int vrpn_noint_block_read_timeout(SOCKET infile, char buffer[], 
				 int length, struct timeval *timeout)
#else
int vrpn_noint_block_read_timeout(int infile, char buffer[], 
				 int length, struct timeval *timeout)
#endif
{
        register int    sofar;          /* How many we read so far */
        register int    ret;            /* Return value from the read() */
        struct	timeval timeout2;
        struct	timeval *timeout2ptr;
	struct	timeval	start, stop, now;

  // TCH 4 Jan 2000 - hackish - Cygwin will block forever on a 0-length
  // read(), and from the man pages this is close enough to in-spec that
  // other OS may do the same thing.

  if (!length) {
    return 0;
  }

	/* If the timeout parameter is non-NULL and non-zero, then we
	 * may have to adjust it due to an interrupt.  In these cases,
	 * we will copy the timeout to timeout2, which will be used
	 * to keep track.  Also, the current time is found so that we
	 * can track elapsed time. */
	if ( (timeout != NULL) && 
	     ((timeout->tv_sec != 0) || (timeout->tv_usec != 0)) ) {
		timeout2 = *timeout;
		timeout2ptr = &timeout2;
		vrpn_gettimeofday(&start, NULL);	/* Find start time */
		stop = vrpn_TimevalSum(start, *timeout);/* Find stop time */
	} else {
		timeout2ptr = timeout;
	}

        sofar = 0;
        do {	
		int	sel_ret;
		fd_set	readfds, exceptfds;

		/* See if there is a character ready for read */
		FD_ZERO(&readfds);
		FD_SET(infile, &readfds);
		FD_ZERO(&exceptfds);
		FD_SET(infile, &exceptfds);
		sel_ret = vrpn_noint_select(infile+1, &readfds, NULL, &exceptfds,
					   timeout2ptr);
		if (sel_ret == -1) {	/* Some sort of error on select() */
			return -1;
		}
		if (FD_ISSET(infile, &exceptfds)) {	/* Exception */
			return -1;
		}
		if (!FD_ISSET(infile, &readfds)) {	/* No characters */
			if ( (timeout != NULL) &&
			     (timeout->tv_sec == 0) &&
			     (timeout->tv_usec == 0) ) {	/* Quick poll */
				return sofar;	/* Timeout! */
			}
		}

		/* See what time it is now and how long we have to go */
		if (timeout2ptr) {
			vrpn_gettimeofday(&now, NULL);
			if (vrpn_TimevalGreater(now, stop)) {	/* Timeout! */
				return sofar;
			} else {
				timeout2 = vrpn_TimevalDiff(stop, now);
			}
		}

		if (!FD_ISSET(infile, &readfds)) {	/* No chars yet */
			ret = 0;
			continue;
		}

#ifndef VRPN_USE_WINSOCK_SOCKETS
                ret = read(infile, buffer+sofar, length-sofar);
                sofar += ret;

                /* Ignore interrupted system calls - retry */
                if ( (ret == -1) && (errno == EINTR) ) {
                        ret = 1;	/* So we go around the loop again */
                        sofar += 1;	/* Restoring it from above -1 */
                }
#else
                {
                    int nread;
                    nread = recv(infile, buffer+sofar, length-sofar, 0);
                    sofar += nread;
                    ret = nread;
                }
#endif

        } while ((ret > 0) && (sofar < length));
#ifndef VRPN_USE_WINSOCK_SOCKETS
        if (ret == -1) return(-1);	/* Error during read */
#endif
        if (ret == 0) return(0);	/* EOF reached */

        return(sofar);			/* All bytes read */
}

/**
 * This routine opens a socket with the requested port number.
 * The routine returns -1 on failure and the file descriptor on success.
 * The portno parameter is filled in with the actual port that is opened
 * (this is useful when the address INADDR_ANY is used, since it tells
 * what port was opened).
 * To select between multiple NICs, we can specify the IP address of the
 * socket to open;  NULL selects the default NIC.
 */

static SOCKET open_socket (int type,
                           unsigned short * portno,
                           const char * IPaddress) {
  SOCKET sock;
  struct sockaddr_in name;
  struct hostent * phe;           /* pointer to host information entry   */
  int	namelen;

  // create an Internet socket of the appropriate type
  sock = socket(AF_INET, type, 0);
  if (sock == -1)  {
    fprintf(stderr, "open_socket: can't open socket.\n");
#ifndef _WIN32_WCE
    fprintf(stderr, "  -- errno %d (%s).\n", errno, strerror(errno));
#endif
    return (SOCKET) -1;
  }

  namelen = sizeof(name);

  // bind to local address
  memset((void *) &name, 0, namelen);
  name.sin_family = AF_INET;
  if (portno) {
    name.sin_port = htons(*portno);
  } else {
    name.sin_port = htons(0);
  }

  // Map our host name to our IP address, allowing for dotted decimal
  if (!IPaddress) {
    name.sin_addr.s_addr = INADDR_ANY;
  } else if ((name.sin_addr.s_addr = inet_addr(IPaddress))
              == INADDR_NONE) {
      if ( (phe = gethostbyname(IPaddress)) != NULL) {
          memcpy((void *) &name.sin_addr, (const void *) phe->h_addr, phe->h_length);
      } else {
          fprintf(stderr, "open_socket:  can't get %s host entry\n", IPaddress);
          return (SOCKET) -1;
      }
  }

#ifdef VERBOSE3
  //NIC will be 0.0.0.0 if we use INADDR_ANY 
fprintf(stderr, "open_socket:  request port %d, using NIC %d %d %d %d.\n",
portno ? *portno : 0 , ntohl(name.sin_addr.s_addr) >> 24,
(ntohl(name.sin_addr.s_addr) >> 16) & 0xff,
(ntohl(name.sin_addr.s_addr) >> 8) & 0xff,
ntohl(name.sin_addr.s_addr) & 0xff);
#endif

  if (bind(sock, (struct sockaddr *) &name, namelen) < 0){
    fprintf(stderr, "open_socket:  can't bind address %d", *portno);
#ifndef _WIN32_WCE
    fprintf(stderr, "  --  %d  --  %s\n", errno, strerror(errno));
#endif
    fprintf(stderr,"  (This probably means that another application has the port open already)\n");
    return (SOCKET) -1;
  }

  // Find out which port was actually bound
  if (getsockname(sock, (struct sockaddr *) &name, GSN_CAST &namelen)) {
    fprintf(stderr, "vrpn: open_socket: cannot get socket name.\n");
    return (SOCKET) -1;
  }
  if (portno) {
    *portno = ntohs(name.sin_port);
  }

#ifdef VERBOSE3
//NIC will be 0.0.0.0 if we use INADDR_ANY 
fprintf(stderr, "open_socket:  got port %d, using NIC %d %d %d %d.\n",
portno ? *portno : ntohs(name.sin_port), ntohl(name.sin_addr.s_addr) >> 24,
(ntohl(name.sin_addr.s_addr) >> 16) & 0xff,
(ntohl(name.sin_addr.s_addr) >> 8) & 0xff,
ntohl(name.sin_addr.s_addr) & 0xff);
#endif

  return sock;
}

/**
 * Create a UDP socket and bind it to its local address.
 */

static SOCKET open_udp_socket (unsigned short * portno,
                               const char * IPaddress) {
  return open_socket(SOCK_DGRAM, portno, IPaddress);
}

/**
 * Create a TCP socket and bind it to its local address.
 */

static SOCKET open_tcp_socket (unsigned short * portno = NULL,
                               const char * NIC_IP = NULL) {
  return open_socket(SOCK_STREAM, portno, NIC_IP);
}




/**
 * Create a UDP socket and connect it to a specified port.
 */

static SOCKET vrpn_connect_udp_port
                       (const char * machineName, int remotePort,
                        const char * NIC_IP = NULL) {
  SOCKET udp_socket;
  struct sockaddr_in udp_name;
  struct hostent * remoteHost;
  int udp_namelen;

  udp_socket = open_udp_socket(NULL, NIC_IP);

  udp_namelen = sizeof(udp_name);

  memset((void *) &udp_name, 0, udp_namelen);
  udp_name.sin_family = AF_INET;

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name. 
// MS Documentation says it will always fail and inet_addr should 
// be called first. Avoids a 30+ second wait for
// gethostbyname() to fail.

  if ((udp_name.sin_addr.s_addr = inet_addr(machineName))
              == INADDR_NONE) {
      remoteHost = gethostbyname(machineName);
      if (remoteHost) {

#ifdef CRAY
          int i;
          u_long foo_mark = 0L;
          for  (i = 0; i < 4; i++) {
              u_long one_char = remoteHost->h_addr_list[0][i];
              foo_mark = (foo_mark << 8) | one_char;
          }
          udp_name.sin_addr.s_addr = foo_mark;
#else
          memcpy(&(udp_name.sin_addr.s_addr), remoteHost->h_addr,  remoteHost->h_length);
#endif

      } else {
          vrpn_closeSocket(udp_socket);
          fprintf(stderr,
                  "vrpn_connect_udp_port: error finding host by name (%s).\n",machineName);
          return (SOCKET) (-1);
      }
  }
#ifndef VRPN_USE_WINSOCK_SOCKETS
  udp_name.sin_port = htons(remotePort);
#else
  udp_name.sin_port = htons((u_short)remotePort);
#endif

  if ( connect(udp_socket,(struct sockaddr *) &udp_name,udp_namelen) ) {
      fprintf(stderr,"vrpn_connect_udp_port: can't bind udp socket.\n");
      vrpn_closeSocket(udp_socket);
      return (SOCKET) (-1);
  }

  // Find out which port was actually bound
  udp_namelen = sizeof(udp_name);
  if (getsockname(udp_socket, (struct sockaddr *) &udp_name,
                  GSN_CAST &udp_namelen)) {
    fprintf(stderr, "vrpn_connect_udp_port: cannot get socket name.\n");
    return (SOCKET) -1;
  }

#ifdef VERBOSE3
  // NOTE NIC will be 0.0.0.0 if we listen on all NICs. 
fprintf(stderr, "vrpn_connect_udp_port:  got port %d, using NIC %d %d %d %d.\n",
ntohs(udp_name.sin_port), ntohl(udp_name.sin_addr.s_addr) >> 24,
(ntohl(udp_name.sin_addr.s_addr) >> 16) & 0xff,
(ntohl(udp_name.sin_addr.s_addr) >> 8) & 0xff,
ntohl(udp_name.sin_addr.s_addr) & 0xff);
#endif

  return udp_socket;
}




/**
 * This section deals with implementing a method of connection termed a
 * UDP request.  This works by having the client open a TCP socket that
 * it listens on. It then lobs datagrams to the server asking to be
 * called back at the socket. This allows it to timeout on waiting for
 * a connection request, resend datagrams in case some got lost, or give
 * up at any time. The whole algorithm is implemented in the
 * vrpn_udp_request_call() function; the functions before that are helper
 * functions that have been broken out to allow a subset of the algorithm
 * to be run by a connection whose server has dropped and they want to
 * re-establish it.
 *
 * This routine will lob a datagram to the given port on the given
 * machine asking it to call back at the port on this machine that
 * is also specified. It returns 0 on success and -1 on failure.
 */

int vrpn_udp_request_lob_packet(
		const char * machine,	// Name of the machine to call
		const int remote_port,	// UDP port on remote machine
		const int local_port,	// TCP port on this machine
                const char * NIC_IP = NULL)
{
	SOCKET	udp_sock;		/* We lob datagrams from here */
	char	msg[150];		/* Message to send */
	vrpn_int32	msglen;		/* How long it is (including \0) */
	char	myIPchar[100];		/* IP decription this host */

  /* Create a UDP socket and connect it to the port on the remote
   * machine. */

  udp_sock = vrpn_connect_udp_port (machine, remote_port, NIC_IP);

  /* Fill in the request message, telling the machine and port that
   * the remote server should connect to.  These are ASCII, separated
   * by a space.  vrpn_getmyIP returns the NIC_IP if it is not null,
   * or the host name of this machine using gethostname() if it is
   * NULL.  If the NIC_IP is NULL but we have a socket (as we do here),
   * then it returns the address associated with the socket.
   */
  if (vrpn_getmyIP(myIPchar, sizeof(myIPchar), NIC_IP, udp_sock)) {
    fprintf(stderr,
       "vrpn_udp_request_lob_packet: Error finding local hostIP\n");
    vrpn_closeSocket(udp_sock);
    return(-1);
  }
  sprintf(msg, "%s %d", myIPchar, local_port);
  msglen = strlen(msg) + 1;	/* Include the terminating 0 char */

  // Lob the message
  if (send(udp_sock, msg, msglen, 0) == -1) {
    perror("vrpn_udp_request_lob_packet: send() failed");
    vrpn_closeSocket(udp_sock);
    return -1;
  }

  vrpn_closeSocket(udp_sock);	// We're done with the port
  return 0;
}

/**
 * This routine will get a TCP socket that is ready to accept connections.
 * That is, listen() has already been called on it.
 * It will get whatever socket is available from the system. It returns
 * 0 on success and -1 on failure. On success, it fills in the pointers to
 * the socket and the port number of the socket that it obtained.
 * To select between multiple network interfaces, we can specify an IPaddress;
 * the default value is NULL, which uses the default NIC.
 */

static
int vrpn_get_a_TCP_socket (SOCKET * listen_sock, int * listen_portnum,
                           const char * NIC_IP = NULL)
{
  struct sockaddr_in listen_name;	/* The listen socket binding name */
  int	listen_namelen;

  listen_namelen = sizeof(listen_name);

  /* Create a TCP socket to listen for incoming connections from the
   * remote server. */

  *listen_sock = open_tcp_socket(NULL, NIC_IP);
  if (*listen_sock < 0) {
    fprintf(stderr, "vrpn_get_a_TCP_socket:  socket didn't open.\n");
    return -1;
  }

  if (listen(*listen_sock, 1) ) {
    fprintf(stderr,"vrpn_get_a_TCP_socket: listen() failed.\n");
    vrpn_closeSocket(*listen_sock);
    return(-1);
  }

  if (getsockname(*listen_sock,
                  (struct sockaddr *) &listen_name,
                  GSN_CAST &listen_namelen)) {
    fprintf(stderr, "vrpn_get_a_TCP_socket: cannot get socket name.\n");
    vrpn_closeSocket(*listen_sock);
    return(-1);
  }

  *listen_portnum = ntohs(listen_name.sin_port);

//fprintf(stderr, "Listening on port %d, address %d %d %d %d.\n",
//*listen_portnum, listen_name.sin_addr.s_addr >> 24,
//(listen_name.sin_addr.s_addr >> 16) & 0xff,
//(listen_name.sin_addr.s_addr >> 8) & 0xff,
//listen_name.sin_addr.s_addr & 0xff);

	return 0;
}

/**
 * This routine will check the listen socket to see if there has been a
 * connection request. If so, it will accept a connection on the accept
 * socket and set TCP_NODELAY on that socket. The attempt will timeout
 * in the amount of time specified.  If the accept and set are
 * successfull, it returns 1. If there is nothing asking for a connection,
 * it returns 0. If there is an error along the way, it returns -1.
 */

static
int vrpn_poll_for_accept(SOCKET listen_sock, SOCKET *accept_sock, double timeout = 0.0)
{
	fd_set	rfds;
	struct	timeval t;

	// See if we have a connection attempt within the timeout
	FD_ZERO(&rfds);
	FD_SET(listen_sock, &rfds);	/* Check for read (connect) */
	t.tv_sec = (long)(timeout);
	t.tv_usec = (long)( (timeout - t.tv_sec) * 1000000L );
	if (vrpn_noint_select(listen_sock+1, &rfds, NULL, NULL, &t) == -1) {
	  perror("vrpn_poll_for_accept: select() failed");
	  return -1;
	}
	if (FD_ISSET(listen_sock, &rfds)) {	/* Got one! */
	    /* Accept the connection from the remote machine and set TCP_NODELAY
	    * on the socket. */
	    if ( (*accept_sock = accept(listen_sock,0,0)) == -1 ) {
		perror("vrpn_poll_for_accept: accept() failed");
		return -1;
	    }
#ifndef	_WIN32_WCE
	    {	struct	protoent	*p_entry;
		int	nonzero = 1;

		if ( (p_entry = getprotobyname("TCP")) == NULL ) {
			fprintf(stderr,
			"vrpn_poll_for_accept: getprotobyname() failed.\n");
			vrpn_closeSocket(*accept_sock);
			return(-1);
		}
	
		if (setsockopt(*accept_sock, p_entry->p_proto,
		TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
			perror("vrpn_poll_for_accept: setsockopt() failed");
			vrpn_closeSocket(*accept_sock);
			return(-1);
		}
	    }
#endif
	    return 1;	// Got one!
	}

	return 0;	// Nobody called
}

// This is like sdi_start_server except that the convention for
// passing information on the client machine to the server program is 
// different; everything else has been left the same
/**
 *      This routine will start up a given server on a given machine.  The
 * name of the machine requesting service and the socket number to connect to
 * are supplied after the -client argument. The "args" parameters are
 * passed next.  They should be space-separated arguments, each of which is
 * passed separately.  It is NOT POSSIBLE to send parameters with embedded
 * spaces.
 *      This routine returns a file descriptor that points to the socket
 * to the server on success and -1 on failure.
 */
static
int vrpn_start_server(const char * machine, char * server_name, char * args,
                      const char * IPaddress = NULL)
{
#if defined(VRPN_USE_WINSOCK_SOCKETS) || defined(__CYGWIN__)
        fprintf(stderr,"VRPN: vrpn_start_server not ported"
                " for windows winsocks or cygwin!\n");
	IPaddress = IPaddress;
	args = args;
	server_name = server_name;
	machine = machine;
        return -1;
#else
        int     pid;    /* Child's process ID */
        int     server_sock;    /* Where the accept returns */
        int     child_socket;   /* Where the final socket is */
        int     PortNum;        /* Port number we got */

        /* Open a socket and ensure we can bind it */
	if (vrpn_get_a_TCP_socket(&server_sock, &PortNum, IPaddress)) {
		fprintf(stderr,"vrpn_start_server: Cannot get listen socket\n");
		return -1;
	}

        if ( (pid = fork()) == -1) {
                fprintf(stderr,"vrpn_start_server: cannot fork().\n");
                vrpn_closeSocket(server_sock);
                return(-1);
        }
        if (pid == 0) {  /* CHILD */
                int     loop;
                int     ret;
                int     num_descriptors;/* Number of available file descr */
                char    myIPchar[100];    /* Host name of this host */
                char    command[600];   /* Command passed to system() call */
                char    *rsh_to_use;    /* Full path to Rsh command. */

                if (vrpn_getmyIP(myIPchar,sizeof(myIPchar), IPaddress, server_sock)) {
                        fprintf(stderr,
                           "vrpn_start_server: Error finding my IP\n");
                        vrpn_closeSocket(server_sock);
                        return(-1);
                }

                /* Close all files except stdout and stderr. */
                /* This prevents a hung child from keeping devices open */
                num_descriptors = getdtablesize();
                for (loop = 0; loop < num_descriptors; loop++) {
                  if ( (loop != 1) && (loop != 2) ) {
                        close(loop);
                  }
                }

                /* Find the RSH command, either from the environment
                 * variable or the default, and use it to start up the
                 * remote server. */

                if ( (rsh_to_use =(char *)getenv("VRPN_RSH")) == NULL) {
                        rsh_to_use = RSH;
                }
                sprintf(command,"%s %s %s %s -client %s %d",rsh_to_use, machine,
                        server_name, args, myIPchar, PortNum);
                ret = system(command);
                if ( (ret == 127) || (ret == -1) ) {
                        fprintf(stderr,
                                "vrpn_start_server: system() failed !!!!!\n");
                        perror("Error");
                        fprintf(stderr, "Attempted command was: '%s'\n",
                                command);
                        vrpn_closeSocket(server_sock);
                        exit(-1);  /* This should never occur */
                }
                exit(0);

        } else {  /* PARENT */
                int     waitloop;

                /* Check to see if the child
                 * is trying to call us back.  Do SERVCOUNT waits, each of
                 * which is SERVWAIT long.
                 * If the child dies while we are waiting, then we can be
                 * sure that they will not be calling us back.  Check for
                 * this while waiting for the callback. */

                for (waitloop = 0; waitloop < (SERVCOUNT); waitloop++) {
		    int ret;
                    pid_t deadkid;
#if defined(sparc) || defined(FreeBSD) || defined(_AIX)
                    int status;  // doesn't exist on sparc_solaris or FreeBSD
#else
                    union wait status;
#endif
		    
                    /* Check to see if they called back yet. */
		    ret = vrpn_poll_for_accept(server_sock, &child_socket, SERVWAIT);
		    if (ret == -1) {
			    fprintf(stderr,"vrpn_start_server: Accept poll failed\n");
			    vrpn_closeSocket(server_sock);
			    return -1;
		    }
		    if (ret == 1) {
			    break;	// Got it!
		    }

                    /* Check to see if the child is dead yet */
#if defined(hpux) || defined(sgi) || defined(__hpux) || defined(__CYGWIN__) || defined(__APPLE__)
                    /* hpux include files have the wrong declaration */
                    deadkid = wait3((int*)&status, WNOHANG, NULL);
#else
                    deadkid = wait3(&status, WNOHANG, NULL);
#endif
                    if (deadkid == pid) {
                        fprintf(stderr,
                                "vrpn_start_server: server process exited\n");
                        vrpn_closeSocket(server_sock);
                        return(-1);
                    }
                }
		if (waitloop == SERVCOUNT) {
			fprintf(stderr,
                        "vrpn_start_server: server failed to connect in time\n");
                    fprintf(stderr,
                        "                  (took more than %d seconds)\n",
                        SERVWAIT*SERVCOUNT);
                    vrpn_closeSocket(server_sock);
                    kill(pid,SIGKILL);
                    wait(0);
                    return(-1);
                }

                vrpn_closeSocket(server_sock);
                return(child_socket);
        }
        return 0;
#endif
}


//**  End of section pulled from SDI library
//*********************************************************************

// COOKIE MANIPULATION

/**
 * Writes the magic cookie into buffer with given length.
 * On failure (if the buffer is too short), returns -1;
 * otherwise returns 0.
 *
 * This is exposed in vrpn_Connection.h so that we can write
 * add_vrpn_cookie.
 */

int write_vrpn_cookie (char * buffer, int length, long remote_log_mode)
{
  if (length < vrpn_MAGICLEN + vrpn_ALIGN + 1)
    return -1;

  sprintf(buffer, "%s  %c", vrpn_MAGIC, remote_log_mode + '0');
  return 0;
}

/**
 * Checks to see if the given buffer has the magic cookie.
 * Returns -1 on a mismatch, 0 on an exact match,
 * 1 on a minor version difference.
 */

int check_vrpn_cookie (const char * buffer)
{
  const char * bp;

  // Comparison changed 9 Feb 98 by TCH
  //   We don't care if the minor version numbers don't match,
  // so only check the characters through the last '.' in our
  // template.  (If there is no last '.' in our template, somebody's
  // modified this code to break the constraints above, and we just
  // use a maximally restrictive check.)
  //   XXX This pointer arithmetic isn't completely safe.

  bp = strrchr(buffer, '.');
  if (strncmp(buffer, vrpn_MAGIC,
      (bp == NULL ? vrpn_MAGICLEN : bp + 1 - buffer))) {
    fprintf(stderr, "check_vrpn_cookie:  "
            "bad cookie (wanted '%s', got '%s'\n", vrpn_MAGIC, buffer);
    return -1;
  }

  if (strncmp(buffer, vrpn_MAGIC, vrpn_MAGICLEN)) {
    fprintf(stderr, "check_vrpn_cookie(): "
        "VRPN Note: minor version number doesn't match: (prefer '%s', got '%s').  This is not normally a problem.\n",
	    vrpn_MAGIC, buffer);
    return 1;
  }

  return 0;
}


int check_vrpn_file_cookie (const char * buffer)
{
  const char * bp;

  // Comparison changed 9/1/00 by AAS and KTS
  // Here the difference is that we let the major version number be
  // less than or equal to our major version number as long as the major
  // version number is >= 4

  //   We don't care if the minor version numbers don't match,
  // so only check the characters through the last '.' in our
  // template.  (If there is no last '.' in our template, somebody's
  // modified this code to break the constraints above, and we just
  // use a maximally restrictive check.)
  //   XXX This pointer arithmetic isn't completely safe.

  bp = strrchr(buffer, '.');
  int majorComparison = strncmp(buffer, vrpn_MAGIC,
      (bp == NULL ? vrpn_MAGICLEN : bp + 1 - buffer));
  if (majorComparison > 0 || 
        strncmp(buffer, vrpn_FILE_MAGIC, 
	(bp == NULL ? vrpn_MAGICLEN : bp + 1 - buffer)) < 0) {
    fprintf(stderr, "check_vrpn_file_cookie:  "
            "bad cookie (wanted >='%s' and <='%s', "
            "got '%s'\n", vrpn_FILE_MAGIC, vrpn_MAGIC, buffer);
    return -1;
  }

  if (majorComparison == 0 &&
	strncmp(buffer, vrpn_MAGIC, vrpn_MAGICLEN)) {
    fprintf(stderr, "check_vrpn_file_cookie(): "
        "Note: Version number doesn't match: (prefer '%s', got '%s').  This is not normally a problem.\n",
	    vrpn_MAGIC, buffer);
    return 1;
  }

  return 0;
}

int vrpn_cookie_size (void) {
  return vrpn_MAGICLEN + vrpn_ALIGN;
}

// END OF COOKIE CODE


vrpn_Endpoint::vrpn_Endpoint (vrpn_TypeDispatcher * dispatcher,
                              vrpn_int32 * connectedEndpointCounter) :
    status(BROKEN),
    d_remoteLogMode (0),
    d_remoteInLogName (NULL),
    d_remoteOutLogName (NULL),
    d_inLog (NULL),
    d_outLog (NULL),
    d_senders (NULL),
    d_types (NULL),
    d_dispatcher (dispatcher),
    d_connectionCounter (connectedEndpointCounter)
{
  vrpn_Endpoint::init();
}

vrpn_Endpoint_IP::vrpn_Endpoint_IP (vrpn_TypeDispatcher * dispatcher,
                              vrpn_int32 * connectedEndpointCounter) :
    vrpn_Endpoint (dispatcher, connectedEndpointCounter),
    d_tcpSocket (INVALID_SOCKET),
    d_tcpListenSocket (INVALID_SOCKET),
    d_tcpListenPort (0),
    remote_machine_name (NULL),
    d_remote_port_number (0),
    d_udpOutboundSocket (INVALID_SOCKET),
    d_udpInboundSocket (INVALID_SOCKET),
    d_tcpOutbuf (new char [vrpn_CONNECTION_TCP_BUFLEN]),
    d_udpOutbuf (new char [vrpn_CONNECTION_UDP_BUFLEN]),
    d_tcpBuflen (d_tcpOutbuf ? vrpn_CONNECTION_TCP_BUFLEN : 0),
    d_udpBuflen (d_udpOutbuf ? vrpn_CONNECTION_UDP_BUFLEN : 0),
    d_tcpNumOut (0),
    d_udpNumOut (0),
    d_tcpSequenceNumber (0),
    d_udpSequenceNumber (0),
    d_tcpInbuf ((char *) d_tcpAlignedInbuf),
    d_udpInbuf ((char *) d_udpAlignedInbuf),
    d_NICaddress (NULL),
    d_tcp_only(vrpn_FALSE)
{
  vrpn_Endpoint_IP::init();
}


vrpn_Endpoint::~vrpn_Endpoint (void) {

  // Delete type and sender arrays
  if (d_senders) {
    delete d_senders;
  }
  if (d_types) {
    delete d_types;
  }

  // Delete the log, if any
  if (d_inLog) {
    // close() is called by destructor IFF necessary
    delete d_inLog;
  }
  if (d_outLog) {
    // close() is called by destructor IFF necessary
    delete d_outLog;
  }

  // Delete any file names created during the running
  if (d_remoteInLogName) { delete [] d_remoteInLogName; }
  if (d_remoteOutLogName) { delete [] d_remoteOutLogName; }
}

vrpn_Endpoint_IP::~vrpn_Endpoint_IP (void) {

  // Close all of the sockets that are left open
  if (d_tcpSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_tcpSocket);
        d_tcpSocket = INVALID_SOCKET;
        d_tcpNumOut = 0;      // Ignore characters waiting to go
  }
  if (d_udpOutboundSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_udpOutboundSocket);
        d_udpOutboundSocket = INVALID_SOCKET;
        d_udpNumOut = 0;      // Ignore characters waiting to go
  }
  if (d_udpInboundSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_udpInboundSocket);
        d_udpInboundSocket = INVALID_SOCKET;
  }
  if (d_tcpListenSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_tcpListenSocket);
        d_tcpListenSocket = INVALID_SOCKET;
  }

  // Delete the buffers created in the constructor
  if (d_tcpOutbuf) { delete [] d_tcpOutbuf; d_tcpOutbuf = NULL; }
  if (d_udpOutbuf) { delete [] d_udpOutbuf; d_udpOutbuf = NULL; }
}


vrpn_bool vrpn_Endpoint_IP::outbound_udp_open (void) const {
  return (d_udpOutboundSocket != -1);
}

int vrpn_Endpoint::local_type_id (vrpn_int32 remote_type) const {
  return d_types->mapToLocalID(remote_type);
}

int vrpn_Endpoint::local_sender_id (vrpn_int32 remote_sender) const {
  return d_senders->mapToLocalID(remote_sender);
}

vrpn_int32 vrpn_Endpoint_IP::tcp_outbuf_size (void) const {
  return d_tcpBuflen;
}

vrpn_int32 vrpn_Endpoint_IP::udp_outbuf_size (void) const {
  return d_udpBuflen;
}

vrpn_bool vrpn_Endpoint_IP::doing_okay (void) const {
  return ((status >= TRYING_TO_CONNECT) || (status == LOGGING));
}


void vrpn_Endpoint::init (void) {
  // Set all of the local IDs to -1, in case the other side
  // sends a message of a type that it has not yet defined.
  // (for example, arriving on the UDP line ahead of its TCP
  // definition).
  d_senders = new vrpn_TranslationTable ();
  d_types = new vrpn_TranslationTable ();

  if (!d_senders || !d_types) {
    fprintf(stderr, "vrpn_Endpoint::init:  Out of memory!\n");
    return;
  }

  d_inLog = new vrpn_Log (d_senders, d_types);

  if (!d_inLog) {
    fprintf(stderr, "vrpn_Endpoint::init:  Out of memory!\n");
    return;
  }

  d_outLog = new vrpn_Log (d_senders, d_types);

  if (!d_outLog) {
    fprintf(stderr, "vrpn_Endpoint::init:  Out of memory!\n");
    return;
  }
}

void vrpn_Endpoint_IP::init (void) {
  d_tcpSocket = INVALID_SOCKET;
  d_tcpListenSocket = INVALID_SOCKET;
  d_tcpListenPort = 0;
  d_udpOutboundSocket = INVALID_SOCKET;
  d_udpInboundSocket = INVALID_SOCKET;

  // Never tried a reconnect yet
  d_last_connect_attempt.tv_sec = 0;
  d_last_connect_attempt.tv_usec = 0;
}

int vrpn_Endpoint_IP::mainloop (timeval * timeout) {
  fd_set readfds, exceptfds;
  int tcp_messages_read;
  int udp_messages_read;
  int fd_max = d_tcpSocket;
  bool time_to_try_again = false;

  switch (status) {

    case CONNECTED:
    
      // Send all pending reports on the way out
      send_pending_reports();
  
      // check for pending incoming tcp or udp reports
      // we do this so that we can trigger out of the timeout
      // on either type of message without waiting on the other
    
      FD_ZERO(&readfds);              /* Clear the descriptor sets */
      FD_ZERO(&exceptfds);

      // Read incoming messages from both the UDP and TCP channels

      FD_SET(d_tcpSocket, &readfds);
      FD_SET(d_tcpSocket, &exceptfds);

      if (d_udpInboundSocket != -1) {
        FD_SET(d_udpInboundSocket, &readfds);
        FD_SET(d_udpInboundSocket, &exceptfds);
        if( d_udpInboundSocket > d_tcpSocket ) fd_max = d_udpInboundSocket;
      }

      // Select to see if ready to hear from other side, or exception
    
      if (vrpn_noint_select(fd_max+1, &readfds, NULL, &exceptfds, timeout) == -1) {
          fprintf(stderr, "vrpn_Endpoint::mainloop: select failed.\n");
#ifndef _WIN32_WCE
          fprintf(stderr, "  Errno (%d):  %s.\n", errno, strerror(errno));
#endif
          status = BROKEN;
	  return -1;
      }

      // See if exceptional condition on either socket
      if (FD_ISSET(d_tcpSocket, &exceptfds) ||
            ((d_udpInboundSocket != -1) && 
             FD_ISSET(d_udpInboundSocket, &exceptfds))) {
        fprintf(stderr, "vrpn_Endpoint::mainloop: Exception on socket\n");
        status = BROKEN;
        return -1;
      }

    // Read incoming messages from the UDP channel
    if ((d_udpInboundSocket != -1) && 
        FD_ISSET(d_udpInboundSocket,&readfds)) {
      udp_messages_read = handle_udp_messages(NULL);
      if (udp_messages_read == -1) {
        fprintf(stderr, "vrpn_Endpoint::mainloop:  "
                        "UDP handling failed, dropping connection\n");
        status = BROKEN;
        break;
      }
#ifdef VERBOSE3
      if(udp_messages_read != 0 )
        printf("udp message read = %d\n",udp_messages_read);
#else
	udp_messages_read = udp_messages_read;	// Avoid compiler warning
#endif
    }

    // Read incoming messages from the TCP channel
    if (FD_ISSET(d_tcpSocket,&readfds)) {
      tcp_messages_read = handle_tcp_messages(NULL);
      if (tcp_messages_read == -1) {
        fprintf(stderr, "vrpn: TCP handling failed, dropping connection (this is normal when a connection is dropped)\n");
        status = BROKEN;
        break;
      }
#ifdef VERBOSE3
      else {
        if (tcp_messages_read) {
          printf("tcp_message_read %d bytes\n",tcp_messages_read);
        }
      }
#else
	tcp_messages_read = tcp_messages_read; // Avoid compiler warning
#endif
    }
#ifdef	PRINT_READ_HISTOGRAM
#define      HISTSIZE 25
   {
        static vrpn_uint32 count = 0;
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

      case COOKIE_PENDING:

        poll_for_cookie(timeout);

        break;

    case TRYING_TO_CONNECT:
        struct timeval	now;
      	int ret;

#ifdef	VERBOSE
      	printf("TRYING_TO_CONNECT\n");
#endif
        // See if it has been long enough since our last attempt
        // to try again.
      	vrpn_gettimeofday(&now, NULL);
      	if (now.tv_sec - d_last_connect_attempt.tv_sec >= 2) {
          d_last_connect_attempt.tv_sec = now.tv_sec;
          time_to_try_again = true;
        }

        // If we are a TCP-only connection, then we retry to establish the connection
        // whenever it is time to try again.  Otherwise, we're done.
        if (d_tcp_only) {
          if (time_to_try_again) {
            status = TRYING_TO_CONNECT;
            if (connect_tcp_to(remote_machine_name, d_remote_port_number) == 0) {
      	      status = COOKIE_PENDING;
              if (setup_new_connection()) {
                fprintf(stderr, "vrpn_Endpoint::mainloop: "
		                "Can't set up new connection!\n");
                break;
              }
            }
          }
          break;
        }

        // We are not a TCP-only connect.
      	// See if we have a connection yet (nonblocking select).
      	ret = vrpn_poll_for_accept(d_tcpListenSocket, &d_tcpSocket);
      	if (ret  == -1) {
      		fprintf(stderr,
      		  "vrpn_Endpoint: mainloop: Can't poll for accept\n");
      		status = BROKEN;
      		break;
      	}
      	if (ret == 1) {	// Got one!
      	  status = COOKIE_PENDING;
#ifdef	VERBOSE
      	  printf("vrpn: Connection established\n");
#endif
      	  // Set up the things that need to happen when a new connection
      	  // is established.
      	  if (setup_new_connection()) {
      	      fprintf(stderr, "vrpn_Endpoint: mainloop: "
      			      "Can't set up new connection!\n");
      	      status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Endpoint::mainloop.\n");
      	      break;
      	  }
      	  break;
      	}

      	// Lob a request-to-connect packet every couple of seconds
        // If we don't wait a while between these we flood buffers and
        // do BAD THINGS (TM).

      	if (time_to_try_again) {
          if (vrpn_udp_request_lob_packet(remote_machine_name,
					  d_remote_port_number,
					  d_tcpListenPort,
                                          d_NICaddress) == -1) {
            fprintf(stderr,
            "vrpn_Endpoint: mainloop: Can't lob UDP request\n");
            status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Endpoint::mainloop.\n");
            break;
          }
        }
      break;

    case BROKEN:
      //// XXXX
      return -1;

    case LOGGING:   //  Just logging, so go about your business.
  	break;

    default:
      fprintf(stderr, "vrpn_Endpoint::mainloop():  "
                      "Unknown status (%d)\n", status);
      status = BROKEN;
      return -1;
  }

  return 0;
} // MAINLOOP


// Clear out the remote mapping list. This is done when a
// connection is dropped and we want to try and re-establish
// it.
void vrpn_Endpoint::clear_other_senders_and_types (void) {
  d_senders->clear();
  d_types->clear();
}


// Make the local mapping for the otherside sender with the same
// name, if there is one.  Return 1 if there was a mapping; this
// lets the higher-ups know that there is someone that cares
// on the other side.
int vrpn_Endpoint::newLocalSender (const char * name, vrpn_int32 which) {
  return d_senders->addLocalID(name, which);
}


// If the other side has declared this type, establish the
// mapping for it.  Return 1 if there was a mapping; this
// lets the higher-ups know that there is someone that cares
// on the other side.
int vrpn_Endpoint::newLocalType (const char * name, vrpn_int32 which) {
  return d_types->addLocalID(name, which);
}

// Adds a new remote type and returns its index.  Returns -1 on error.
int vrpn_Endpoint::newRemoteType (cName type_name, vrpn_int32 remote_id,
                                  vrpn_int32 local_id) {
  return d_types->addRemoteEntry(type_name, remote_id, local_id);
}

// Adds a new remote sender and returns its index.  Returns -1 on error.
int vrpn_Endpoint::newRemoteSender (cName sender_name, vrpn_int32 remote_id,
                                    vrpn_int32 local_id) {
  return d_senders->addRemoteEntry(sender_name, remote_id, local_id);
}

/** Pack a message into the appropriate output buffer (TCP or UDP)
    depending on the class of service for the message, and handle
    logging for the message (but not filtering).  This function
    does not handle semantic checking, local callbacks and filtering.
    (these must be done in the vrpn_Connection class routine that
    calls this one).

    Parameters: The length of the message, the local-clock time value
    for the message, the type and sender IDs for the message, the buffer
    that holds the message contents, and the class of service (currently,
    only reliable/unreliable is used).

    Returns 0 on success and -1 on failure.
*/

int vrpn_Endpoint_IP::pack_message
        (vrpn_uint32 len, timeval time,
         vrpn_int32 type, vrpn_int32 sender, const char * buffer,
         vrpn_uint32 class_of_service) {
  int ret;

  // Any semantic checking needs to have been done by the Connection
  // class: the Endpoint doesn't know enough to do it. Similarly
  // for any local callbacks that need to be done. Similarly, filtering.

  // Logging must come before filtering and should probably come before
  // any other failure-prone action (such as do_callbacks_for()).  Only
  // semantic checking should precede it.

  if (d_outLog->logOutgoingMessage (len, time, type, sender, buffer)) {
    fprintf(stderr, "vrpn_Endpoint::pack_message:  "
                    "Couldn't log outgoing message.!\n");
    return -1;
  }

  if (status == LOGGING) {
    // No error message;  this endpoint is ONLY logging.
    return 0;
  }

  // TCH 26 April 2000
  if (status != CONNECTED) {
#ifdef VERBOSE2
    fprintf(stderr, "vrpn_Endpoint::pack_message:  "
                    "Not connected, so throwing out message.\n");
#endif
    return 0;
  }

  // Determine the class of service and pass it off to the
  // appropriate service (TCP for reliable, UDP for everything else).
  // If we don't have a UDP outbound channel, send everything TCP
  if ((d_udpOutboundSocket == -1) ||
      (class_of_service & vrpn_CONNECTION_RELIABLE)) {

    // Ensure that we have an outgoing TCP buffer.  If not, then
    // we don't have anywhere to send it.
    if (d_tcpSocket == -1) {
	ret = 0;
    } else {
        ret = tryToMarshall(d_tcpOutbuf, d_tcpBuflen, d_tcpNumOut,
  			len, time, type, sender, buffer,
                        d_tcpSequenceNumber);
        d_tcpNumOut += ret;
        if (ret > 0) {
          d_tcpSequenceNumber++;
        }
    }
  } else {

    ret = tryToMarshall(d_udpOutbuf, d_udpBuflen, d_udpNumOut,
  			len, time, type, sender, buffer,
                        d_udpSequenceNumber);
    d_udpNumOut += ret;
    if (ret > 0) {
      d_udpSequenceNumber++;
    }
  }
  return (!ret) ? -1 : 0;
}

int vrpn_Endpoint_IP::send_pending_reports (void) {
  vrpn_int32 ret, sent = 0;
  int connection;
  timeval timeout;

  // Make sure we've got a valid TCP connection; else we can't send them.
  if (d_tcpSocket == -1) {
    fprintf(stderr,"vrpn_Endpoint::send_pending_reports(): No TCP connection\n");
    status = BROKEN;
    clearBuffers();
    return -1;
  }

  // Check for an exception on the socket.  If there is one, shut it
  // down and go back to listening.
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  fd_set f;
  FD_ZERO(&f);
  FD_SET(d_tcpSocket, &f);

  connection = vrpn_noint_select(d_tcpSocket+1, NULL, NULL, &f, &timeout);
  if (connection) {
    fprintf(stderr, "vrpn_Endpoint::send_pending_reports():  "
                    "select() failed.\n");
#ifndef _WIN32_WCE
    fprintf(stderr, "Errno (%d):  %s.\n", errno, strerror(errno));
#endif
    status = BROKEN;
    return -1;
  }

  // Send all of the messages that have built
  // up in the TCP buffer.  If there is an error during the send, or
  // an exceptional condition, close the accept socket and go back
  // to listening for new connections.
#ifdef  VERBOSE
  if (d_tcpNumOut) printf("TCP Need to send %d bytes\n", d_tcpNumOut);
#endif
  while (sent < d_tcpNumOut) {
    ret = send(d_tcpSocket, &d_tcpOutbuf[sent], d_tcpNumOut - sent, 0);
#ifdef  VERBOSE
    printf("TCP Sent %d bytes\n",ret);
#endif
    if (ret == -1) {
      fprintf(stderr, "vrpn_Endpoint::send_pending_reports:  "
                      "TCP send failed.\n");
      status = BROKEN;
      return -1;
    }
    sent += ret;
  }

   // Send all of the messages that have built
   // up in the UDP buffer.  If there is an error during the send, or
   // an exceptional condition, close the accept socket and go back
   // to listening for new connections.

   if ( (d_udpOutboundSocket != -1) && (d_udpNumOut > 0) ) {

        ret = send(d_udpOutboundSocket, d_udpOutbuf, d_udpNumOut, 0);
#ifdef  VERBOSE
   printf("UDP Sent %d bytes\n",ret);
#endif
      if (ret == -1) {
        fprintf(stderr, "vrpn_Endpoint::send_pending_reports:  "
                        " UDP send failed.");
        status = BROKEN;
        return -1;
      }
   }

  clearBuffers();
  return 0;
}

// Pack a message telling to call back this host on the specified
// port number.  It is important that the IP address of the host
// refers to the one that was used by the original TCP connection
// rather than (for example) the default IP address for the host.
// This is because the remote machine may not have a route to
// that NIC, but only the one used to establish the TCP connection.

int vrpn_Endpoint_IP::pack_udp_description (int portno)
{
  struct timeval now;
  vrpn_uint32 portparam = portno;
  char myIPchar [1000];
  int retval;

#ifdef VERBOSE2
  fprintf(stderr, "Getting IP address of NIC %s.\n", d_NICaddress);
#endif

  // Find the local host name that we should be using to connect.
  // If d_NICaddress is set, use it; otherwise, use the d_tcpSocket
  // if it is valid.
  retval = vrpn_getmyIP(myIPchar, sizeof(myIPchar), d_NICaddress, d_tcpSocket);
  if (retval) {
    perror("vrpn_Endpoint::pack_udp_description: can't get host name");
    return -1;
  }

  // Pack a message with type vrpn_CONNECTION_UDP_DESCRIPTION
  // whose sender ID is the ID of the port that is to be
  // used and whose body holds the zero-terminated string
  // name of the host to contact.

#ifdef  VERBOSE
  fprintf(stderr, "vrpn_Endpoint::pack_udp_description:  "
                  "Packing UDP %s:%d\n", myIPchar, portno);
#endif
  vrpn_gettimeofday(&now, NULL);

  return pack_message(strlen(myIPchar) + 1, now,
                      vrpn_CONNECTION_UDP_DESCRIPTION,
                      portparam, myIPchar, vrpn_CONNECTION_RELIABLE);
}

int vrpn_Endpoint::pack_log_description (void) {
  struct timeval now;

  // Handle the case of NULL pointers in the log file names
  // by pointing local copies at the empty string if they occur.
  const char *inName = "";
  const char *outName = "";
  if (d_remoteInLogName) { inName = d_remoteInLogName; }
  if (d_remoteOutLogName) { outName = d_remoteOutLogName; }
  
  // Include the NULL termination for the strings in the length of the buffer.
  vrpn_int32 bufsize = 2*sizeof(vrpn_int32) + strlen(inName) + 1 + strlen(outName) + 1;
  char *buf = new char[bufsize];
  if (buf == NULL) {
    fprintf(stderr,"vrpn_Endpoint::pack_log_description(): Out of memory\n");
    return -1;
  }

  // If we're not requesting remote logging, don't send any message.

  if (!d_remoteLogMode) {
    delete [] buf;
    return 0;
  }

  // Pack a message with type vrpn_CONNECTION_LOG_DESCRIPTION whose
  // sender ID is the logging mode to be used by the remote connection
  // and whose body holds the zero-terminated string name of the file
  // to write to.

  vrpn_gettimeofday(&now, NULL);
  char *bpp = buf;
  char **bp = &bpp;
  vrpn_int32 bufleft = bufsize;
  vrpn_buffer(bp, &bufleft, (vrpn_int32) strlen(inName));
  vrpn_buffer(bp, &bufleft, (vrpn_int32) strlen(outName));
  vrpn_buffer(bp, &bufleft, inName, strlen(inName));
  vrpn_buffer(bp, &bufleft, (char) 0);
  vrpn_buffer(bp, &bufleft, outName, strlen(outName));
  vrpn_buffer(bp, &bufleft, (char) 0);
  int ret = pack_message(bufsize - bufleft, now, vrpn_CONNECTION_LOG_DESCRIPTION,
                      d_remoteLogMode, buf, vrpn_CONNECTION_RELIABLE);
  delete [] buf;
  return ret;
}


// Read all messages available on the given file descriptor (a TCP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.

int vrpn_Endpoint_IP::handle_tcp_messages
          (const struct timeval * timeout) {
  timeval localTimeout;
  fd_set readfds, exceptfds;
  unsigned num_messages_read = 0;
  int retval;
  int sel_ret;

#ifdef  VERBOSE2
  printf("vrpn_Endpoint::handle_tcp_messages() called\n");
#endif

  if (timeout) {
    localTimeout.tv_sec = timeout->tv_sec;
    localTimeout.tv_usec = timeout->tv_usec;
  } else {
    localTimeout.tv_sec = 0;
    localTimeout.tv_usec = 0;
  }

  // Read incoming messages until there are no more characters to
  // read from the other side.  For each message, determine what
  // type it is and then pass it off to the appropriate handler
  // routine.  If d_stop_processing_messages_after has been set
  // to a nonzero value, then stop processing if we have received
  // at least that many messages.

  do {
    // Select to see if ready to hear from other side, or exception
    FD_ZERO(&readfds);              /* Clear the descriptor sets */
    FD_ZERO(&exceptfds);
    FD_SET(d_tcpSocket, &readfds);     /* Check for read */
    FD_SET(d_tcpSocket, &exceptfds);   /* Check for exceptions */
    sel_ret = vrpn_noint_select(d_tcpSocket+1, &readfds, NULL, &exceptfds, &localTimeout);
    if (sel_ret == -1) {
        fprintf(stderr, "vrpn_Endpoint::handle_tcp_messages:  "
                        "select failed");
        return(-1);
    }


    // See if exceptional condition on socket
    if (FD_ISSET(d_tcpSocket, &exceptfds)) {
      fprintf(stderr, "vrpn_Endpoint::handle_tcp_messages:  "
                      "Exception on socket\n");
      return(-1);
    }

    // If there is anything to read, get the next message
    if (FD_ISSET(d_tcpSocket, &readfds)) {
      retval = getOneTCPMessage(d_tcpSocket, d_tcpInbuf,
                                sizeof(d_tcpAlignedInbuf));
      if (retval) {
        return -1;
      }

      // Got one more message
      num_messages_read++;
    }

    // If we've been asked to process only a certain number of
    // messages, then stop if we've gotten at least that many.
    if (d_parent->get_Jane_value() != 0) {
      if (num_messages_read >= d_parent->get_Jane_value()) {
        break;
      }
    }
  } while (sel_ret);

  return num_messages_read;
}

// Read all messages available on the given file descriptor (a UDP link).
// Handle each message that is received.
// Return the number of messages read, or -1 on failure.
// This routine and the TCP read routine are annoyingly similar, so it
// seems like they should be merged.  They can't though:  using read() on
// the UDP socket fails, so we need the UDP version.  If we use the UDP
// version for the TCP code, it hangs when we the client drops its
// connection, so we need the TCP code as well.
// If d_stop_processing_messages_after has been set
// to a nonzero value, then stop processing if we have received
// at least that many messages.

int vrpn_Endpoint_IP::handle_udp_messages
               (const struct timeval * timeout) {
  timeval localTimeout;
  fd_set readfds, exceptfds;
  unsigned num_messages_read = 0;
  int sel_ret;
  int retval;

#ifdef  VERBOSE2
  printf("vrpn_Endpoint::handle_udp_messages() called\n");
#endif

  if (timeout) {
    localTimeout.tv_sec = timeout->tv_sec;
    localTimeout.tv_usec = timeout->tv_usec;
  } else {
    localTimeout.tv_sec = 0;
    localTimeout.tv_usec = 0;
  }

  // Read incoming messages until there are no more packets to
  // read from the other side.  Each packet may have more than one
  // message in it.  For each message, determine what
  // type it is and then pass it off to the appropriate handler
  // routine.

  do {
    // Select to see if ready to hear from server, or exception
    FD_ZERO(&readfds);              /* Clear the descriptor sets */
    FD_ZERO(&exceptfds);
    FD_SET(d_udpInboundSocket, &readfds);     /* Check for read */
    FD_SET(d_udpInboundSocket, &exceptfds);   /* Check for exceptions */
    sel_ret = vrpn_noint_select(d_udpInboundSocket+1, &readfds, NULL, &exceptfds, &localTimeout);
    if (sel_ret == -1) {
          perror("vrpn_Endpoint::handle_udp_messages: select failed()");
          return(-1);
    }

    // See if exceptional condition on socket
    if (FD_ISSET(d_udpInboundSocket, &exceptfds)) {
          fprintf(stderr, "vrpn: vrpn_Endpoint::handle_udp_messages: Exception on socket\n");
          return(-1);
    }

    // If there is anything to read, get the next message
    if (FD_ISSET(d_udpInboundSocket, &readfds)) {
      char * inbuf_ptr;
      int inbuf_len;

      inbuf_ptr = d_udpInbuf;
      inbuf_len = recv(d_udpInboundSocket, d_udpInbuf,
                       sizeof(d_udpAlignedInbuf), 0);
      if (inbuf_len == -1) {
        fprintf(stderr, "vrpn_Endpoint::handle_udp_message:  "
                        "recv() failed.\n");
        return -1;
      }

      while (inbuf_len) {
        retval = getOneUDPMessage(inbuf_ptr, inbuf_len);
        if (retval == -1) {
          return -1;
        }
        inbuf_len -= retval;
        inbuf_ptr += retval;
//fprintf(stderr, "  Advancing inbuf pointer %d bytes.\n", retval);
        // Got one more message
        num_messages_read++;
      }
    }

    // If we've been asked to process only a certain number of
    // messages, then stop if we've gotten at least that many.
    if (d_parent->get_Jane_value() != 0) {
      if (num_messages_read >= d_parent->get_Jane_value()) {
        break;
      }
    }

  } while (sel_ret);

  return num_messages_read;
}


//---------------------------------------------------------------------------
//  This routine opens a TCP socket and connects it to the machine and port
// that are passed in the msg parameter.  This is a string that contains
// the machine name, a space, then the port number.
//  The routine returns -1 on failure and the file descriptor on success.

int vrpn_Endpoint_IP::connect_tcp_to (const char * msg) {
  char	machine [1000];
  int	port;

  // Find the machine name and port number
  if (sscanf(msg, "%s %d", machine, &port) != 2) {
  	return -1;
  }

  return connect_tcp_to(machine, port);
}

int vrpn_Endpoint_IP::connect_tcp_to (const char * addr, int port) {
  struct	sockaddr_in client;     /* The name of the client */
  struct	hostent *host;          /* The host to connect to */

  /* set up the socket */
  d_tcpSocket = open_tcp_socket(NULL, d_NICaddress);
  if (d_tcpSocket < 0) {
  	fprintf(stderr, "vrpn_Endpoint::connect_tcp_to:  "
                        "can't open socket\n");
  	return -1;
  }
  client.sin_family = AF_INET;

// gethostbyname() fails on SOME Windows NT boxes, but not all,
// if given an IP octet string rather than a true name. 
// MS Documentation says it will always fail and inet_addr should 
// be called first. Avoids a 30+ second wait for
// gethostbyname() to fail.

  if ((client.sin_addr.s_addr = inet_addr(addr))
              == INADDR_NONE) {
      host = gethostbyname(addr);
      if (host) {

#ifdef CRAY
          { 
  		int i;
  		u_long foo_mark = 0;
  		for  (i = 0; i < 4; i++) {
  			u_long one_char = host->h_addr_list[0][i];
  			foo_mark = (foo_mark << 8) | one_char;
  		}
  		client.sin_addr.s_addr = foo_mark;
          }
#else
          memcpy(&(client.sin_addr.s_addr), host->h_addr,  host->h_length);
#endif

      } else {

#if !defined(hpux) && !defined(__hpux) && !defined(_WIN32)  && !defined(sparc)
          herror("gethostbyname error:");
#else
          perror("gethostbyname error:");
#endif
          fprintf(stderr, "vrpn_Endpoint::connect_tcp_to:  "
                  "error finding host by name (%s)\n",addr);
          return -1;
      }
  }

#ifndef VRPN_USE_WINSOCK_SOCKETS
  client.sin_port = htons(port);
#else
  client.sin_port = htons((u_short)port);
#endif

  if (connect(d_tcpSocket,(struct sockaddr*)&client,sizeof(client)) < 0 ){
#ifdef VRPN_USE_WINSOCK_SOCKETS
    if (!d_tcp_only) {
      fprintf(stderr,
  	     "vrpn_Endpoint::connect_tcp_to: Could not connect to machine %d.%d.%d.%d port %d\n",
             (int)(client.sin_addr.S_un.S_un_b.s_b1), (int)(client.sin_addr.S_un.S_un_b.s_b2),
             (int)(client.sin_addr.S_un.S_un_b.s_b3), (int)(client.sin_addr.S_un.S_un_b.s_b4),
             (int)(ntohs(client.sin_port)));
      int error = WSAGetLastError();
      fprintf(stderr, "Winsock error: %d\n", error);
    }
#else
    fprintf(stderr,
  	     "vrpn_Endpoint::connect_tcp_to: Could not connect to machine %d.%d.%d.%d port %d\n",
             (int)( (client.sin_addr.s_addr >> 24) & 0xff), (int)( (client.sin_addr.s_addr >> 16) & 0xff),
             (int)( (client.sin_addr.s_addr >> 8) & 0xff), (int)( (client.sin_addr.s_addr >> 0) & 0xff),
             (int)(ntohs(client.sin_port)));
#endif
    vrpn_closeSocket(d_tcpSocket);
    status = BROKEN;
    return(-1);
  }

	/* Set the socket for TCP_NODELAY */
#ifndef _WIN32_WCE
	{	struct	protoent	*p_entry;
		int	nonzero = 1;

		if ( (p_entry = getprotobyname("TCP")) == NULL ) {
			fprintf(stderr,
			  "vrpn_Endpoint::connect_tcp_to: getprotobyname() failed.\n");
			vrpn_closeSocket(d_tcpSocket);
                        status = BROKEN;
			return -1;
		}

		if (setsockopt(d_tcpSocket, p_entry->p_proto,
			TCP_NODELAY, SOCK_CAST &nonzero, sizeof(nonzero))==-1) {
			perror("vrpn_Endpoint::connect_tcp_to: setsockopt() failed");
			vrpn_closeSocket(d_tcpSocket);
                        status = BROKEN;
			return -1;
		}
	}
#endif
  status = COOKIE_PENDING;

  return 0;
}

int vrpn_Endpoint_IP::connect_udp_to (const char * addr, int port) {
    if (!d_tcp_only) {
	d_udpOutboundSocket = ::vrpn_connect_udp_port(addr, port, d_NICaddress);
	if (d_udpOutboundSocket == -1) {
		fprintf(stderr, "vrpn_Endpoint::connect_udp_to:  "
                   "Couldn't open outbound UDP link.\n");
		status = BROKEN;
		return -1;
	}
    }
    return 0;
}

vrpn_int32 vrpn_Endpoint_IP::set_tcp_outbuf_size (vrpn_int32 bytecount) {
  char * new_outbuf;

  if (bytecount < 0) {
    return d_tcpBuflen;
  }

  new_outbuf = new char [bytecount];

  if (!new_outbuf) {
    return -1;
  }

  if (d_tcpOutbuf) {
    delete [] d_tcpOutbuf;
  }

  d_tcpOutbuf = new_outbuf;
  d_tcpBuflen = bytecount;

  return d_tcpBuflen;
}

void vrpn_Endpoint_IP::drop_connection (void) {

  if (d_tcpSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_tcpSocket);
        d_tcpSocket = INVALID_SOCKET;
        d_tcpNumOut = 0;      // Ignore characters waiting to go
  }
  if (d_udpOutboundSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_udpOutboundSocket);
        d_udpOutboundSocket = INVALID_SOCKET;
        d_udpNumOut = 0;      // Ignore characters waiting to go
  }
  if (d_udpInboundSocket != INVALID_SOCKET) {
        vrpn_closeSocket(d_udpInboundSocket);
        d_udpInboundSocket = INVALID_SOCKET;
  }

  // Remove the remote mappings for senders and types. If we
  // reconnect, we will want to fill them in again. First,
  // free the space allocated for the list of names, then
  // set all of the local IDs to -1, in case the other side
  // sends a message of a type that it has not yet defined.
  // (for example, arriving on the UDP line ahead of its TCP
  // definition).

  clear_other_senders_and_types();

  // Clear out the buffers; nothing to read or send if no connection.
  clearBuffers();

  struct timeval now;
  vrpn_gettimeofday(&now, NULL);

  // If we are logging, put a message in the log telling that we
  // have had a disconnection. We don't close the logfile here unless
  // there is an error logging the message. This is because we'll want
  // to keep logging if there is a reconnection. We close the file when
  // the endpoint is destroyed.
  if (d_outLog->logMode()) {
    if (d_outLog->logMessage
               (0, now, vrpn_CONNECTION_DISCONNECT_MESSAGE, 0, NULL, 0)
            == -1) {
      fprintf(stderr,"vrpn_Endpoint::drop_connection: Can't log\n");
      d_outLog->close();       // Hope for the best...
    }
  }

  // Recall that the connection counter is a pointer to our parent
  // connection's count of active endpoints.  If it exists, we need
  // to send disconnect messages to those who care.  If this is the
  // last endpoint, then we send the last endpoint message; we
  // always send a connection dropped message.
  // Message needs to be dispatched *locally only*, so we do_callbacks_for()
  // and never pack_message()

  if (d_connectionCounter != NULL) {	// Do nothing on NULL pointer

	(*d_connectionCounter)--;	// One less connection

	d_dispatcher->doCallbacksFor
	       (d_dispatcher->registerType(vrpn_dropped_connection),
		d_dispatcher->registerSender(vrpn_CONTROL),
		now, 0, NULL);

	if (*d_connectionCounter == 0) { // None more left
	    d_dispatcher->doCallbacksFor
		 (d_dispatcher->registerType(vrpn_dropped_last_connection),
		  d_dispatcher->registerSender(vrpn_CONTROL),
		  now, 0, NULL);
	}
  }
}

void vrpn_Endpoint_IP::clearBuffers (void) {
  d_tcpNumOut = 0;
  d_udpNumOut = 0;
}

void vrpn_Endpoint_IP::setNICaddress (const char * address) {
  if (d_NICaddress) {
    delete [] d_NICaddress;
  }

#ifdef VERBOSE
  fprintf(stderr, "Setting endpoint NIC address to %s.\n", address);
#endif

  d_NICaddress = NULL;
  if (!address) {
    return;
  }
  d_NICaddress = new char [1 + strlen(address)];
  if (!d_NICaddress) {
    fprintf(stderr, "vrpn_Endpoint::setNICaddress:  Out of memory.\n");
    return;
  }
  strcpy(d_NICaddress, address);
}

int vrpn_Endpoint_IP::setup_new_connection (void) {
  char sendbuf [501];
  vrpn_int32 sendlen;
  int retval;

  retval = write_vrpn_cookie(sendbuf, vrpn_cookie_size() + 1,
                             d_remoteLogMode);
  if (retval < 0) {
          perror("vrpn_Endpoint::setup_new_connection:  "
             "Internal error - array too small.  The code's broken.");
          return -1;
  }
  sendlen = vrpn_cookie_size();

  // Write the magic cookie header to the server
  if (vrpn_noint_block_write(d_tcpSocket, sendbuf, sendlen)
      != sendlen) {
    fprintf(stderr, "vrpn_Endpoint::setup_new_connection:  "
                    "Can't write cookie.\n");
    status = BROKEN;
    return -1;
  }

  status = COOKIE_PENDING;
  poll_for_cookie();

  return 0;
}

void vrpn_Endpoint_IP::poll_for_cookie (const timeval * pTimeout) {
  timeval timeout;

  if (pTimeout) {
    timeout = *pTimeout;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

  fd_set  readfds, exceptfds;

  // most of this code copied from mainloop() case CONNECTED

  // check for pending incoming tcp or udp reports
  // we do this so that we can trigger out of the timeout
  // on either type of message without waiting on the other

  FD_ZERO(&readfds);              /* Clear the descriptor sets */
  FD_ZERO(&exceptfds);

  // Read incoming COOKIE from TCP channel

  FD_SET(d_tcpSocket, &readfds);
  FD_SET(d_tcpSocket, &exceptfds);

  // Select to see if ready to hear from other side, or exception

  if (vrpn_noint_select(d_tcpSocket+1, &readfds, NULL ,&exceptfds, &timeout) == -1) {
      fprintf(stderr, "vrpn_Endpoint::poll_for_cookie(): select failed.\n");
      status = BROKEN;
      return;
  }

  // See if exceptional condition on either socket
  if (FD_ISSET(d_tcpSocket, &exceptfds)) {
    fprintf(stderr, "vrpn_Endpoint::poll_for_cookie(): Exception on socket\n");
    return;
  }

  // Read incoming COOKIE from the TCP channel
  if (FD_ISSET(d_tcpSocket, &readfds)) {
    finish_new_connection_setup();
    if (!doing_okay()) {
      fprintf(stderr,
              "vrpn_Endpoint::poll_for_cookie: cookie handling failed\n"
              "    while connecting to \"%s\"\n",
              remote_machine_name);
      return;
    }
#ifdef VERBOSE3
    else if (status == CONNECTED) {
      printf("vrpn_Endpoint::poll_for_cookie() got cookie\n");
    }
#endif
  }
}

int vrpn_Endpoint_IP::finish_new_connection_setup (void) {
  char recvbuf [501];  // HACK
  vrpn_int32 sendlen;
  long received_logmode;
  unsigned short udp_portnum;
  int i;

  sendlen = vrpn_cookie_size();

  // Try to read the magic cookie from the server.
  int ret = vrpn_noint_block_read(d_tcpSocket, recvbuf, sendlen);
  if ( ret != sendlen) {
    perror(
      "vrpn_Endpoint::finish_new_connection_setup: Can't read cookie");
    status = BROKEN;
    return -1;
  }

  if (check_vrpn_cookie(recvbuf) < 0) {
    status = BROKEN;
    return -1;
  }

  // Store the magic cookie from the other side into a buffer so
  // that it can be put into an incoming log file.
  d_inLog->setCookie(recvbuf);

  // Find out what log mode they want us to be in BEFORE we pack
  // type, sender, and udp descriptions!  That is because we will
  // need the type and sender messages to go into the log file if
  // we're logging outgoing messages.  If it's nonzero, the
  // filename to use should come in a log_description message later.

  received_logmode = recvbuf[vrpn_MAGICLEN + 2] - '0';
  if ((received_logmode < 0) ||
      (received_logmode > (vrpn_LOG_INCOMING | vrpn_LOG_OUTGOING))) {
    fprintf(stderr, "vrpn_Endpoint::finish_new_connection_setup:  "
                    "Got invalid log mode %d\n", received_logmode);
    status = BROKEN;
    return -1;
  }
  if (received_logmode & vrpn_LOG_INCOMING) {
    d_inLog->logMode() |= vrpn_LOG_INCOMING;
  }
  if (received_logmode & vrpn_LOG_OUTGOING) {
    d_outLog->logMode() |= vrpn_LOG_OUTGOING;
  }

  // status must be sent to CONNECTED *before* any messages are
  // packed;  otherwise they're silently discarded in pack_message.
  status = CONNECTED;

  if (pack_log_description() == -1) {
    fprintf(stderr, "vrpn_Endpoint::finish_new_connection_setup:  "
                      "Can't pack remote logging instructions.\n");
    status = BROKEN;
    return -1;
  }

  // If we do not have a socket for inbound connections open, and if we
  // are allowed to do other-than-TCP sockets, then open one and tell the
  // other side that it can use it.
  if (!d_tcp_only) {

	  if (d_udpInboundSocket == INVALID_SOCKET) {
		// Open the UDP port to accept time-critical messages on.
		udp_portnum = (unsigned short)INADDR_ANY;
		d_udpInboundSocket = ::open_udp_socket(&udp_portnum, d_NICaddress);
		if (d_udpInboundSocket == -1)  {
		  fprintf(stderr, "vrpn_Endpoint::finish_new_connection_setup:  "
						  "can't open UDP socket\n");
		  status = BROKEN;
		  return -1;
		}

		// Tell the other side what port number to send its UDP messages to.
		if (pack_udp_description(udp_portnum) == -1) {
		    fprintf(stderr,
		      "vrpn_Endpoint::finish_new_connection_setup: Can't pack UDP msg\n");
		    status = BROKEN;
		    return -1;
		}
	  }
  }

#ifdef VERBOSE
  fprintf(stderr, "CONNECTED - vrpn_Endpoint::finish_new_connection_setup.\n");
#endif

  // Pack messages that describe the types of messages and sender
  // ID mappings that have been described to this connection.  These
  // messages use special IDs (negative ones).
  for (i = 0; i < d_dispatcher->numSenders(); i++) {
    pack_sender_description(i);
  }
  for (i = 0; i < d_dispatcher->numTypes(); i++) {
    pack_type_description(i);
  }

  // Send the messages
  if (send_pending_reports() == -1) {
    fprintf(stderr,
      "vrpn_Endpoint::finish_new_connection_setup: Can't send UDP msg\n");
    status = BROKEN;
    return -1;
  }

  // The connection-established messages need to be dispatched *locally only*,
  // so we do_callbacks_for and never pack_message()
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);

  // Connection counter gives us a single point to count connections that
  // actually make it to CONNECTED, not just constructed, so we send
  // got-first-/dropped-last-connection messages properly.

  if (d_connectionCounter && !*d_connectionCounter) {
    d_dispatcher->doCallbacksFor
         (d_dispatcher->registerType(vrpn_got_first_connection),
          d_dispatcher->registerSender(vrpn_CONTROL),
          now, 0, NULL);
  }

  d_dispatcher->doCallbacksFor
       (d_dispatcher->registerType(vrpn_got_connection),
        d_dispatcher->registerSender(vrpn_CONTROL),
        now, 0, NULL);

  if (d_connectionCounter) {
    (*d_connectionCounter)++;
  }

  return 0;
}



int vrpn_Endpoint_IP::getOneTCPMessage (int fd, char * buf, int buflen) {
  vrpn_int32 header [5];
  struct timeval time;
  vrpn_int32 sender, type;
  vrpn_int32 len, payload_len, ceil_len;
  int retval;

#ifdef  VERBOSE2
  fprintf(stderr, "vrpn_Endpoint::handle_tcp_messages():  something to read\n");
#endif

  // Read and parse the header
  if (vrpn_noint_block_read(fd, (char *) header, sizeof(header)) !=
          sizeof(header)) {
    fprintf(stderr,"vrpn_Endpoint::handle_tcp_messages:  "
           "Can't read header (this is normal when a connection is dropped)\n");
          return -1;
  }
  len = ntohl(header[0]);
  time.tv_sec = ntohl(header[1]);
  time.tv_usec = ntohl(header[2]);
  sender = ntohl(header[3]);
  type = ntohl(header[4]);
#ifdef  VERBOSE2
  fprintf(stderr, "  header: Len %d, Sender %d, Type %d\n",
          (int)len, (int)sender, (int)type);
#endif

  // skip up to alignment
  vrpn_int32 header_len = sizeof(header);
  if (header_len%vrpn_ALIGN) {header_len += vrpn_ALIGN - header_len%vrpn_ALIGN;}
  if (header_len > sizeof(header)) {
    // the difference can be no larger than this
    char rgch[vrpn_ALIGN];
    if (vrpn_noint_block_read(fd, (char *) rgch, header_len - sizeof(header)) !=
        (int)(header_len-sizeof(header))) {
      fprintf(stderr, "vrpn_Endpoint::handle_tcp_messages:  "
             "Can't read header + alignment\n");
      return -1;
    }
  }

  // Figure out how long the message body is, and how long it
  // is including any padding to make sure that it is a
  // multiple of four bytes long.
  payload_len = len - header_len;
  ceil_len = payload_len;
  if (ceil_len%vrpn_ALIGN) {ceil_len += vrpn_ALIGN - ceil_len%vrpn_ALIGN;}

  // Make sure the buffer is long enough to hold the whole
  // message body.
  if (buflen < ceil_len) {
       fprintf(stderr, "vrpn: vrpn_Endpoint::handle_tcp_messages: Message too long\n");
       return -1;
  }

  // Read the body of the message
  if (vrpn_noint_block_read(fd, buf, ceil_len) != ceil_len) {
   perror("vrpn: vrpn_Endpoint::handle_tcp_messages: Can't read body");
   return -1;
  }

  if (d_inLog->logIncomingMessage (payload_len, time, type, sender, buf)) {
    fprintf(stderr, "Couldn't log incoming message.!\n");
    return -1;
  }

  retval = dispatch(type, sender, time, payload_len, buf);
  if (retval) {
    return -1;
  }

  return 0;
}

int vrpn_Endpoint_IP::getOneUDPMessage (char * inbuf_ptr, int inbuf_len) {
  vrpn_int32      header[5];
  struct timeval  time;
  vrpn_int32      sender, type;
  vrpn_uint32     len, payload_len, ceil_len;
  int retval;

  // Read and parse the header
  // skip up to alignment
  vrpn_uint32 header_len = sizeof(header);
  if (header_len % vrpn_ALIGN) {
    header_len += vrpn_ALIGN - header_len % vrpn_ALIGN;
  }

  if (header_len > (vrpn_uint32) inbuf_len) {
     fprintf(stderr, "vrpn_Endpoint::getOneUDPMessage: Can't read header");
     return -1;
  }
  memcpy(header, inbuf_ptr, sizeof(header));
  inbuf_ptr += header_len;
  len = ntohl(header[0]);
  time.tv_sec = ntohl(header[1]);
  time.tv_usec = ntohl(header[2]);
  sender = ntohl(header[3]);
  type = ntohl(header[4]);


#ifdef VERBOSE
  fprintf(stderr, "Message type %ld (local type %ld), sender %ld received\n",
          type,local_type_id(type),sender);
  fprintf(stderr, "Message length is %d (buffer length %d).\n", len, inbuf_len);
#endif

  // Figure out how long the message body is, and how long it
  // is including any padding to make sure that it is a
  // multiple of vrpn_ALIGN bytes long.
  payload_len = len - header_len;
  ceil_len = payload_len;
  if (ceil_len%vrpn_ALIGN) {
    ceil_len += vrpn_ALIGN - ceil_len % vrpn_ALIGN;
  }

  // Make sure we received enough to cover the entire payload
  if (header_len + ceil_len > (vrpn_uint32) inbuf_len) {
     fprintf(stderr, "vrpn_Endpoint::getOneUDPMessage:  Can't read payload");
     return -1;
  }

  if (d_inLog->logIncomingMessage
       (payload_len, time, type, sender, inbuf_ptr)) {
    fprintf(stderr, "Couldn't log incoming message.!\n");
    return -1;
  }

  retval = dispatch(type, sender, time, payload_len, inbuf_ptr);
  if (retval) {
    return -1;
  }

  return ceil_len + header_len;
}

int vrpn_Endpoint::dispatch (vrpn_int32 type, vrpn_int32 sender,
                             timeval time, vrpn_uint32 payload_len,
                             char * bufptr) {

  // Call the handler for this message type
  // If it returns nonzero, return an error.
  if (type >= 0) {        // User handler, map to local id

    // Only process if local id has been set.

    if (local_type_id(type) >= 0) {
      if (d_dispatcher->doCallbacksFor
                           (local_type_id(type),
                            local_sender_id(sender),
                            time, payload_len, bufptr)) {
        return -1;
      }
    }

  } else {        // System handler

    if (d_dispatcher->doSystemCallbacksFor
          (type, sender, time, payload_len, bufptr, this)) {
      fprintf(stderr, "vrpn_Endpoint::dispatch:  "
                      "Nonzero system return\n");
      return -1;
    }
  }

  return 0;
}

int vrpn_Endpoint::tryToMarshall
         (char * outbuf, vrpn_int32 &buflen, vrpn_int32 &numOut,
          vrpn_uint32 len, timeval time,
          vrpn_int32 type, vrpn_int32 sender,
          const char * buffer, vrpn_uint32 sequenceNumber) {
  int retval;

  retval = marshall_message(outbuf, buflen, numOut,
                            len, time, type, sender, buffer,
                            sequenceNumber);

  // If the marshalling failed, try clearing the outgoing buffers
  // by sending the stuff in them to see if this makes enough
  // room.  If not, we'll have to give up.
  if (!retval) {
    if (send_pending_reports() != 0) {
      return 0;
    }
    retval = marshall_message(outbuf, buflen, numOut,
                              len, time, type, sender, buffer,
                              sequenceNumber);
  }

  return retval;
}


/** Marshal the message into the buffer if it will fit.  Return the number
    of characters sent (either 0 or the number requested).  This function
    should not be called directly; rather, call tryToMarshall, which will
    flush the outgoing buffer if the marshalling attempt fails.
*/

// TCH 22 Feb 99
// Marshall the sequence number, but never unmarshall it - it's currently
// only provided for the benefit of sniffers.

int vrpn_Endpoint::marshall_message
       (char * outbuf,          // Base pointer to the output buffer
        vrpn_uint32 outbuf_size,// Total size of the output buffer
        vrpn_uint32 initial_out,// How many characters are already in outbuf
        vrpn_uint32 len,        // Length of the message payload
        struct timeval time,    // Time the message was generated
        vrpn_int32 type,        // Type of the message
        vrpn_int32 sender,      // Sender of the message
        const char * buffer,    // Message payload
        vrpn_uint32 seqNo)      // Sequence number
{
  vrpn_uint32 ceil_len, header_len, total_len;
  vrpn_uint32 curr_out = initial_out; // How many out total so far

  // Compute the length of the message plus its padding to make it
  // an even multiple of vrpn_ALIGN bytes.

  // Compute the total message length and put the message
  // into the message buffer (if we have room for the whole message)
  ceil_len = len;
  if (len % vrpn_ALIGN) {
    ceil_len += vrpn_ALIGN - len % vrpn_ALIGN;
  }
  header_len = 5*sizeof(vrpn_int32);
  if (header_len%vrpn_ALIGN) {
    header_len += vrpn_ALIGN - header_len % vrpn_ALIGN;
  }
  total_len = header_len + ceil_len;
  if ((curr_out + total_len) > (vrpn_uint32) outbuf_size) {
       return 0;
  }

//fprintf(stderr, "  Marshalling message type %d, sender %d, length %d.\n",
//type, sender, len);

  // The packet header len field does not include the padding bytes,
  // these are inferred on the other side.
  // Later, to make things clearer, we should probably infer the header
  // len on the other side (in the same way the padding is done)
  // The reason we don't include the padding in the len is that we
  // would not be able to figure out the size of the padding on the
  // far side)
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(header_len + len);
  curr_out += sizeof(vrpn_uint32);

  // Pack the time (using gettimeofday() format) into the buffer
  // and do network byte ordering.
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(time.tv_sec);
  curr_out += sizeof(vrpn_uint32);
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(time.tv_usec);
  curr_out += sizeof(vrpn_uint32);

  // Pack the sender and type and do network byte-ordering
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(sender);
  curr_out += sizeof(vrpn_uint32);
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(type);
  curr_out += sizeof(vrpn_uint32);

  // Pack the sequence number.  If something's really screwy with
  // our sizes/types and there isn't room for the sequence number,
  // skipping for alignment below will overwrite it!
  *(vrpn_uint32*)(void*)(&outbuf[curr_out]) = htonl(seqNo);
  curr_out += sizeof(vrpn_uint32);

  // skip chars if needed for alignment
  curr_out = initial_out + header_len;

  // Pack the message from the buffer.  Then skip as many characters
  // as needed to make the end of the buffer fall on an even alignment
  // of vrpn_ALIGN bytes (the size of largest element sent via vrpn.
  if (buffer != NULL) {
    memcpy(&outbuf[curr_out], buffer, len);
  }
  curr_out += ceil_len;
#ifdef  VERBOSE
  printf("Marshalled: len %d, ceil_len %d: '",len,ceil_len);
  printf("'\n");
#endif
  return curr_out - initial_out;       // How many extra bytes we sent
}


// static
int vrpn_Endpoint::handle_type_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
  vrpn_Endpoint * endpoint = static_cast<vrpn_Endpoint *>(userdata);
  cName	type_name;
  vrpn_int32	i;
  vrpn_int32	local_id;

  if (p.payload_len > sizeof(cName)) {
    fprintf(stderr,"vrpn: vrpn_Endpoint::handle_type_message:  "
			"Type name too long\n");
    return -1;
  }

  // Find out the name of the type (skip the length)
  strncpy(type_name, p.buffer + sizeof(vrpn_int32),
                p.payload_len - sizeof(vrpn_int32));

  // Use the exact length packed into the start of the buffer
  // to figure out where to put the trailing '\0'
  i = ntohl(*((vrpn_int32 *) p.buffer));
  type_name[i] = '\0';

#ifdef	VERBOSE
  printf("Registering other-side type: '%s'\n", type_name);
#endif
  // If there is a corresponding local type defined, find the mapping.
  local_id = endpoint->d_dispatcher->getTypeID(type_name);
  // If not, add this type locally
  if( local_id == -1 )
  {
    if( endpoint->d_parent != NULL ) {
	  local_id = endpoint->d_parent->register_message_type( type_name );
    }
#ifdef VERBOSE
    else {
	  printf( "vrpn_Endpoint::handle_type_message:  NULL d_parent "
	  "when trying to auto-register remote message type %s.\n", type_name );
    }
#endif
  }
  if (endpoint->newRemoteType(type_name, p.sender, local_id) == -1) {
    fprintf(stderr, "vrpn: Failed to add remote type %s\n", type_name);
    return -1;
  }

  return 0;
}


void vrpn_Endpoint::setLogNames (const char * inName, const char * outName) 
{
  if( inName != NULL ) { d_inLog->setName(inName); }
  if( outName != NULL ) { d_outLog->setName(outName); }
}

int vrpn_Endpoint::openLogs (void) {

  if (d_inLog->open()) { return -1; }
  if (d_outLog->open()) { return -1; }

  return 0;
}


// static
int vrpn_Endpoint::handle_sender_message(void *userdata,
		vrpn_HANDLERPARAM p)
{
  vrpn_Endpoint * endpoint = static_cast<vrpn_Endpoint *>(userdata);
  cName	sender_name;
  vrpn_int32 i;
  vrpn_int32 local_id;

  if (p.payload_len > sizeof(cName)) {
    fprintf(stderr,"vrpn: vrpn_Endpoint::handle_sender_message():Sender name too long\n");
     return -1;
  }

  // Find out the name of the sender (skip the length)
  strncpy(sender_name, p.buffer + sizeof(vrpn_int32),
  	p.payload_len - sizeof(vrpn_int32));

  // Use the exact length packed into the start of the buffer
  // to figure out where to put the trailing '\0'
  i = ntohl(*((vrpn_int32 *) p.buffer));
  sender_name[i] = '\0';

#ifdef	VERBOSE
  printf("Registering other-side sender: '%s'\n", sender_name);
#endif
  // If there is a corresponding local sender defined, find the mapping.
  local_id = endpoint->d_dispatcher->getSenderID(sender_name);
  // If not, add this sender locally
  if( local_id == -1 )
  {
	  if( endpoint->d_parent != NULL )
	  {
		  local_id = endpoint->d_parent->register_sender( sender_name );
	  }
#ifdef VERBOSE
	  else
	  {
		  printf( "vrpn_Endpoint::handle_sender_message:  NULL d_parent "
			  "when trying to auto-register remote message sender %s\n", sender_name );
	  }
#endif
  }
  if (endpoint->newRemoteSender(sender_name, p.sender, local_id) == -1) 
  {
    fprintf(stderr, "vrpn: Failed to add remote sender %s\n", sender_name);
    return -1;
  }

  return 0;
}


int vrpn_Endpoint::pack_type_description (vrpn_int32 which) {
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(d_dispatcher->typeName(which)) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_TYPE_DESCRIPTION
   // whose sender ID is the ID of the type that is being
   // described and whose body contains the length of the name
   // and then the name of the type.

#ifdef	VERBOSE
   printf("  vrpn_Connection: Packing type '%s', %d\n",
          d_dispatcher->typeName(which), which);
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], d_dispatcher->typeName(which),
          (vrpn_int32) len);
   vrpn_gettimeofday(&now,NULL);

  return pack_message((vrpn_uint32) (len + sizeof(len)), now,
              vrpn_CONNECTION_TYPE_DESCRIPTION, which, buffer,
              vrpn_CONNECTION_RELIABLE);
}

int vrpn_Endpoint::pack_sender_description (vrpn_int32 which) {
   struct timeval now;

   // need to pack the null char as well
   vrpn_uint32	len = strlen(d_dispatcher->senderName(which)) + 1;
   vrpn_uint32 netlen;
   char buffer [sizeof(len) + sizeof(cName)];

   netlen = htonl(len);
   // Pack a message with type vrpn_CONNECTION_SENDER_DESCRIPTION
   // whose sender ID is the ID of the sender that is being
   // described and whose body contains the length of the name
   // and then the name of the sender.

#ifdef	VERBOSE
  printf("  vrpn_Connection: Packing sender '%s'\n",
         d_dispatcher->senderName(which));
#endif
   memcpy(buffer, &netlen, sizeof(netlen));
   memcpy(&buffer[sizeof(len)], d_dispatcher->senderName(which),
          (vrpn_int32) len);
   vrpn_gettimeofday(&now,NULL);

  return pack_message((vrpn_uint32)(len + sizeof(len)), now,
       vrpn_CONNECTION_SENDER_DESCRIPTION, which, buffer,
       vrpn_CONNECTION_RELIABLE);
}

int flush_udp_socket (int fd) {
  timeval localTimeout;
  fd_set readfds, exceptfds;
  char buf [10000];
  int sel_ret;

//fprintf(stderr, "flush_udp_socket().\n");

  localTimeout.tv_sec = 0;
  localTimeout.tv_usec = 0;

  // Empty out any pending UDP messages by reading the socket and
  // then throwing it away.

  do {
    // Select to see if ready to hear from server, or exception
    FD_ZERO(&readfds);              /* Clear the descriptor sets */
    FD_ZERO(&exceptfds);
    FD_SET(fd, &readfds);     /* Check for read */
    FD_SET(fd, &exceptfds);   /* Check for exceptions */
    sel_ret = vrpn_noint_select(fd+1, &readfds, NULL, &exceptfds, &localTimeout);
    if (sel_ret == -1) {
        fprintf(stderr, "flush_udp_socket:  select failed().");
        return -1;
    }

    // See if exceptional condition on socket
    if (FD_ISSET(fd, &exceptfds)) {
      fprintf(stderr, "flush_udp_socket:  Exception on socket.\n");
      return -1;
    }

    // If there is anything to read, get the next message
    if (FD_ISSET(fd, &readfds)) {
      int inbuf_len;

      inbuf_len = recv(fd, buf, 10000, 0);
      if (inbuf_len == -1) {
        fprintf(stderr, "flush_udp_socket:  recv() failed.\n");
        return -1;
      }

    }

  } while (sel_ret);

  return 0;
}

int vrpn_Connection::pack_type_description (vrpn_int32 which) {
  int retval;
  int i;

  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      retval = d_endpoints[i]->pack_type_description(which);
      if (retval) {
        return -1;
      }
    }
  }

  return 0;
}

int vrpn_Connection::pack_sender_description (vrpn_int32 which) {
  int retval;
  int i;

  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      retval = d_endpoints[i]->pack_sender_description(which);
      if (retval) {
        return -1;
      }
    }
  }

  return 0;
}

//static
int vrpn_Connection::handle_log_message (void * userdata,
                                         vrpn_HANDLERPARAM p) {
  vrpn_Endpoint * endpoint = (vrpn_Endpoint *) userdata;
  int retval = 0;
  vrpn_int32 inNameLen, outNameLen;
  const char ** bp = &p.buffer;

  // TCH 16 Feb 01
  vrpn_unbuffer(bp, &inNameLen);
  vrpn_unbuffer(bp, &outNameLen);

  // must deal properly with only opening one log file
  //  the log message contains "" (an empty string) if
  //  there is no desire to log that file.
  endpoint->setLogNames( inNameLen == 0 ? NULL : *bp, 
			 outNameLen == 0 ? NULL : *bp + inNameLen + 1);
  if( inNameLen > 0 )
	  retval = endpoint->d_inLog->open();
  if( outNameLen > 0 )
	  retval = endpoint->d_outLog->open();

  // Safety check:
  // If we can't log when the client asks us to, close the connection.
  // Something more talkative would be useful.
  // The problem with implementing this is that it's over-strict:  clients
  // that assume logging succeeded unless the connection was dropped
  // will be running on the wrong assumption if we later change this to
  // be a notification message.

  if (retval == -1) {
    // Will be dropped automatically on next pass through mainloop
    endpoint->status = BROKEN;
  } else {
    fprintf(stderr, "vrpn_Connection::handle_log_message:  "
                    "Remote connection requested logging.\n");
  }

  // OR the remotely-requested logging mode with whatever we've
  // been told to do locally
  if (p.sender & vrpn_LOG_INCOMING) {
    endpoint->d_inLog->logMode() |= vrpn_LOG_INCOMING;
  }
  if (p.sender & vrpn_LOG_OUTGOING) {
    endpoint->d_outLog->logMode() |= vrpn_LOG_OUTGOING;
  }

  return retval;
}

// Pack a message to all open endpoints. If the pack fails for any of
// the endpoints, return failure.

int vrpn_Connection::pack_message(vrpn_uint32 len, struct timeval time,
                vrpn_int32 type, vrpn_int32 sender, const char * buffer,
                vrpn_uint32 class_of_service)
{
  int i, ret;

  // Make sure I'm not broken
  if (connectionStatus == BROKEN) {
    printf("vrpn_Connection::pack_message: Can't pack because the connection is broken\n");
    return -1;
  }

  // Make sure type is either a system type (-) or a legal user type
  if (type >= d_dispatcher->numTypes()) {
    printf("vrpn_Connection::pack_message: bad type (%ld)\n", type);
    return -1;
  }

  // If this is not a system message, make sure the sender is legal.
  if (type >= 0) {
    if ((sender < 0) || (sender >= d_dispatcher->numSenders())) {
      printf("vrpn_Connection::pack_message: bad sender (%ld)\n", sender);
      return -1;
    }
  }

  // Pack the message to all open endpoints  This must be done before
  // yanking local callbacks in order to have message delivery be the
  // same on local and remote systems in the case where a local handler
  // packs one or more messages in response to this message.
  ret = 0;
  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i] &&
        (d_endpoints[i]->pack_message(len, time, type, sender, buffer,
                                   class_of_service) != 0)) {
      ret = -1;
    }
  }

  // See if there are any local handlers for this message type from
  // this sender.  If so, yank the callbacks.  This needs to be done
  // AFTER the message is packed to open endpoints so that messages
  // will be sent in the same order from local and remote senders
  // (since a local message handler may pack its own messages before
  // returning).

  if (do_callbacks_for(type, sender, time, len, buffer)) {
    return -1;
  }

  return ret;
}

// Returns the time since the connection opened.
// Some subclasses may redefine time.

// virtual
int vrpn_Connection::time_since_connection_open
                                (struct timeval * elapsed_time) {
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  *elapsed_time = vrpn_TimevalDiff(now, start_time);

  return 0;
}


// returns the current time in the connection since the epoch (UTC time).
// virtual
timeval vrpn_Connection::get_time( ) 
{
	struct timeval now;
	vrpn_gettimeofday(&now, NULL);
	return now;
}



// Returns the name of the specified sender/type, or NULL
// if the parameter is invalid.
// virtual
const char * vrpn_Connection::sender_name (vrpn_int32 sender) {
  return d_dispatcher->senderName(sender);
}

// virtual
const char * vrpn_Connection::message_type_name (vrpn_int32 type) {
  return d_dispatcher->typeName(type);
}

// virtual
int vrpn_Connection::register_log_filter (vrpn_LOGFILTER filter,
                                          void * userdata) {
  int i;
  for (i = 0; i < d_numEndpoints; i++) {
    d_endpoints[i]->d_inLog->addFilter(filter, userdata);
    d_endpoints[i]->d_outLog->addFilter(filter, userdata);
  }
  return 0;
}

// virtual
int vrpn_Connection::save_log_so_far() {
  int i;
  int final_retval = 0;
  for (i = 0; i < d_numEndpoints; i++) {
    final_retval |= d_endpoints[i]->d_inLog->saveLogSoFar();
    final_retval |= d_endpoints[i]->d_outLog->saveLogSoFar();
  }
  return final_retval;
}

// virtual
vrpn_File_Connection * vrpn_Connection::get_File_Connection (void) {
  return NULL;
}


void vrpn_Connection::init (void) {
  vrpn_int32	i;

  // Lots of constants used to be set up here.  They were moved
  // into the constructors in 02.10;  this will create a slight
  // increase in maintenance burden keeping the constructors consistient.

  for (i = 0; i < vrpn_MAX_ENDPOINTS; i++) {
    d_endpoints[i] = NULL;
  }

  vrpn_gettimeofday(&start_time, NULL);

  d_dispatcher = new vrpn_TypeDispatcher;

  // These should be among the first senders & types sent over the wire
  d_dispatcher->registerSender(vrpn_CONTROL);
  d_dispatcher->registerType(vrpn_got_first_connection);
  d_dispatcher->registerType(vrpn_got_connection);
  d_dispatcher->registerType(vrpn_dropped_connection);
  d_dispatcher->registerType(vrpn_dropped_last_connection);

  d_dispatcher->setSystemHandler
        (vrpn_CONNECTION_SENDER_DESCRIPTION,
         vrpn_Endpoint::handle_sender_message);
  d_dispatcher->setSystemHandler
        (vrpn_CONNECTION_TYPE_DESCRIPTION,
         vrpn_Endpoint::handle_type_message);
  d_dispatcher->setSystemHandler
        (vrpn_CONNECTION_DISCONNECT_MESSAGE, handle_disconnect_message);

  d_stop_processing_messages_after = 0;
}

/**
 * Deletes the endpoint and NULLs the entry in the list of open endpoints.
 */

int vrpn_Connection::delete_endpoint (int endpointIndex) {

  vrpn_Endpoint * endpoint = d_endpoints[endpointIndex];

  if (endpoint) {
    delete endpoint;
  }
  d_endpoints[endpointIndex] = NULL;

  return 0;
}

/**
 * Makes sure the endpoint array is set up cleanly for the next pass through.
 * XXX HACK - this is fragile and bound to break.  Should replace with STL
 * or some other clean linked list that guarantees traversal in spite of
 * deletions.
 */

int vrpn_Connection::compact_endpoints (void) {
  int i;

  for (i = 0; i < d_numEndpoints; i++) {
    if (!d_endpoints[i]) {
      d_endpoints[i] = d_endpoints[d_numEndpoints - 1];
      d_endpoints[d_numEndpoints - 1] = NULL;

      d_numEndpoints--;
    }
  }

  return 0;
}


// Set up to be a server connection, creating a logging connection if
// asked for.
vrpn_Connection::vrpn_Connection
      (const char * local_in_logfile_name,
       const char * local_out_logfile_name,
       vrpn_Endpoint_IP * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    d_numEndpoints (0),
    d_numConnectedEndpoints (0),
    d_dispatcher (NULL),
    d_serverLogCount (0),
    d_serverLogMode
         ((local_in_logfile_name ? vrpn_LOG_INCOMING : vrpn_LOG_NONE) |
          (local_out_logfile_name ? vrpn_LOG_OUTGOING : vrpn_LOG_NONE)),
    d_serverLogName (NULL),
    d_endpointAllocator (epa),
    d_updateEndpoint (vrpn_FALSE),
    d_references (0),
    d_autoDeleteStatus (false)
{
  int retval;
  vrpn_Endpoint * endpoint;  // shorthand for d_endpoints[0]

  // Initialize the things that must be for any constructor
  vrpn_Connection::init();

  // Server connections should handle log messages.
  d_dispatcher->setSystemHandler
        (vrpn_CONNECTION_LOG_DESCRIPTION, handle_log_message);

  if (local_out_logfile_name) {
    d_endpoints[0] = (*d_endpointAllocator)(this, NULL);
    d_endpoints[0]->setConnection( this );
    d_updateEndpoint = vrpn_TRUE;
    endpoint = d_endpoints[0];
    if (!endpoint) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
                      "Couldn't create endpoint for log file.\n");
      connectionStatus = BROKEN;
      return;
    }
    endpoint->d_outLog->setName(local_out_logfile_name);
    endpoint->d_outLog->logMode() = d_serverLogMode;
    retval = endpoint->d_outLog->open();
    if (retval == -1) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
                      "Couldn't open outgoing log file.\n");
      delete d_endpoints[0];
      d_endpoints[0] = NULL;
      connectionStatus = BROKEN;
      return;
    }
    d_numEndpoints = 1;
    endpoint->d_remoteLogMode = vrpn_LOG_NONE;
    endpoint->d_remoteInLogName = new char [10];
    strcpy(endpoint->d_remoteInLogName, "");
    endpoint->d_remoteOutLogName = new char [10];
    strcpy(endpoint->d_remoteOutLogName, "");
    // Outgoing messages are logged regardless of connection status.
    endpoint->status = LOGGING;
  }

  if (local_in_logfile_name) {
    d_serverLogName = new char [1 + strlen(local_in_logfile_name)];
    if (!d_serverLogName) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
                      "Out of memory.\n");
      connectionStatus = BROKEN;
      return;
    }
    strcpy(d_serverLogName, local_in_logfile_name);
  }

}

vrpn_Connection::vrpn_Connection
      (const char * local_in_logfile_name,
       const char * local_out_logfile_name,
       const char * remote_in_logfile_name,
       const char * remote_out_logfile_name,
       vrpn_Endpoint_IP * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    connectionStatus (BROKEN),  // default value if not otherwise set in ctr
    d_numEndpoints (0),
    d_numConnectedEndpoints (0),
    d_dispatcher (NULL),
    d_serverLogCount (0),
    d_serverLogMode (vrpn_LOG_NONE),
    d_serverLogName (NULL),
    d_endpointAllocator (epa),
    d_updateEndpoint (vrpn_FALSE),
    d_references (0),
    d_autoDeleteStatus (false)
{
  vrpn_Endpoint * endpoint;
  int retval;

  // Initialize the things that must be for any constructor
  vrpn_Connection::init();

  // We're a client;  create our single endpoint and initialize it.
  d_endpoints[0] = (*d_endpointAllocator)(this, &d_numConnectedEndpoints);
  d_endpoints[0]->setConnection( this );
  d_updateEndpoint = vrpn_TRUE;
  if (!d_endpoints[0]) {
    fprintf(stderr, "vrpn_Connection:  Out of memory.\n");
    connectionStatus = BROKEN;
    return;
  }

  d_numEndpoints = 1;
  endpoint = d_endpoints[0];  // shorthand

  // Store the remote log file name and the remote log mode
  endpoint->d_remoteLogMode = 
         (( (remote_in_logfile_name && strlen(remote_in_logfile_name) > 0) ? vrpn_LOG_INCOMING : vrpn_LOG_NONE) |
          ( (remote_out_logfile_name && strlen(remote_out_logfile_name) > 0) ? vrpn_LOG_OUTGOING : vrpn_LOG_NONE));
  if (!remote_in_logfile_name) {
    endpoint->d_remoteInLogName = new char [10];
    strcpy(endpoint->d_remoteInLogName, "");
  } else {
    endpoint->d_remoteInLogName =
    	new char [strlen(remote_in_logfile_name) + 1];
    strcpy(endpoint->d_remoteInLogName, remote_in_logfile_name);
  }
   
  if (!remote_out_logfile_name) {
    endpoint->d_remoteOutLogName = new char [10];
    strcpy(endpoint->d_remoteOutLogName, "");
  } else {
    endpoint->d_remoteOutLogName =
    	new char [strlen(remote_out_logfile_name) + 1];
    strcpy(endpoint->d_remoteOutLogName, remote_out_logfile_name);
  }
   
  // If we are doing local logging, turn it on here. If we
  // can't open the file, then the connection is broken.

  if (local_in_logfile_name && (strlen(local_in_logfile_name) != 0)) {
    endpoint->d_inLog->setName(local_in_logfile_name);
    endpoint->d_inLog->logMode() = vrpn_LOG_INCOMING;
    retval = endpoint->d_inLog->open();
    if (retval == -1) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
    		      "Couldn't open incoming log file.\n");
      connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
      return;
    }
//fprintf(stderr, "vrpn_Connection: opened logfile.\n");
  }

  if (local_out_logfile_name && (strlen(local_out_logfile_name) != 0)) {
    endpoint->d_outLog->setName(local_out_logfile_name);
    endpoint->d_outLog->logMode() = vrpn_LOG_OUTGOING;
    retval = endpoint->d_outLog->open();
    if (retval == -1) {
      fprintf(stderr, "vrpn_Connection::vrpn_Connection:  "
    		      "Couldn't open local outgoing log file.\n");
      connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::vrpn_Connection.\n");
      return;
    }
//fprintf(stderr, "vrpn_Connection: opened logfile.\n");
  }
}

vrpn_Connection::~vrpn_Connection (void) {

  // Clean up types, senders, and callbacks.
  delete d_dispatcher;

  if (d_references > 0) {
    fprintf(stderr, "Connection was deleted while %d references still remain.\n",
            d_references);
  }
}

// Some object is now using this connection.
void vrpn_Connection::addReference()
{
	d_references++;
}

// Some object has stopped using this connection.  Decrement the ref counter.
// If there aren't any references, this connection is no longer in use.
//  Once it's no longer in use destroy the connection iff d_autoDeleteStatus
//  has been set to TRUE.  If d_autoDeleteStatus is FALSE, the user must
//  destroy the connection explicitly.
void vrpn_Connection::removeReference()
{
	d_references--;
	if (d_references == 0 && d_autoDeleteStatus == true) {
		delete this;
	}
	else if (d_references < 0) {	// this shouldn't happen.
		// sanity check
		fprintf(stderr, "Negative reference count.  This shouldn't happen.");
	}
}

vrpn_int32 vrpn_Connection::register_sender (const char * name) {
   vrpn_int32 retval;
   vrpn_int32 i;

#ifdef VERBOSE
  fprintf(stderr, "vrpn_Connection::register_sender:  "
  "%d senders;  new name \"%s\"\n", d_dispatcher->numSenders(), name);
#endif

  // See if the name is already in the list.  If so, return it.
  retval = d_dispatcher->getSenderID(name);
  if (retval != -1) {
#ifdef VERBOSE
  fprintf(stderr, "Sender already defined as id %d.\n", retval);
#endif
    return retval;
  }

  retval = d_dispatcher->addSender(name);

#ifdef VERBOSE
  fprintf(stderr, "Packing sender description for %s, type %d.\n",
  name, retval);
#endif

   // Pack the sender description.
   // TCH 24 Jan 00 - Need to do this even if not connected so
   // that it goes into the logs (if we're keeping any).
   pack_sender_description(retval);

   // If the other side has declared this sender, establish the
   // mapping for it.
   for (i = 0; i < d_numEndpoints; i++) {
     d_endpoints[i]->newLocalSender(name, retval);
   }

   // One more in place -- return its index
   return retval;
}

vrpn_int32 vrpn_Connection::register_message_type (const char * name) {
  vrpn_int32 retval;
  vrpn_int32 i;

#ifdef VERBOSE
  fprintf(stderr, "vrpn_Connection::register_message_type:  "
  "%d type;  new name \"%s\"\n", d_dispatcher->numTypes(), name);
#endif

  // See if the name is already in the list.  If so, return it.
  retval = d_dispatcher->getTypeID(name);
  if (retval != -1) {
#ifdef VERBOSE
  fprintf(stderr, "Type already defined as id %d.\n", retval);
#endif
    return retval;
  }

  retval = d_dispatcher->addType(name);

  // Pack the type description.
  // TCH 24 Jan 00 - Need to do this even if not connected so
  // that it goes into the logs (if we're keeping any).

#ifdef VERBOSE
  fprintf(stderr, "Packing type description for %s, type %d.\n", name, retval);
#endif

  pack_type_description(retval);

  // If the other side has declared this type, establish the
  // mapping for it.
  for (i = 0; i < d_numEndpoints; i++) {
    d_endpoints[i]->newLocalType(name, retval);
  }

  // One more in place -- return its index
  return retval;
}


// Yank the callback chain for a message type.  Call all handlers that
// are interested in messages from this sender.  Return 0 if they all
// return 0, -1 otherwise.

int	vrpn_Connection::do_callbacks_for(vrpn_int32 type, vrpn_int32 sender,
		struct timeval time, vrpn_uint32 payload_len, const char * buf)
{
  return d_dispatcher->doCallbacksFor(type, sender, time, payload_len, buf);
}

int vrpn_Connection::doSystemCallbacksFor (vrpn_HANDLERPARAM p, void * ud) {
  return d_dispatcher->doSystemCallbacksFor(p, ud);
}


void vrpn_Connection::
get_log_names(char **local_in_logname, char **local_out_logname, char **remote_in_logname, char **remote_out_logname)
{
	if( !(d_endpoints[0]) ) return;
	vrpn_Endpoint* endpoint = d_endpoints[0];
	// XXX it is possible to have more than one endpoint, and other endpoints may have other log names

	*local_in_logname = endpoint->d_inLog->getName();
	*local_out_logname = endpoint->d_outLog->getName();

	if( endpoint->d_remoteInLogName != NULL )
	{
		*remote_in_logname = new char[ strlen( endpoint->d_remoteInLogName ) + 1 ];
		strcpy( *remote_in_logname, endpoint->d_remoteInLogName );
	}
	else
	{
		*remote_in_logname = NULL;
	}

	if( endpoint->d_remoteOutLogName != NULL )
	{
		*remote_out_logname = new char[ strlen( endpoint->d_remoteOutLogName ) + 1 ];
		strcpy( *remote_out_logname, endpoint->d_remoteOutLogName );
	}
	else
	{
		*remote_out_logname = NULL;
	}
}


// virtual
void vrpn_Connection::updateEndpoints (void) {

}

// static
vrpn_Endpoint_IP * vrpn_Connection::allocateEndpoint (vrpn_Connection * me,
                                                   vrpn_int32 * connectedEC) {
  return new vrpn_Endpoint_IP (me->d_dispatcher, connectedEC);
}



// This is called when a disconnect message is found in the logfile.
// It causes the other-side sender and type messages to be cleared,
// in anticipation of a possible new set of messages caused by a
// reconnected server.

int	vrpn_Connection::handle_disconnect_message (void * userdata,
		                                    vrpn_HANDLERPARAM) {
  vrpn_Endpoint *endpoint = (vrpn_Endpoint *) userdata;

#ifdef	VERBOSE
  printf("Just read disconnect message from logfile\n");
#endif
  endpoint->clear_other_senders_and_types();

  return 0;
}

int vrpn_Connection::register_handler(vrpn_int32 type,
			vrpn_MESSAGEHANDLER handler,
                        void * userdata, vrpn_int32 sender)
{
  return d_dispatcher->addHandler(type, handler, userdata, sender);
}

int vrpn_Connection::unregister_handler(vrpn_int32 type,
			vrpn_MESSAGEHANDLER handler,
                        void *userdata, vrpn_int32 sender)
{
  return d_dispatcher->removeHandler(type, handler, userdata, sender);
}

int vrpn_Connection::message_type_is_registered (const char * name) const
{
  return d_dispatcher->getTypeID(name);
}

// Changed 8 November 1999 by TCH
// With multiple connections allowed, TRYING_TO_CONNECT is an
// "ok" status, so we need to admit it.  (Used to check >= 0)
// XXX What if one endpoint is BROKEN? Don't we need to loop?
vrpn_bool vrpn_Connection::doing_okay (void) const {

    int endpointIndex;
    
    for (endpointIndex = 0; endpointIndex < d_numEndpoints; endpointIndex++) {
        if (d_endpoints[endpointIndex] &&
            (!d_endpoints[endpointIndex]->doing_okay())) {
          return VRPN_FALSE;
        }
    }
    return (connectionStatus > BROKEN);
}

// Loop over endpoints and return TRUE if any of them are connected.
vrpn_bool vrpn_Connection::connected (void) const
{
    int endpointIndex;
    
    for (endpointIndex = 0; endpointIndex < d_numEndpoints; endpointIndex++) {
        if (d_endpoints[endpointIndex] &&
            (d_endpoints[endpointIndex]->status == CONNECTED)) {
          return VRPN_TRUE;
        }
    }
    return VRPN_FALSE;
}


//------------------------------------------------------------------------
//	This section holds data structures and functions to open
// connections by name.
//	The intention of this section is that it can open connections for
// objects that are of different types (trackers, buttons and sound),
// even if they all refer to the same connection.

//	This routine will return a pointer to the connection whose name
// is passed in.  If the routine is called multiple times with the same
// name, it will return the same pointer, rather than trying to make
// multiple of the same connection (unless force_connection = true).
// If the connection is in a bad way, it returns NULL.
//	This routine will strip off any part of the string before and
// including the '@' character, considering this to be the local part
// of a device name, rather than part of the connection name.  This allows
// the opening of a connection to "Tracker0@ioglab" for example, which will
// open a connection to ioglab.

// This routine adds to the reference count of the connection in question.
// This happens regardless of whether the connection already exists
//  or it is to be created.
// Any user code that calls vrpn_get_connection_by_name() directly
//  should call vrpn_Connection::removeReference() when it is finished
//  with the pointer.  It's ok if you have old code that doesn't do this;
//  it just means the connection will remain open until the program quits,
//  which isn't so bad.

vrpn_Connection * vrpn_get_connection_by_name (
	const char * cname,
	const char * local_in_logfile_name,
	const char * local_out_logfile_name,
	const char * remote_in_logfile_name,
	const char * remote_out_logfile_name,
	const char * NIC_IPaddress,
        bool force_connection)
{
	if (cname == NULL) {
		fprintf(stderr,"vrpn_get_connection_by_name(): NULL name\n");
		return NULL;
	}

	// Find the relevant part of the name (skip past last '@'
	// if there is one)
	const char *where_at;	// Part of name past last '@'
	if ( (where_at = strrchr(cname, '@')) != NULL) {
		cname = where_at+1;	// Chop off the front of the name
	}

	vrpn_Connection * c = NULL;
        if (!force_connection) {
	  c = vrpn_ConnectionManager::instance().getByName(cname);
        }

	// If its not already open, open it.
	// Its constructor will add it to the list (?).
	if (!c) {

		// connections now self-register in the known list --
		// this is kind of odd, but oh well (can probably be done
		// more cleanly later).

		int is_file = !strncmp(cname, "file:", 5);

		if (is_file) {
			c = new vrpn_File_Connection (cname, 
			                              local_in_logfile_name,
			                              local_out_logfile_name);
		} else {
			int port = vrpn_get_port_number(cname);
			c = new vrpn_Connection_IP (cname, port,
				local_in_logfile_name, local_out_logfile_name,
				remote_in_logfile_name, remote_out_logfile_name,
				NIC_IPaddress);
		}

		if (c) {	// creation succeeded
			c->setAutoDeleteStatus(true);	// destroy when refcount hits zero.
		}
		else {		// creation failed
			fprintf(stderr, "Could not create new connection.");
			return NULL;
		}
	}
	// else the connection was already open.

	c->addReference();	// increment the reference count either way.

	// Return a pointer to the connection, even if it is not doing
	// okay. This will allow a connection to retry over and over
	// again before connecting to the server.
	return c;
}

// Create a server connection that will listen for client connections.
// To create a VRPN TCP/UDP server, use a name like:
//    x-vrpn:machine_name_or_ip:port
//    x-vrpn::port
//    machine_name_or_ip:port
//    machine_name_or_ip
//    :port
// To create an MPI server, use a name like:
//    mpi:MPI_COMM_WORLD
//    mpi:comm_number
//
//	This routine will strip off any part of the string before and
// including the '@' character, considering this to be the local part
// of a device name, rather than part of the connection name.  This allows
// the opening of a connection to "Tracker0@ioglab" for example, which will
// open a connection to ioglab.

vrpn_Connection * vrpn_create_server_connection (
	const char * cname,
	const char * local_in_logfile_name,
	const char * local_out_logfile_name)
{
  vrpn_Connection * c = NULL;

  // Parse the name to find out what kind of connection we are to make.
  if (cname == NULL) {
	fprintf(stderr,"vrpn_create_server_connection(): NULL name\n");
	return NULL;
  }
  char *location = vrpn_copy_service_location(cname);
  if (location == NULL) { return NULL; }
  int is_mpi = !strncmp(cname, "mpi:", 4);
  if (is_mpi) {
#ifdef  vrpn_USE_MPI
    XXX_implement_MPI_server_connection;
#else
    fprintf(stderr,"vrpn_create_server_connection(): MPI support not compiled in.  Set vrpn_USE_MPI in vrpn_Configure.h and recompile.\n");
    return NULL;
#endif
  } else {
    // Not an MPI port, so we presume that we are a standard VRPN UDP/TCP
    // port.  Open that kind, based on the machine and port name.  If we don't
    // have a machine name, then we pass NULL to the NIC address.  If we do have
    // one, we pass it to the NIC address.
    if (strlen(location) == 0) {
      c = new vrpn_Connection_IP(vrpn_DEFAULT_LISTEN_PORT_NO,
        local_in_logfile_name, local_out_logfile_name);
    } else {
      // Find machine name and port number.  Port number returns default
      // if there is not one specified.  If the machine name is zero length
      // (just got :port) then use NULL, which means the default NIC.
      char *machine = vrpn_copy_machine_name(location);
      if (strlen(machine) == 0) {
        delete [] machine;
        machine = NULL;
      }
      int port = vrpn_get_port_number(location);
      c = new vrpn_Connection_IP(port,
        local_in_logfile_name, local_out_logfile_name,
        machine);
      if (machine) { delete [] machine; }
    }
  }
  delete [] location;

  if (!c){		// creation failed
    fprintf(stderr, "vrpn_create_server_connection(): Could not create new connection.");
    return NULL;
  }


  c->setAutoDeleteStatus(true);	// destroy when refcount hits zero.
  c->addReference();	// increment the reference count for the connection.

  // Return a pointer to the connection, even if it is not doing
  // okay. This will allow a connection to retry over and over
  // again before connecting to the server.
  return c;
}


/** Create a new endpoint for this connection and connect to using a
    TCP connection directly to the specified machine and port.  This
    bypasses the UDP send, and is used as part of the vrpn "RSH" startup,
    where the server is started by the client program and calls it back
    at a specified port.

    Returns 0 on success and -1 on failure.
*/

int vrpn_Connection_IP::connect_to_client (const char *machine, int port)
{
	if (connectionStatus != LISTEN) { return -1; };

	int which_end = d_numEndpoints;

	// Make sure that we have room for a new connection
	if (which_end >= vrpn_MAX_ENDPOINTS) {
	  fprintf(stderr, "vrpn_Connection_IP::connect_to_client:"
			" Too many existing connections.\n");
	  return -1;
	}

	d_endpoints[which_end] 
		= (*d_endpointAllocator)(this, &d_numConnectedEndpoints);
	d_endpoints[which_end]->setConnection( this );
	d_updateEndpoint = vrpn_TRUE;
	vrpn_Endpoint_IP * endpoint = d_endpoints[which_end];

	if (!endpoint) {
		fprintf(stderr, "vrpn_Connection_IP::connect_to_client:"
			" Out of memory on new endpoint\n");
		return -1;
	}

	char msg [100];
	sprintf(msg, "%s %d", machine, port);
	printf("vrpn_Connection_IP::connect_to_client:  "
	   "Connection request received: %s\n", msg);
	endpoint->connect_tcp_to(msg);
	if (endpoint->status != COOKIE_PENDING) {   // Something broke
		endpoint->status = BROKEN;
		return -1;
	} else {
		d_numEndpoints++;
		handle_connection(which_end);
	}

    return 0;
}

void vrpn_Connection_IP::handle_connection (int endpointIndex) {

  vrpn_Endpoint * endpoint = d_endpoints[endpointIndex];

   // Set up the things that need to happen when a new connection is
   // started.
   if (endpoint->setup_new_connection()) {
	fprintf(stderr,"vrpn_Connection_IP::handle_connection():  "
                       "Can't set up new connection!\n");
	drop_connection(endpointIndex);
	return;
   }
}

// Get the UDP port description from the other side and connect the
// outgoing UDP socket to that port so we can send lossy but time-
// critical (tracker) messages that way.

// static
int vrpn_Connection_IP::handle_UDP_message (void * userdata,
		                         vrpn_HANDLERPARAM p) {
  vrpn_Endpoint_IP * endpoint = (vrpn_Endpoint_IP *) userdata;
  char rhostname [1000];  // name of remote host

#ifdef	VERBOSE
  printf("  Received request for UDP channel to %s\n", p.buffer);
#endif

  // Get the name of the remote host from the buffer (ensure terminated)
  strncpy(rhostname, p.buffer, sizeof(rhostname));
  rhostname[sizeof(rhostname) - 1] = '\0';

  // Open the UDP outbound port and connect it to the port on the
  // remote machine.
  // (remember that the sender field holds the UDP port number)
  endpoint->connect_udp_to(rhostname, (int)p.sender);
  if (endpoint->status == BROKEN) {
    return -1;
  }

  // Put this here because currently every connection opens a UDP
  // port and every host will get this data.  Previous implementation
  // did it in connect_tcp_to, which only gets called by servers.

  strncpy(endpoint->rhostname, rhostname, sizeof(endpoint->rhostname));

#ifdef	VERBOSE
  printf("  Opened UDP channel to %s:%d\n", rhostname, p.sender);
#endif
  return 0;
}

int vrpn_Connection_IP::send_pending_reports (void) {
  int i;

  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i] &&
        (d_endpoints[i]->send_pending_reports() != 0)) {
      fprintf(stderr, "vrpn_Connection_IP::send_pending_reports:  "
                      "Closing failed endpoint.\n");
      drop_connection(i);
    }
  }

  compact_endpoints();

  return 0;
}

void vrpn_Connection_IP::init (void) {

#ifdef VRPN_USE_WINSOCK_SOCKETS
  // Make sure sockets are set up
  // TCH 2 Nov 98 after Phil Winston

  WSADATA wsaData;
  int winStatus;

  winStatus = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (winStatus) {
    fprintf(stderr, "vrpn_Connection_IP::init():  "
                    "Failed to set up sockets.\n");
    fprintf(stderr, "WSAStartup failed with error code %d\n", winStatus);
    exit(0);
  }
#endif  // windows sockets

  // Ignore SIGPIPE, so that the program does not crash when a
  // remote client OR SERVER shuts down its TCP connection.  We'll find out
  // about it as an exception on the socket when we select() for
  // read.
  // Mips/Ultrix header file signal.h appears to be broken and
  // require the following cast
#ifndef _WIN32  // XXX what about cygwin?
 #ifdef ultrix
   signal( SIGPIPE, (void (*) (int)) SIG_IGN );
 #else
   signal( SIGPIPE, SIG_IGN );
 #endif
#endif

  // Set up to handle the UDP-request system message.
  d_dispatcher->setSystemHandler
        (vrpn_CONNECTION_UDP_DESCRIPTION, handle_UDP_message);
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

void vrpn_Connection_IP::server_check_for_incoming_connections
                         (const struct timeval * pTimeout) {
  vrpn_Endpoint_IP * endpoint;  // shorthand for d_endpoints[which_end]
  int  request;
  char msg[200];       // Message received on the request channel
  timeval timeout;
  int which_end = d_numEndpoints;
  int retval;
  int port;

  if (pTimeout) {
    timeout = *pTimeout;
  } else {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
  }

  // Do a select() with timeout (perhaps zero timeout) to see if
  // there is an incoming packet on the UDP socket.

  fd_set f;
  FD_ZERO (&f);
  FD_SET(listen_udp_sock, &f);
  request = vrpn_noint_select(listen_udp_sock+1, &f, NULL, NULL, &timeout);
  if (request == -1 ) {        // Error in the select()
    fprintf(stderr, "vrpn_Connection_IP::server_check_for_incoming_connections():  "
                    "select failed.\n");
      connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection::server_check_for_incoming_connections.\n");
      return;
  } else if (request != 0) {   // Some data to read!  Go get it.
      struct sockaddr_in from;
      int fromlen = sizeof(from);

      if (recvfrom(listen_udp_sock, msg, sizeof(msg), 0,
                   (struct sockaddr *) &from, GSN_CAST &fromlen) == -1) {
              fprintf(stderr,
                      "vrpn: Error on recvfrom: Bad connection attempt\n");
              return;
      }

      printf("vrpn: Connection request received: %s\n", msg);

      // Make sure that we have room for a new connection
      if (which_end >= vrpn_MAX_ENDPOINTS) {
          fprintf(stderr, "vrpn: Too many existing connections;  "
                          "ignoring request from %s\n", msg);
          return;
      }

      // Create a new endpoint and start trying to connect it to
      // the client.
      d_endpoints[which_end] = (*d_endpointAllocator)(this, &d_numConnectedEndpoints);
      d_endpoints[which_end]->setConnection( this );
      d_updateEndpoint = vrpn_TRUE;
      endpoint = d_endpoints[which_end];
      if (!endpoint) {
          fprintf(stderr,
                  "vrpn_Connection_IP::server_check_for_incoming_connections:\n"
                  "    Out of memory on new endpoint\n");
          return;
      }

      // Server-side logging under multiconnection - TCH July 2000
      // Check for NULL server log name, which happens when the log file
      // already exists and it can't save it.
      if ( (d_serverLogMode & vrpn_LOG_INCOMING) && (d_serverLogName != NULL) ) {
        d_serverLogCount++;
        endpoint->d_inLog->setCompoundName(d_serverLogName, d_serverLogCount);
        endpoint->d_inLog->logMode() = vrpn_LOG_INCOMING;
        retval = endpoint->d_inLog->open();
        if (retval == -1) {
          fprintf(stderr,
                  "vrpn_Connection_IP::server_check_for_incoming_connections:  "
                  "Couldn't open log file.\n");
          connectionStatus = BROKEN;
          return;
        }
      }

      endpoint->setNICaddress(d_NIC_IP);
      endpoint->status = TRYING_TO_CONNECT;

      // d_numEndpoints must be incremented before handle_connection is called
      // otherwise the functions doing_okay and connected do not check all
      // the endpoints. Because of this topo was unable to send the header
      // information and nano crashed...
      d_numEndpoints++;

      // Because we sometimes use multiple NICs, we are ignoring the IP from the
      // client, and filling in the NIC that the udp request arrived on.
      sscanf(msg, "%*s %d", &port);   // get the port
      //fill in NIC address
      unsigned long addr_num = ntohl(from.sin_addr.s_addr);
      sprintf(msg, "%u.%u.%u.%u %d", 
              (addr_num) >> 24,
              (addr_num >> 16) & 0xff,
              (addr_num >> 8) & 0xff,
              addr_num & 0xff, port); 
      endpoint->remote_machine_name = msg;
      endpoint->connect_tcp_to(msg);
      handle_connection(which_end);

      // HACK
      // We don't want to do this, but connection requests are soft state
      // that will be restored in 1 second;  meanwhile, if we accept multiple
      // connection requests from the same source we try to open multiple
      // connections to it, which invariably makes SOMEBODY crash sooner or
      // later.
      flush_udp_socket(listen_udp_sock);
  }

  // Do a zero-time select() to see if there are incoming TCP requests on
  // the listen socket.  This is used when the client needs to punch through
  // a firewall.

  SOCKET newSocket;
  retval = vrpn_poll_for_accept(listen_tcp_sock, &newSocket);

  if (retval == -1) {
    fprintf(stderr, "Error accepting on TCP socket.\n");
    return;
  } else if (retval) { // Some data to read!  Go get it.

    printf("vrpn: TCP connection request received.\n");

    if (which_end >= vrpn_MAX_ENDPOINTS) {
        fprintf(stderr, "vrpn: Too many existing connections;  "
                        "ignoring request.\n");
        return;
    }

    d_endpoints[which_end] 
		= (*d_endpointAllocator)(this, &d_numConnectedEndpoints);
	d_endpoints[which_end]->setConnection( this );
    d_updateEndpoint = vrpn_TRUE;
    endpoint = d_endpoints[which_end];
    if (!endpoint) {
      fprintf(stderr,
              "vrpn_Connection_IP::server_check_for_incoming_connections:\n"
              "    Out of memory on new endpoint\n");
      return;
    }

    // Since we're being connected to using a TCP request, tell the endpoint
    // not to try and establish any other connections (since the client is
    // presumably coming through a firewall or NAT and UDP packets won't get
    // through).
    endpoint->d_tcp_only = vrpn_TRUE;
    endpoint->d_remote_port_number = port;

    // Server-side logging under multiconnection - TCH July 2000
    if (d_serverLogMode & vrpn_LOG_INCOMING) {
      d_serverLogCount++;
      endpoint->d_inLog->setCompoundName(d_serverLogName, d_serverLogCount);
      endpoint->d_inLog->logMode() = vrpn_LOG_INCOMING;
      retval = endpoint->d_inLog->open();
      if (retval == -1) {
        fprintf(stderr,
                "vrpn_Connection_IP::server_check_for_incoming_connections:  "
                "Couldn't open incoming log file.\n");
        connectionStatus = BROKEN;
        return;
      }
    }

    endpoint->setNICaddress(d_NIC_IP);
    endpoint->d_tcpSocket = newSocket;

    d_numEndpoints++;

    handle_connection(which_end);
  }

  return;
}


void vrpn_Connection_IP::drop_connection (int whichEndpoint)
{
  vrpn_Endpoint * endpoint = d_endpoints[whichEndpoint];

  endpoint->drop_connection();

  // If we're a client, try to reconnect to the server
  // that just dropped its connection.
  // If we're a server, delete the endpoint.
  if (listen_udp_sock == INVALID_SOCKET) {
    endpoint->status = TRYING_TO_CONNECT;
  } else  {
    delete_endpoint(whichEndpoint);
  }
}

int vrpn_Connection_IP::mainloop (const struct timeval * pTimeout) {
  vrpn_Endpoint * endpoint;
  timeval timeout;
  int endpointIndex;

#ifdef	VERBOSE2
  //printf("vrpn_Connection_IP::mainloop() called (status %d)\n",connectionStatus);
#endif
  
  if (d_updateEndpoint) {
    updateEndpoints();
    d_updateEndpoint = vrpn_FALSE;
  }
  // struct timeval perSocketTimeout;
  // const int numSockets = 2;
  // divide timeout over all selects()
  //  if (timeout) {
  //    perSocketTimeout.tv_sec = timeout->tv_sec / numSockets;
  //    perSocketTimeout.tv_usec = timeout->tv_usec / numSockets
  //      + (timeout->tv_sec % numSockets) *
  //      (1000000L / numSockets);
  //  } else {
  //    perSocketTimeout.tv_sec = 0;
  //    perSocketTimeout.tv_usec = 0;
  //  }

  // BETTER IDEA: do one select rather than multiple -- otherwise you are
  // potentially holding up one or the other.  This allows clients which
  // should not do anything until they get messages to request infinite
  // waits (servers can't usually do infinite waits this because they need
  // to service other devices to generate info which they then send to
  // clients) .  weberh 3/20/99

  if (connectionStatus == LISTEN) {
    server_check_for_incoming_connections(pTimeout);
  }

  for (endpointIndex = 0; endpointIndex < d_numEndpoints; endpointIndex++) {
    endpoint = d_endpoints[endpointIndex];

    // The current array-sorting code is liable to break when unexpected
    // things happen, so we have to double-check it here.
    if (!endpoint) {
      continue;
    }

    if (pTimeout) {
      timeout = *pTimeout;
    } else {
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
    }

    endpoint->mainloop(&timeout);

    if (endpoint->status == BROKEN) {
      drop_connection(endpointIndex);
    }
  }

  // Do housekeeping on the endpoint array
  compact_endpoints();

  return 0;
}

vrpn_Connection_IP::vrpn_Connection_IP
      (unsigned short listen_port_no,
       const char * local_in_logfile_name,
       const char * local_out_logfile_name,
       const char * NIC_IPaddress,
       vrpn_Endpoint_IP * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    vrpn_Connection(local_in_logfile_name, local_out_logfile_name, epa),
    listen_udp_sock (INVALID_SOCKET),
    listen_tcp_sock (INVALID_SOCKET),
    d_NIC_IP(NULL)
{
  // Copy the NIC_IPaddress so that we do not have to rely on the caller
  // to keep it from changing.
  if (NIC_IPaddress != NULL) {
    char *IP = new char[strlen(NIC_IPaddress) + 1];
    if (IP == NULL) {
      fprintf(stderr,"vrpn_Connection_IP::vrpn_Connection_IP(): Out of memory\n");
    } else {
      strcpy(IP, NIC_IPaddress);
      d_NIC_IP = IP;
    }
  }

  // Initialize the things that must be for any constructor
  vrpn_Connection_IP::init();

  listen_udp_sock = ::open_udp_socket(&listen_port_no, NIC_IPaddress);
  // TCH OHS HACK
  listen_tcp_sock = ::open_tcp_socket(&listen_port_no, NIC_IPaddress);
  if ((listen_udp_sock == INVALID_SOCKET) ||
      (listen_tcp_sock == INVALID_SOCKET)) {
    connectionStatus = BROKEN;
    return;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_I{.\n");
  } else {
    connectionStatus = LISTEN;
//fprintf(stderr, "LISTEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
#ifdef	VERBOSE
    printf("vrpn: Listening for requests on port %d\n",listen_port_no);
#endif
  }

  // TCH OHS HACK
  if (listen(listen_tcp_sock, 1)) {
    fprintf(stderr, "Couldn't listen on TCP listening socket.\n");
    connectionStatus = BROKEN;
    return;
  }

  flush_udp_socket(listen_udp_sock);
  
  vrpn_ConnectionManager::instance().addConnection(this, NULL);
}

vrpn_Connection_IP::vrpn_Connection_IP
      (const char * station_name, int port,
       const char * local_in_logfile_name,
       const char * local_out_logfile_name,
       const char * remote_in_logfile_name,
       const char * remote_out_logfile_name,
       const char * NIC_IPaddress,
       vrpn_Endpoint_IP * (* epa) (vrpn_Connection *, vrpn_int32 *)) :
    vrpn_Connection(local_in_logfile_name, local_out_logfile_name,
      remote_in_logfile_name, remote_out_logfile_name, epa),
    listen_udp_sock (INVALID_SOCKET),
    listen_tcp_sock (INVALID_SOCKET),
    d_NIC_IP (NULL)
{
  vrpn_Endpoint_IP * endpoint;
  vrpn_bool isrsh;
  vrpn_bool istcp;
  int retval;

  // Copy the NIC_IPaddress so that we do not have to rely on the caller
  // to keep it from changing.
  if (NIC_IPaddress != NULL) {
    char *IP = new char[strlen(NIC_IPaddress) + 1];
    if (IP == NULL) {
      fprintf(stderr,"vrpn_Connection_IP::vrpn_Connection_IP(): Out of memory\n");
    } else {
      strcpy(IP, NIC_IPaddress);
      d_NIC_IP = IP;
    }
  }

  isrsh = (strstr(station_name, "x-vrsh:") ? VRPN_TRUE : VRPN_FALSE);
  istcp = (strstr(station_name, "tcp:") ? VRPN_TRUE : VRPN_FALSE);

  // Initialize the things that must be for any constructor
  vrpn_Connection_IP::init();

  endpoint = d_endpoints[0];  // shorthand
  endpoint->setNICaddress(d_NIC_IP);

  // If we are not a TCP-only or remote-server-starting
  // type of connection, then set up to lob UDP packets
  // to the other side and put us in the mode that will
  // wait for the responses. Go ahead and set up the TCP
  // socket that we will listen on and lob a packet.

  if (!isrsh && !istcp) {
    // Open a connection to the station using a UDP request
    // that asks to machine to call us back here.
    endpoint->remote_machine_name = vrpn_copy_machine_name(station_name);
    if (!endpoint->remote_machine_name) {
      fprintf(stderr, "vrpn_Connection_IP: Can't ge remote machine name!\n");
      connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
      return;
    }
    if (port < 0) {
  		endpoint->d_remote_port_number = vrpn_DEFAULT_LISTEN_PORT_NO;
    } else {
  		endpoint->d_remote_port_number = port;
    }

    endpoint->status = TRYING_TO_CONNECT;

//fprintf(stderr, "TRYING_TO_CONNECT - vrpn_Connection_IP::vrpn_Connection_IP.\n");

#ifdef	VERBOSE
	  printf("vrpn_Connection_IP: Getting the TCP port to listen on\n");
#endif
    // Set up the connection that we will listen on.
    if (vrpn_get_a_TCP_socket(&endpoint->d_tcpListenSocket,
  			      &endpoint->d_tcpListenPort,
                              NIC_IPaddress) == -1) {
  	fprintf(stderr,"vrpn_Connection_IP: Can't create listen socket\n");
  	endpoint->status = BROKEN;
	endpoint->d_tcpListenSocket = INVALID_SOCKET;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
  	return;
    }

    // Lob a packet asking for a connection on that port.
    vrpn_gettimeofday(&endpoint->d_last_connect_attempt, NULL);
    if (vrpn_udp_request_lob_packet(endpoint->remote_machine_name,
  			            endpoint->d_remote_port_number,
  			            endpoint->d_tcpListenPort,
                                    NIC_IPaddress) == -1) {
  	fprintf(stderr,"vrpn_Connection_IP: Can't lob UDP request\n");
  	endpoint->status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
      return;
    }

    // the next line is something jeff added to get vrpn to work on his
    // PC.  Tom Hudson independently added a line that is similar, but
    // different.  I'm (jeff) am not sure what the right thing is here.
    //
    // Let's do both, because otherwise connectionStatus is
    // never initialized, and doing_ok() returns FALSE sometimes.
    connectionStatus = TRYING_TO_CONNECT;  // XXX jeff's addition
   
    // here is the line that Tom added
    endpoint->status = TRYING_TO_CONNECT;

    // See if we have a connection yet (wait for 1 sec at most).
    // This will allow the connection to come up fast if things are
    // all set. Otherwise, we'll drop into re-sends when we get
    // to mainloop().
    retval = vrpn_poll_for_accept(endpoint->d_tcpListenSocket,
  			          &endpoint->d_tcpSocket, 1.0);
    if (retval  == -1) {
  	fprintf(stderr,
  	  "vrpn_Connection_IP: Can't poll for accept\n");
  	connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
  	return;
    }
    if (retval == 1) {	// Got one!
      endpoint->status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection_IP::vrpn_Connection_IP.\n");
#ifdef	VERBOSE
      printf("vrpn: Connection established on initial try\n");
#endif
      // Set up the things that need to happen when a new connection
      // is established.
      if (endpoint->setup_new_connection()) {
        fprintf(stderr, "vrpn_Connection_IP: "
  		      "Can't set up new connection!\n");
        drop_connection(0);
        //status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
        return;
      }
    }
  }

  // TCH OHS HACK
  if (istcp) {
    endpoint->remote_machine_name = vrpn_copy_machine_name(station_name);
    if (!endpoint->remote_machine_name) {
      fprintf(stderr, "vrpn_Connection_IP: Can't get remote machine name for tcp: connection!\n");
      connectionStatus = BROKEN;
      return;
    }
    endpoint->d_remote_port_number = port;

    // Since we are doing a TCP connection, tell the endpoint not to try and
    // use any other communication mechanism to get to the server.
    endpoint->d_tcp_only = vrpn_TRUE;

    endpoint->status = TRYING_TO_CONNECT;

#ifdef	VERBOSE
    printf("vrpn_Connection_IP: Getting the TCP port to connect with.\n");
#endif

    // Set up the connection that we will connect with.
    // Blocks, doesn't it?
    retval = endpoint->connect_tcp_to(endpoint->remote_machine_name, port);

    if (retval == -1) {
	fprintf(stderr,"vrpn_Connection_IP: Can't create TCP connection.\n");
  	endpoint->status = BROKEN;
  	return;
    }

    connectionStatus = TRYING_TO_CONNECT;
    endpoint->status = TRYING_TO_CONNECT;

    if (endpoint->setup_new_connection()) {
      fprintf(stderr, "vrpn_Connection_IP: "
		      "Can't set up new connection!\n");
      drop_connection(0);
      return;
    }
  }

  // If we are a remote-server-starting type of connection,
  // Try to start the remote server and connect to it.  If
  // we fail, then the connection is broken. Otherwise, we
  // are connected.

  if (isrsh) {
    // Start up the server and wait for it to connect back
    char * machinename;
    char * server_program;
    char * server_args;   // server program plus its arguments
    char * token;

    machinename = vrpn_copy_machine_name(station_name);
    server_program = vrpn_copy_rsh_program(station_name);
          server_args = vrpn_copy_rsh_arguments(station_name);
    token = server_args;
    // replace all argument separators (',') with spaces (' ')
    while ( (token = strchr(token, ',')) != NULL) { *token = ' '; }

    endpoint->d_tcpSocket = vrpn_start_server(machinename, server_program, 
  						     server_args,
                                                 NIC_IPaddress);
    if (machinename) delete [] (char *) machinename;
    if (server_program) delete [] (char *) server_program;
    if (server_args) delete [] (char *) server_args;

    if (endpoint->d_tcpSocket < 0) {
        fprintf(stderr, "vrpn_Connection_IP:  "
  		      "Can't open %s\n", station_name);
        endpoint->status = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
        return;
    } else {
  	endpoint->status = COOKIE_PENDING;
//fprintf(stderr, "COOKIE_PENDING - vrpn_Connection_IP::vrpn_Connection_IP.\n");

        if (endpoint->setup_new_connection()) {
          fprintf(stderr, "vrpn_Connection_IP:  "
			      "Can't set up new connection!\n");
	      drop_connection(0);
	      connectionStatus = BROKEN;
//fprintf(stderr, "BROKEN - vrpn_Connection_IP::vrpn_Connection_IP.\n");
	      return;
        }

    }
  }

  vrpn_ConnectionManager::instance().addConnection(this, station_name);
}

vrpn_Connection_IP::~vrpn_Connection_IP (void) {

  vrpn_int32 i;

  // Remove myself from the "known connections" list
  //   (or the "anonymous connections" list).
  vrpn_ConnectionManager::instance().deleteConnection(this);

  // Send any pending messages
  send_pending_reports();

  // Close the UDP and TCP listen endpoints if we're a server
  if (listen_udp_sock != INVALID_SOCKET) {
	vrpn_closeSocket(listen_udp_sock);
  }
  if (listen_tcp_sock != INVALID_SOCKET) {
	vrpn_closeSocket(listen_tcp_sock);
  }

  if (d_NIC_IP) {
    delete [] d_NIC_IP;
    d_NIC_IP = NULL;
  }

#ifdef VRPN_USE_WINSOCK_SOCKETS

  if (WSACleanup() == SOCKET_ERROR) {
      fprintf(stderr, "~vrpn_Connection_IP():  "
              "WSACleanup() failed with error code %d\n",
      WSAGetLastError());
  }

#endif  // VRPN_USE_WINSOCK_SOCKETS

  for (i = 0; i < d_numEndpoints; i++) {
    if (d_endpoints[i]) {
      d_endpoints[i]->drop_connection();
      delete d_endpoints[i];
    }
  }
}



// utility routines to parse names (<service>@<URL>)
char * vrpn_copy_service_name (const char * fullname)
{
  if (fullname == NULL) {
    return NULL;
  } else {
    int len = 1 + strcspn(fullname, "@");
    char * tbuf = new char [len];
    if (!tbuf)
      fprintf(stderr, "vrpn_copy_service_name:  Out of memory!\n");
    else {
      strncpy(tbuf, fullname, len - 1);
      tbuf[len - 1] = 0;
    }
    return tbuf;
  }
}

char * vrpn_copy_service_location (const char * fullname)
{
  // If there is no "@" sign in the string, copy the whole string.
  int offset = strcspn(fullname, "@");
  int len = strlen(fullname) - offset;
  if (len == 0) {
    offset = -1;  // We add one to it below.
    len = strlen(fullname) + 1; // We subtract one from it below.
  }
  char * tbuf = new char [len];
  if (!tbuf)
    fprintf(stderr, "vrpn_copy_service_name:  Out of memory!\n");
  else {
    strncpy(tbuf, fullname + offset + 1, len - 1);
    tbuf[len - 1] = 0;
  }
  return tbuf;
}

char * vrpn_copy_file_name (const char * filespecifier)
{
  char * filename;
  const char * fp;
  int len;

  fp = filespecifier;
  if (!fp) return NULL;

  if (!strncmp(fp, "file://", 7)) {
    fp += 7;
  } else if (!strncmp(fp, "file:", 5)) {
    fp += 5;
  }

  len = 1 + strlen(fp);
  filename = new char [len];
  if (!filename) 
    fprintf(stderr, "vrpn_copy_file_name:  Out of memory!\n");
  else {
    strncpy(filename, fp, len - 1);
    filename[len - 1] = 0;
  }
  return filename;
}

// Returns the length in characters of the header on the name that is
// passed to it.  Helper routine for those that follow.

static int header_len(const char *hostspecifier) {
  // If the name begins with "x-vrpn://" or "x-vrsh://" or "tcp://" skip that
  // (also handle the case where there is no // after the colon).
  if (!strncmp(hostspecifier, "x-vrpn://", 9) || 
      !strncmp(hostspecifier, "x-vrsh://", 9)) {
	  return 9;
  } else if (!strncmp(hostspecifier, "x-vrpn:", 7) ||
		     !strncmp(hostspecifier, "x-vrsh:", 7)) {
	  return 7;
  } else if (!strncmp(hostspecifier, "tcp://", 6)) {
	  return 6;
  } else if (!strncmp(hostspecifier, "tcp:", 4)) {
	  return 4;
  } else if (!strncmp(hostspecifier, "mpi://", 6)) {
	  return 6;
  } else if (!strncmp(hostspecifier, "mpi:", 4)) {
	  return 4;
  }

  // No header found.
  return 0;
}

char * vrpn_copy_machine_name (const char * hostspecifier)
{
  int nearoffset = 0;
  int faroffset;
    // if it contains a ':', copy only the prefix before the last ':'
    // otherwise copy all of it
  int len;
  char * tbuf;

  // Skip past the header, if any; this includes any tcp:// or tcp:
  // at the beginning of the string.
  nearoffset = header_len(hostspecifier);

  // stop at first occurrence of :<port #> or /<rsh arguments>.
  // Note that this may be the beginning of the string, right at
  // nearoffset.
  faroffset = strcspn(hostspecifier + nearoffset, ":/");
  len = 1 + faroffset;

  tbuf = new char [len]; //XXX Memory leak, but a small one
  if (!tbuf) {
    fprintf(stderr, "vrpn_copy_machine_name:  Out of memory!\n");
  } else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len - 1] = 0;
  }
  return tbuf;
}


int vrpn_get_port_number (const char * hostspecifier)
{
  const char * pn;
  int port = vrpn_DEFAULT_LISTEN_PORT_NO;

  pn = hostspecifier;
  if (!pn) return -1;

  // Skip over the header, if present
  pn += header_len(hostspecifier);

  pn = strrchr(pn, ':');
  if (pn) {
    pn++;
    port = atoi(pn);
  }

  return port;
}

char * vrpn_copy_rsh_program (const char * hostspecifier)
{
  int nearoffset = 0; // location of first char after machine name
  int faroffset; // location of last character of program name
  int len;
  char * tbuf;

  nearoffset += header_len(hostspecifier);

  nearoffset += strcspn(hostspecifier + nearoffset, "/");
  nearoffset++; // step past the '/'
  faroffset = strcspn(hostspecifier + nearoffset, ",");
  len = 1 + (faroffset ? faroffset : strlen(hostspecifier) - nearoffset);
  tbuf = new char [len];

  if (!tbuf)
    fprintf(stderr, "vrpn_copy_rsh_program: Out of memory!\n");
  else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len-1] = 0;
    //fprintf(stderr, "server program: '%s'.\n", tbuf);
  }
  return tbuf;
}

char * vrpn_copy_rsh_arguments (const char * hostspecifier)
{
  int nearoffset = 0; // location of first char after server name
  int faroffset; // location of last character
  int len;
  char * tbuf;

  nearoffset += header_len(hostspecifier);

  nearoffset += strcspn(hostspecifier + nearoffset, "/");
  nearoffset += strcspn(hostspecifier + nearoffset, ",");
  faroffset = strlen(hostspecifier);
  len = 1 + faroffset - nearoffset;
  tbuf = new char [len];

  if (!tbuf)
    fprintf(stderr, "vrpn_copy_rsh_arguments: Out of memory!\n");
  else {
    strncpy(tbuf, hostspecifier + nearoffset, len - 1);
    tbuf[len-1] = 0;
    //fprintf(stderr, "server args: '%s'.\n", tbuf);
  }
  return tbuf;
}

// For a host specifier without a service name, this routine prepends
//  the given string newServiceName to it.
// For a host specifier with a service name (e.g. "service@URL"),
//  this routine strips off the service name and adds the given
//  string newServiceName in its place (e.g. "newServiceName@URL").
// This routine allocates memory for the return value.  Caller is
//  responsible for freeing the memory.
char * vrpn_set_service_name(const char * specifier, const char * newServiceName)
{
  size_t inputLength = strlen(specifier);
  size_t atSymbolIndex = strcspn(specifier, "@");

  char * location = NULL;

  if (atSymbolIndex == inputLength) {
    // no @ symbol present; just a location.
    location = new char[inputLength+1];
    strcpy(location, specifier);  // take the whole thing to be the location
  }
  else {
    // take everything after the @ symbol to be the location
    location = vrpn_copy_service_location(specifier);
  }

  // prepend newServiceName to location.
  size_t len = strlen(location) + strlen(newServiceName);
  char * newSpecifier = new char[len+2];   // extra space for '@'
                                           //  and terminal '/0'
  strcpy(newSpecifier, newServiceName);
  strcat(newSpecifier, "@");
  strcat(newSpecifier, location);
  return newSpecifier;
}
