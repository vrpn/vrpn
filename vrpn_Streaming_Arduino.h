// Copyright 2015 by Russ Taylor, working for ReliaSolve.
// Based on the vrpn_Tng3.h header file.
// License: Standard VRPN.
//
// See the vrpn_streaming_arduino directory for a program that should be
// loaded onto the Arduino and be running for this device to connect to.

#pragma once
#include "vrpn_Analog.h"                // for vrpn_Serial_Analog
#include "vrpn_Button.h"                // for vrpn_Button_Filter
#include <string>

class VRPN_API vrpn_Streaming_Arduino: public vrpn_Serial_Analog,
			 public vrpn_Button_Filter
{
 public:
   vrpn_Streaming_Arduino(std::string name,
     vrpn_Connection * c,
     std::string port,
     int numchannels = 1,
     int baud = 115200);

   ~vrpn_Streaming_Arduino()  {};

    // Called once through each main loop iteration to handle
    // updates.
    virtual void mainloop (void);
    
 protected:
    int m_status;
    int m_numchannels;	// How many analog channels to open

    std::string m_buffer; // Characters read from the device.
    struct timeval m_timestamp;	// Time of the last report from the device

    virtual int get_report(void);	// Try to read a report from the device

    // send report iff changed
    virtual void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
    // send report whether or not changed
    virtual void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);

    // NOTE:  class_of_service is only applied to vrpn_Analog
    //  values, not vrpn_Button or vrpn_Dial

    void clear_values();
    int reset();

 private:
};
