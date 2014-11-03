// vrpn_Tracker_LibertyHS.h
//	This file contains the class header for a High Speed Polhemus Liberty
// Latus Tracker.
// This file is based on the vrpn_Tracker_Liberty.h file, with modifications made
// to allow it to operate a Liberty Latus instead. It has been tested on Linux.

#ifndef VRPN_TRACKER_LIBERTYHS_H
#define VRPN_TRACKER_LIBERTYHS_H

#include <stdio.h>                      // for NULL

#include "vrpn_Configure.h"             // for VRPN_API, etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Tracker.h"               // for vrpn_Tracker_USB
#include "vrpn_Types.h"                 // for vrpn_uint16, vrpn_uint8, etc

class VRPN_API vrpn_Connection;

#if defined(VRPN_USE_LIBUSB_1_0)

// Vendor and product IDs for High Speed Liberty Latus tracker
static const vrpn_uint16 LIBERTYHS_VENDOR_ID  = 0x0f44;
static const vrpn_uint16 LIBERTYHS_PRODUCT_ID = 0xff20;

// Endpoints to communicate with the USB device
static const vrpn_uint8 LIBERTYHS_WRITE_EP = 0x04;
static const vrpn_uint8 LIBERTYHS_READ_EP  = 0x88;

const int vrpn_LIBERTYHS_MAX_STATIONS = 8;       //< How many stations (i.e. markers) can exist
const int vrpn_LIBERTYHS_MAX_WHOAMI_LEN = 1024;  //< Maximum whoami response length
const int vrpn_LIBERTYHS_MAX_MARKERMAP_LEN = 12; //< Maximum active marker map response length


class VRPN_API vrpn_Tracker_LibertyHS: public vrpn_Tracker_USB {
  
 public:

  /// The constructor is given the name of the tracker (the name of
  /// the sender it should use), the connection on which it is to
  /// send its messages, the baud rate at which it is to communicate 
  /// (default 115200), whether filtering is enabled (default yes), 
  /// the number of stations that are possible on this LibertyHS (default 8) 
  /// and the receptor used to launch the stations (default 1). 
  /// The final parameter is a string that can contain additional
  /// commands that are set to the tracker as part of its reset routine.
  /// These might be used to set the hemisphere or other things that are
  /// not normally included; see the Liberty Latus manual for a list of these.
  /// There can be multiple lines of them but putting <CR> into the string.

  vrpn_Tracker_LibertyHS(const char *name, vrpn_Connection *c, 
                         long baud = 115200, int enable_filtering = 1, 
                         int numstations = vrpn_LIBERTYHS_MAX_STATIONS,
                         int receptoridx = 1, const char *additional_reset_commands = NULL, 
                         int whoamilen = 288);

  ~vrpn_Tracker_LibertyHS();
    
 protected:
  
  virtual int get_report(void);
  virtual void reset();


  int	do_filter;		//< Should we turn on filtering for pos/orient?
  int	num_stations;		//< How many stations (trackers) on this LibertyHS?
  int   receptor_index;         //< Index of receptor used to detect and launch the markers
  int   num_resets;		//< Number of resets we've tried this time.
  char	add_reset_cmd[2048];	//< Additional reset commands to be sent
  int   whoami_len;   //< Number of chars in whoami response
  int   read_len;     //< Number of bytes in usb read buffer
  int   sync_index;   //< Index of first sync char in usb read buffer

  struct timeval liberty_zerotime;  //< When the liberty time counter was zeroed
  struct timeval liberty_timestamp; //< The time returned from the LibertyHS System
  vrpn_uint32	 REPORT_LEN;	    //< The length that the current report should be


  /// Augments the basic LibertyHS format
  int  set_sensor_output_format(int sensor = -1);

  /// Augments the basic LibertyHS report length
  int	report_length(int sensor);

  /// Writes len bytes from data buffer to USB device
  int  write_usb_data(void* data,int len);

  /// Reads at most maxlen bytes from USB device and copy them into 
  /// data buffer. Returns the number of read bytes.
  /// The maximum period in milliseconds to wait for reading USB data
  /// is defined by parameter timeout.
  int  read_usb_data(void* data,int maxlen,unsigned int timeout = 50);

  /// Empties the USB read buffer
  void flush_usb_data();

  /// Launches num_stations markers using receptor receptor_index to detect them.
  int  test_markers();

  /// Returns the number of detected and lauched trackers
  int  launch_markers();
};

// End of LIBUSB
#endif

// End of VRPN_TRACKER_LIBERTYHS_H
#endif
