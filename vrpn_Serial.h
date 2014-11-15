#ifndef VRPN_SERIAL_H
#define VRPN_SERIAL_H

#include "vrpn_Configure.h" // for VRPN_API
#include <stddef.h>         // For size_t

/// @file
///
/// @brief vrpn_Serial: Pulls all the serial port routines into one file to make
/// porting to
/// new operating systems easier.
///
/// @author Russ Taylor, 1998

typedef enum {
    vrpn_SER_PARITY_NONE,
    vrpn_SER_PARITY_ODD,
    vrpn_SER_PARITY_EVEN,
    vrpn_SER_PARITY_MARK,
    vrpn_SER_PARITY_SPACE
} vrpn_SER_PARITY;

/// @brief Open a serial port, given its name and baud rate.
///
/// Default Settings are 8 bits, no parity, 1 start and stop bits with no
/// RTS (hardware) flow control.  Also,
/// set the port so that it will return immediately if there are no
/// characters or less than the number of characters requested.
///
/// @returns the file descriptor on success,-1 on failure.
extern VRPN_API int
vrpn_open_commport(const char *portname, long baud, int charsize = 8,
                   vrpn_SER_PARITY parity = vrpn_SER_PARITY_NONE,
                   bool rts_flow = false);

/// @name RTS Hardware Flow Control
/// Set and clear functions for the RTS ("ready to send") hardware flow-
/// control bit.  These are used on a port that is already open.  Some
/// devices (like the Ascension Flock of Birds) use this to reset the
/// device.  Return 0 on success, nonzero on error.
/// @{
extern VRPN_API int vrpn_set_rts(int comm);
extern VRPN_API int vrpn_clear_rts(int comm);
/// @}

extern VRPN_API int vrpn_close_commport(int comm);

/// @brief Throw out any characters within the input buffer.
/// @returns 0 on success, -1 on error.
extern VRPN_API int vrpn_flush_input_buffer(int comm);

/// @brief Throw out any characters (do not send) within the output buffer
/// @returns 0 on success, tc err codes (whatever those are) on error.
extern VRPN_API int vrpn_flush_output_buffer(int comm);

/// @brief Wait until all of the characters in the output buffer are sent, then
/// return.
///
/// @returns 0 on success, -1 on error.
extern VRPN_API int vrpn_drain_output_buffer(int comm);

/// @name Read routines
///
/// Read up the the requested count of characters from the input buffer,
/// return with less if less (or none) are there.  Return the number of
/// characters read, or -1 if there is an error.  The second of these
/// will keep looking until the timeout period expires before returning
/// (NULL pointer will cause it to block indefinitely).
/// @{
extern VRPN_API int
vrpn_read_available_characters(int comm, unsigned char *buffer, size_t count);
extern VRPN_API int vrpn_read_available_characters(int comm,
                                                   unsigned char *buffer,
                                                   size_t count,
                                                   struct timeval *timeout);
/// @}

/// @name Write routines
///
/// Write the specified number of characters.  Some devices can't accept writes
/// that
/// are too fast, so need time between characters; the write_slowly function
/// handles
/// this case.
/// @}
extern VRPN_API int vrpn_write_characters(int comm, const unsigned char *buffer,
                                          size_t bytes);
extern VRPN_API int vrpn_write_slowly(int comm, const unsigned char *buffer,
                                      size_t bytes, int millisec_delay);

#endif
