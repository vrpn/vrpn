#ifndef	VRPN_SERIAL_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/time.h>
#endif

// flush discards characters in buffer
// drain blocks until they are written
extern int vrpn_open_commport(char *portname, long baud);
extern int vrpn_flush_input_buffer( int comm );
extern int vrpn_flush_output_buffer( int comm );
extern int vrpn_drain_output_buffer( int comm );
extern int vrpn_read_available_characters(int comm, unsigned char *buffer,
		int count);

#define VRPN_SERIAL_H
#endif
