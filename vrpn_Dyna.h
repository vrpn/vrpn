/*                               -*- Mode: C -*- 
 * 
 * This library is free software; you can redistribute it and/or          
 * modify it under the terms of the GNU Library General Public            
 * License as published by the Free Software Foundation.                  
 * This library is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      
 * Library General Public License for more details.                       
 * If you do not have a copy of the GNU Library General Public            
 * License, write to The Free Software Foundation, Inc.,                  
 * 675 Mass Ave, Cambridge, MA 02139, USA.                                
 *                                                                        
 * For more information on this program, contact Blair MacIntyre          
 * (bm@cs.columbia.edu) or Steven Feiner (feiner@cs.columbia.edu)         
 * at the Computer Science Dept., Columbia University,                    
 * 500 W 120th St, Room 450, New York, NY, 10027.                         
 *                                                                        
 * Copyright (C) Blair MacIntyre 1995, Columbia University 1995           
 * 
 * Author          : Ruigang Yang
 * Created On      : Tue Feb 17 13:52:58 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Wed Feb 18 16:39:42 1998
 * Update Count    : 4
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Dyna.h,v $
 * $Date: 1998/02/19 21:00:45 $
 * $Author: ryang $
 * $Revision: 1.1 $
 * 
 * $Log: vrpn_Dyna.h,v $
 * Revision 1.1  1998/02/19 21:00:45  ryang
 * drivers for DynaSight
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

#ifndef INCLUDED_DYNA
#define INCLUDED_DYNA

#include "vrpn_Tracker.h"
// only 13 receivers allowed in normal addressing mode
#define MAX_SENSORS 13

#ifndef _WIN32

// This is a class which provides a server for an ascension 
// DynaSight.  The server will send out messages
// The timestamp is the time when the first character was read
// from the serial driver with "read".  No adjustment is currently
// made to this time stamp.

// If this is running on a non-linux system, then the serial port driver
// is probably adding more latency -- see the vrpn README for more info.

class vrpn_Tracker_Dyna: public vrpn_Tracker_Serial {
private:
  int reportLength;
  int totalReportLength;

 public:
  
  vrpn_Tracker_Dyna(char *name, vrpn_Connection *c, int cSensors=1,
		      char *port = "/dev/ttyd3", long baud = 38400);
    
  virtual ~vrpn_Tracker_Dyna();
  virtual void mainloop(void);
    
private:
  void my_flush() {
    // clear the input data buffer 
    unsigned char foo[128];
    while (read_available_characters(foo, 1) > 0) ;
  }
  int valid_report();
  int decode_record();
  int get_status();
 protected:

  virtual void get_report(void);
  virtual void reset();
  void printError(unsigned char uchErrCode, unsigned char uchExpandedErrCode);
  int checkError();
  int cResets;
  int cSensors;
};

#endif  // #ifndef _WIN32


#endif
