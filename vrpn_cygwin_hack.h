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
 $Revision: 1.1 $
*****************************************************************************/

#ifndef _VRPN_CYGWIN_HACK_H
#define _VRPN_CYGWIN_HACK_H

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

int gettimeofday(struct timeval *, struct timezone *);

#ifdef __cplusplus
}
#endif

#endif // __CYGWIN__

#endif

/*****************************************************************************\
  $Log: vrpn_cygwin_hack.h,v $
  Revision 1.1  1999/04/14 21:23:48  lovelace
  Added #ifdefs so that vrpn will compile under the Cygnus solutions
  cygwin environment.  Also modified Makefile so that it correctly
  compiles under the cygwin environment.


\*****************************************************************************/


