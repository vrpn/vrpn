#ifndef VRPN_TYPES_H
#define VRPN_TYPES_H

//------------------------------------------------------------------
//   This section contains definitions for architecture-dependent
// types.  It is important that the data sent over a vrpn_Connection
// be of the same size on all hosts sending and receiving it.  Since
// C++ does not constrain the size of 'int', 'long', 'double' and
// so forth, we create new types here that are defined correctly for
// each architecture and use them for all data that might be sent
// across a connection.
//   Part of porting VRPN to a new architecture is defining the
// types below on that architecture in such as way that the compiler
// can determine which machine type it is on.
//------------------------------------------------------------------

#ifdef	sgi
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	hpux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

// For PixelFlow aCC compiler
#ifdef	__hpux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	sparc
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	linux
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	int
#define	vrpn_uint32	unsigned int
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

// _WIN32 is defined for all compilers for Windows (Cygnus G++ included)
// WIN32 is defined only by the Windows VC compiler
#ifdef	_WIN32
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	long
#define	vrpn_uint32	unsigned long
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifdef	FreeBSD
#define	vrpn_int8	char
#define	vrpn_uint8	unsigned char
#define	vrpn_int16	short
#define	vrpn_uint16	unsigned short
#define	vrpn_int32	long
#define	vrpn_uint32	unsigned long
#define	vrpn_float32	float
#define	vrpn_float64	double
#endif

#ifndef	vrpn_int8
XXX	Need to define architecture-dependent sizes here
#endif

typedef vrpn_int16 vrpn_bool;
const vrpn_int16 vrpn_true=1;
const vrpn_int16 vrpn_false=0;

// should we add a success & fail?

#endif //VRPN_TYPES_H
