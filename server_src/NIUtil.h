#ifndef NIUtil_Util
#define NIUtil_Util

/*** NIUtil Class - Handy utility functions for getting the correct board
 *   and handling errors. Originally none of these were supplied by NI,
 *   now some of them are.
 ***/

#include <nidaqex.h>


namespace NIUtil
{

   char *nameCodeToString
   (
      int code
   ); ///< Convert NI-DAQ device name code into meaningful string

   char *getDeviceName
   (
      int deviceNumber
   ); ///< Get name of some device

   int getDeviceCode
   (
      int deviceNumber
   ); ///< Get device code of some device

   int findDevice
   (
      const char *name
   ); ///< Get code for installed device matching name

   int checkError
   (
      int    status,
      char * message,
      bool   warn
   ); ///< Error/Warning messages for NI-DAQ calls
}


#endif
