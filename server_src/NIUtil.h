#ifndef NIUtil_Util
#define NIUtil_Util

/*** NIUtil Class - Handy utility functions for getting the correct board
 *   and handling errors. Originally none of these were supplied by NI,
 *   now some of them are.
 ***/

#include <nidaqex.h>


namespace NIUtil
{

   const char *nameCodeToString
   (
      int code
   ); ///< Convert NI-DAQ device name code into meaningful string

   const char *getDeviceName
   (
      int deviceNumber
   ); ///< Get name of some device

   unsigned long getDeviceCode
   (
      short deviceNumber
   ); ///< Get device code of some device

   short findDevice
   (
      const char *name
   );
   ///< Get code for installed device matching name.  If the same program
   // asks for a device with the same name again, then it will look for a
   // second instance of a device with that name (it will not return the
   // same device ID twice).

   int checkError
   (
      int    status,
      const char * message,
      bool   warn
   ); ///< Error/Warning messages for NI-DAQ calls
}


#endif
