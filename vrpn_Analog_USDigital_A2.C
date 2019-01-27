// vrpn_Analog_USDigital_A2.C
//
//      This is a driver for USDigital A2 Absolute Encoders.
// They can be daisy changed together, and utlimately, one or
// more plug into a serial port and communicate using RS-232.
// You can find out more at www.usdigital.com.
//
// To use this class, install the US Digital software, specifying
// the "SEI Explorer Demo Software" to install.
//
// Then uncomment the following line in vrpn_configure.h:
//   #define VRPN_USE_USDIGITAL
//
// Note that because the 3rd party library is used, this class
// will only work under Windows.
//
// You must also include the following in your compilers include
// path for the 'vrpn' project:
//   $(SYSTEMDRIVE)\Program Files\SEI Explorer
//
// Finally, the following must be included in vrpn.cfg to use
// the generic server:
//
//   ################################################################################
//   # US Digital A2 Absolute Encoder Analog Input server.  This will open the COM
//   # port specified, configure the number of channels specified, and report
//   # Absolute Encoder values in tenths of a degree from 0 to 3599.
//   #
//   # Arguments:
//   #       char    name_of_this_device[]
//   #       int     COM_port.  If 0, search for correct COM port.
//   #       int     number_of_channels
//   #       int     0 to report always, 1 to report on change only (optional, default=0)
//   
//   vrpn_Analog_USDigital_A2        Analog0 0       2
//
// This code was written in October 2006 by Bill West, who
// used the vrpn_Analog_Server sample code written by 
// Tom Hudson in March 1999 as a starting point.  Bill also
// used some ideas from vrpn_Radamec_SPI.[Ch] written by
// Russ Taylor in August 2000.

#include "vrpn_Analog_USDigital_A2.h"

class VRPN_API vrpn_Connection;
#ifdef VRPN_USE_USDIGITAL
extern "C" {
#include <SEIDrv32.H>
}
#endif
#include <stdio.h>                      // for fprintf, stderr

//  Constants used by this class
const vrpn_uint32 vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX = 
    //  deallocate the list of device addresses.
    (15<vrpn_CHANNEL_MAX) ? 15 : vrpn_CHANNEL_MAX ;    //  pick the least
const vrpn_uint32 vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_FIND_PORT = 0 ;

//  Constructor initializes the USDigital's SEI communication, and prepares to read the A2
vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2 (const char * name,
                                                    vrpn_Connection * c,
                                                    vrpn_uint32 portNum,
                                                    vrpn_uint32 numChannels,
                                                    vrpn_int32 reportOnChangeOnly) :
