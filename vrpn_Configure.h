#ifndef	VRPN_CONFIGURE_H
#define	VRPN_CONFIGURE_H

//--------------------------------------------------------------
/* This file contains configuration options for VRPN.  The first
   section has definition lines that can be commented in or out
   at build time.  The second session has automaticly-generated
   directives and should not be edited. */
//--------------------------------------------------------------

//--------------------------------------------------------//
// EDIT BELOW THIS LINE FOR NORMAL CONFIGURATION SETTING. //
//--------------------------------------------------------//

//-----------------------
// Instructs VRPN not to use any iostream library functions, but
// rather to always use C-style FILE objects.  This is to provide
// compatibility with code that uses the other kind of streams than
// VRPN uses.
#define	VRPN_NO_STREAMS

//-----------------------
// Instructs VRPN to use the default room space transforms for
// the Desktop Phantom as used in the nanoManipulator application
// rather than the default world-origin with identity rotation.
// Please don't anyone new use the room space transforms built
// into VRPN -- they are a hack pulled forward from Trackerlib.
#define	DESKTOP_PHANTOM_DEFAULTS

//-----------------------
// Instructs VRPN library and server to include code that uses
// the DirectX SDK (from its standard installation in C:\DXSDK).
// Later in this file, we also instruct the compiler to link with
// the DirectX library if this is defined.
//#define	VRPN_USE_DIRECTINPUT

//-----------------------
// Instructs the VRPN server to create an entry for the Adrienne
// time-code generator.  This is a device that produces time values
// from an analog video stream so that events in the virtual world
// can be synchronized with events on a movie.  The Adrienne folder
// should be located at the same level as the VRPN folder for the
// code to find it.
//#define	INCLUDE_TIMECODE_SERVER

//---------------------------------------------------------------//
// DO NOT EDIT BELOW THIS LINE FOR NORMAL CONFIGURATION SETTING. //
//---------------------------------------------------------------//

// Load DirectX SDK libraries and tell which version we need if we are using it.
#ifdef	VRPN_USE_DIRECTINPUT
#define	DIRECTINPUT_VERSION 0x0800
// Since this probably doesn't match your file system, add them to 
// your project settings, if needed. 
//#pragma comment (lib, "C:/DXSDK/lib/dxguid.lib")
//#pragma comment (lib, "C:/DXSDK/lib/dxerr8.lib")
//#pragma comment (lib, "C:/DXSDK/lib/dinput8.lib")
#endif

// Load Adrienne libraries if we are using the timecode generator.
#ifdef	INCLUDE_TIMECODE_SERVER
#pragma comment (lib, "../../Adrienne/AEC_DLL/AEC_NTTC.lib")
#endif

#endif
