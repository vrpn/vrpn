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
 * Created On      : Sun Feb  8 16:28:25 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Sun Feb  8 17:33:38 1998
 * Update Count    : 12
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_canned.h,v $
 * $Date: 1998/02/11 20:35:42 $
 * $Author: ryang $
 * $Revision: 1.1 $
 * 
 * $Log: vrpn_canned.h,v $
 * Revision 1.1  1998/02/11 20:35:42  ryang
 * canned class
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */


#ifndef CANNED_H
#define CANNED_H

// this is a virtual device that playback a pre-recorded sensor data

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"


// only 13 receivers allowed in normal addressing mode
#define MAX_SENSORS 13

class vrpn_Tracker_Canned: public vrpn_Tracker {
  
 public:
  
  vrpn_Tracker_Canned(char *name, vrpn_Connection *c, char * datafile);

    
  virtual ~vrpn_Tracker_Canned();
  virtual void mainloop(void);

private:
  void copy();

 protected:

  virtual void get_report(void) {/* do nothing */} ;
  virtual void reset();


  FILE * fp;
  int totalRecord;
  vrpn_TRACKERCB t;
  int current;

};

#endif





