/*****************************************************************************\ 
 This is a hack to get rid of "implicit declaration of int gettimeofday(...)"
 warning messages that we get while compiling under the cygwin environment.
 
 Since under cygwin, we use the supplied gettimeofday function, if we want
 a prototype of it, we need to make sure it is defined as a C function, 
 otherwise we get all kinds of link errors.
  
  ----------------------------------------------------------------------------
 Author: lovelace
 Created: Wed Apr 14 16:56:41 1999 by lovelace
 Revised: 
 $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_cygwin_hack.h,v $
 $Locker:  $
 $Revision: 1.2 $
*****************************************************************************/

#ifndef _VRPN_CYGWIN_HACK_H
#define _VRPN_CYGWIN_HACK_H

#ifdef __CYGWIN__
#include <sys/time.h>
#endif
/*
#ifdef __CYGWIN__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_H_WINDOWS32_SOCKETS

struct timeval {
  long tv_sec;
  long tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
  
#endif // _GNU_H_WINDOWS32_SOCKETS

    //int gettimeofday(struct timeval *, struct timezone *);

#ifdef __cplusplus
}
#endif

#endif // __CYGWIN__
*/
#endif

/*****************************************************************************\
  $Log: vrpn_cygwin_hack.h,v $
  Revision 1.2  1999/10/14 15:54:29  helser
  Changes vrpn_FileConnection so it will allow local logging. I used this
  ability to write a log-file manipulator. I read in a log file, check and
  apply a fix to the timestamps, then send out an identical message to the
  one I received on the file connection, which gets logged. Problem: I don't
  get system messages in the new log file.

  Played with gettimeofday in Cygwin environment, and found it is broken and
  can't be fixed using Han's tricks in vrpn_Shared.C. But, instead found a
  way to get rid of vrpn_cygwin_hack.h - just include sys/time.h AFTER the
  standard windows headers. vrpn_cygwin_hack.h will be totally removed in my
  next commit.

  Added quick hack to make Clock messages system messages. This allows clock
  sync to happen before any user messages are send, if you spin the
  connection's mainloop a few times on each end, and allows the
  File_Connection to replay a log of the session correctly later. This is
  the work-around for the broken Cygwin gettimeofday.

  Revision 1.1  1999/04/14 21:23:48  lovelace
  Added #ifdefs so that vrpn will compile under the Cygnus solutions
  cygwin environment.  Also modified Makefile so that it correctly
  compiles under the cygwin environment.


\*****************************************************************************/