vrpn_Analog (name, c),
_SEIopened(vrpn_false),
_devAddr(NULL),
_reportChange(reportOnChangeOnly!=0),
_numDevices(0)
{
#ifdef VRPN_USE_USDIGITAL
    this->_devAddr = new long[vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX];
    this->setNumChannels( numChannels );

    // Check if we got a connection.
    if (d_connection == NULL) {
        fprintf(stderr,"vrpn_Analog_USDigital_A2: Can't get connection!\n");
	return;
    }    

    //  Prepare to get data from the SEI bus
    long err;
#ifdef VRPN_USE_USDIGITAL
    err = InitializeSEI(portNum, AUTOASSIGN) ;
#else
    fprintf(stderr,"vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2(): Not yet implemented for this architecture\n");
    err = -1;
#endif
    if (err) {
        fprintf(stderr, "vrpn_Analog_USDigital_A2:  Can't initialize SEI bus for port %d.\n", 
#ifdef VRPN_USE_USDIGITAL
        GetCommPort()
#else
	0
#endif
	);

        return ;
    } else {
        _SEIopened = vrpn_true ;
    }

    //  Check if the number of devices matches that expected
#ifdef VRPN_USE_USDIGITAL
    _numDevices = GetNumberOfDevices() ;
#endif
    if (_numDevices<0 || _numDevices>vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX) {
        fprintf(stderr,
            "vrpn_Analog_USDigital_A2:  Error (%d) returned from GetNumberOfDevices call on SEI bus",
            _numDevices) ;
        _numDevices = 0 ;
    }
    if (_numDevices != numChannels)
        fprintf(stderr, 
    "vrpn_Analog_USDigital_A2:  Warning, number of requested devices (%d) is not the same as found (%d)\n", 
        numChannels, _numDevices) ;

    //  Initialize the addresses
    for (int c=0 ; c<vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX ; c++)
        _devAddr[c] = -1 ;

    //  Get the device addresses.
    for (vrpn_uint32 d=0 ; d<_numDevices ; d++) {
        long deviceInfoErr, model, serialnum, version, addr ;
#ifdef VRPN_USE_USDIGITAL
        deviceInfoErr = GetDeviceInfo(d, &model, &serialnum, &version, &addr) ;
        if (!deviceInfoErr)
           _devAddr[d] = addr ;
#endif

#ifdef VERBOSE
        //  Dump out the device data
        if (deviceInfoErr)
            fprintf(stderr, "vrpn_Analog_USDigital_A2: could not get information on Device #%d!\n", d) ;
        else
            fprintf(stderr, "vrpn_Analog_USDigital_A2: Device #%d: model=%d, serialnum=%d, version=%d, addr=%d\n",
                    d, model, serialnum, version, addr) ;
#endif    //  VERBOSE
    }
#else
    fprintf(stderr,"vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2(): Not compiled in; define VRPN_USE_USDIGITAL in vrpn_Configure.h and recompile VRPN\n");
    portNum = portNum + 1; // Remove unused parameter warning.
    numChannels = numChannels + 1; // Remove unused parameter warning.
    _SEIopened = !_SEIopened; // Removed unused variable warning.
    _devAddr = _devAddr + 1; // Removed unused variable warning.
    _numDevices = _numDevices + 1; // Removed unused variable warning.
#endif
}    //  constructor

//  This destructor closes out the SEI bus, and deallocates memory
vrpn_Analog_USDigital_A2::~vrpn_Analog_USDigital_A2()
{
#ifdef  VRPN_USE_USDIGITAL
    //  close out the SEI bus
    if (_SEIopened==vrpn_true) {
        (void) CloseSEI() ;
    }

    //  deallocate the list of device addresses.
    try {
      delete _devAddr;
    } catch (...) {
      fprintf(stderr, "vrpn_Analog_USDigital_A2::~vrpn_Analog_USDigital_A2(): delete failed\n");
      return;
    }
    _devAddr = 0 ;
#endif
}    //  destructor

/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds
*/

void    vrpn_Analog_USDigital_A2::mainloop()
{
    server_mainloop();    //  let the server do its stuff
#ifdef VRPN_USE_USDIGITAL
    long readErr, readVal ;

    //  Read the data from the available channels
    for (vrpn_uint32 c=0 ; c<(vrpn_uint32) num_channel ; c++)
    {
        //  see if there's really a readable device there.
        if (c<_numDevices && _devAddr[c]>=0)
        {
            readErr = A2GetPosition(_devAddr[c], &readVal) ;
            if (readErr)
            {
                fprintf(stderr, 
                "vrpn_Analog_USDigital_A2: Error code %d received while reading channel %d.\n",
                    readErr, c) ;
                fprintf(stderr, "vrpn_Analog_USDigital_A2: Attempting to reinitialize SEI bus...") ;
                readErr = ResetSEI() ;
                if (readErr)
                    fprintf(stderr, "failed.") ;
                fprintf(stderr, "failed.") ;
                //  don't flood the log, and give the reset time to work
                vrpn_SleepMsecs(1000) ;
            }
            else
                channel[c] = (vrpn_float64) readVal ;
        }
        else
            channel[c] = 0 ;    //  default to 0 for unreadable/unavailable.
    }    //  for
#endif
  
    //  Finally, the point of all this, deliver the data
    if (_reportChange)
        report_changes() ;
    else
        report() ;

}    // mainloop

vrpn_int32 vrpn_Analog_USDigital_A2::setNumChannels (vrpn_int32 sizeRequested) 
{
    if (sizeRequested < 0) 
        num_channel = 0;
    else if (static_cast<unsigned>(sizeRequested) > vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX) 
        num_channel = (vrpn_int32) vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX;
    else
        num_channel = (vrpn_int32) sizeRequested;
  
    return num_channel;

}    //  setNumChannels

