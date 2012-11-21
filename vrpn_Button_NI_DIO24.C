// vrpn_Button_NI_DIO24.C
//
//      This is a driver for National Instruments DAQCard
// DIO-24, a PCMCIA card, which provides 24-bit digital I/O.  
// The I/O is accessed in 3 "ports" with 8 bits per port,
// though the user is protected from that detail.  The
// user of this class need only request inputs 1 through 24.
//
// Unlike the other National Instrument devices currently
// in vrpn, this uses their new "mx" library.  To access
// that library, install their software from the NI-DAQmx
// CD.  Then uncomment the following line in vrpn_configure.h:
//   #define VRPN_USE_NATIONAL_INSTRUMENTS_MX
//
// Note that because the 3rd party library is used, this class
// will only work under Windows.
//
// You must also include the following in your compilers include
// path for the 'vrpn' and 'vrpn_server' projects:
//   $(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C DEV\include
//
// Finally, the following must be included in vrpn.cfg to use
// the generic server:
//   
//   ################################################################################
//   #      This is a driver for National Instruments DAQCard-
//   # DIO-24, a PCMCIA card, which provides 24-bit digital I/O.
//   #
//   # Arguments:
//   #       char    name_of_this_device[]
//   #       int     number_of_channls to read: 1-24 (optional.  default=24)
//   
//   vrpn_Button_NI_DIO24    Button0 1
//
// This code was written in October 2006 by Bill West, based on some example
// code provided by National Instruments.

#include "vrpn_Button_NI_DIO24.h"

class VRPN_API vrpn_Connection;
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
extern "C" {
#include <NIDAQmx.h>
}
#else
typedef	vrpn_int32	int32;
#endif
#include <stdio.h>                      // for fprintf, sprintf, stderr, etc

//  Constants used by this class
const vrpn_int32 vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX = 
    (24<vrpn_BUTTON_MAX_BUTTONS) ? 24 : vrpn_BUTTON_MAX_BUTTONS ;    //  pick the least

vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24 (const char * name,
                                        vrpn_Connection * c,
                                        vrpn_int32 numChannels) :
vrpn_Button_Filter (name, c)
{
    char    portName[127] ;
    
    this->setNumChannels( numChannels );

    // Check if we got a connection.
    if (d_connection == NULL) {
        fprintf(stderr,"vrpn_Button_NI_DIO24: Can't get connection!\n");
    }    

    //  Initialize the task handles
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    _taskHandle = 0 ;
#else
    fprintf(stderr,"vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24(): Not compiled into VRPN, edit vrpn_Configure.h and define VRPN_USE_NATIONAL_INSTRUMENTS_MX and then recompile VRPN.\n");
#endif

    //  Initialize the DAQCard-DIO-24 for each of the ports used
    //  Define various names the library needs
    sprintf(portName,"Dev1/line0:%d", num_buttons-1) ;

    /*********************************************/
    // DAQmx Configure Code
    /*********************************************/
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    int32    error=0;
    error = DAQmxCreateTask(d_servicename,&_taskHandle);

    if (!error)
        error = DAQmxCreateDIChan(_taskHandle,portName,"",DAQmx_Val_ChanForAllLines);
#endif

/*  The following code *should* make the 0's into 1's, and vice versa, but it only works
 *  for one channel, and even then, it causes the 1's (button pressed) to "flicker" 1-0-1.
 *  National Instruments can't explain it.  Furthermore, if you try to set this property
 *  for all inputs, it errors out.
 *
 *  if (!error)
 *      error = DAQmxSetDIInvertLines(_taskHandle, portName, true) ;
 */

    /*********************************************/
    // DAQmx Start Code
    /*********************************************/
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    if (!error)
        error = DAQmxStartTask(_taskHandle);

    //  If error, report it and exit
    if (error)
        reportError(error, vrpn_true) ;
#endif

}    //  constructor

//  This destructor closes out the SEI bus, and deallocates memory
vrpn_Button_NI_DIO24::~vrpn_Button_NI_DIO24()
{
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    if( _taskHandle!=0 ) 
    {
        /*********************************************/
        // DAQmx Stop Code
        /*********************************************/
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = 0 ;
    }
#endif
}    //  destructor

/** This routine is called each time through the server's main loop. It will
    take a course of action depending on the current status of the device,
    either trying to reset it or trying to get a reading from it.  It will
    try to reset the device if no data has come from it for a couple of
    seconds
*/

void    vrpn_Button_NI_DIO24::mainloop()
{

    server_mainloop();    //  let the server do its stuff

    //  Read the data from the available ports
    /*********************************************/
    // DAQmx Read Code
    /*********************************************/
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    int32    error ;
    int32    bytesRead,bytesPerSamp;
    error = DAQmxReadDigitalLines(_taskHandle,1,5.0,DAQmx_Val_GroupByChannel,
                    buttons, vrpn_BUTTON_MAX_BUTTONS,
                    &bytesPerSamp,&bytesRead,NULL);

    //  check for obvious errors
    if (bytesRead < num_buttons)
        fprintf(stderr, 
            "vrpn_Button_NI_DIO24: Warning, number of bytes read was %d, not %d as expected.\n",
            bytesRead, num_buttons) ;
    //  If other error, report it and sleep
    if (error)
            reportError(error, vrpn_false) ;
#endif

    //  Finally, the point of all this, deliver the data
    report_changes() ;

}    // mainloop

vrpn_int32 vrpn_Button_NI_DIO24::setNumChannels (vrpn_int32 sizeRequested) 
{
    //  Set the number of buttons we have to read
    //  If the number is invalid, default to the maximum
    if (sizeRequested<1 || sizeRequested > vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX) 
        num_buttons = vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX;
    else
        num_buttons = sizeRequested;

    this->set_all_momentary() ;    // as the default case anyway
    
    return num_buttons;

}    //  setNumChannels

//  This handles error reporting
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
void vrpn_Button_NI_DIO24::reportError(int32 errnumber, vrpn_bool exitProgram)
{
    char    errBuff[2048]={'\0'};

    if( DAQmxFailed(errnumber) )
    {
        DAQmxGetExtendedErrorInfo(errBuff,2048);
        printf("DAQmx Error: %s\n",errBuff);
        if (exitProgram==vrpn_true)
        {
            printf("Exiting...\n") ;
            throw(errnumber) ;    //  this will quit, cause the destructor to be called
        }
        else
        {
            printf("Sleeping...\n") ;
            vrpn_SleepMsecs(1000.0*1) ;    //  so at least the log will slow down so someone can see the error
        }
    }
}    //  reportError
#endif

