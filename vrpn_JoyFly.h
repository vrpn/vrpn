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
 * Author   : Sang-Uok Kum
 * Modified : 4/17/98
 * Content  : Changed the funciton update(q_matrix_type)        
 *            from update(const q_matrix_type &)
 *
 * Copyright (C) Blair MacIntyre 1995, Columbia University 1995           
 * 
 * Author          : Ruigang Yang
 * Created On      : Tue Mar 31 17:16:25 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Tue Apr  7 13:31:54 1998
 * Update Count    : 20
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_JoyFly.h,v $
 * $Date: 1998/12/02 19:44:55 $
 * $Author: hudson $
 * $Revision: 1.5 $
 * 
 * $Log: vrpn_JoyFly.h,v $
 * Revision 1.5  1998/12/02 19:44:55  hudson
 * Converted JoyFly so it could run on the same server as the Joybox whose
 * data it is processing.  (Previously they had to run on different servers,
 * which could add significant latency to the tracker and make startup awkward.)
 * Added some header comments to vrpn_Serial and some consts to vrpn_Tracker.
 *
 * Revision 1.4  1998/06/01 20:12:12  kumsu
 * changed to ANSI to compile with aCC for hp
 *
 * Revision 1.3  1998/04/08 14:07:37  taylorr
 * Makes it compile on HPs
 *
 * Revision 1.2  1998/04/07 17:32:33  ryang
 * change to const XXX
 *
 * Revision 1.1  1998/04/07 16:55:12  taylorr
 * Implements more sound code.
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */


#ifndef INCLUDED_JOYFLY
#define INCLUDED_JOYFLY

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"
#include "vrpn_Analog.h"

#include <quat.h>
#ifndef _WIN32



class vrpn_Tracker_JoyFly : public vrpn_Tracker {

  private:
    double chanAccel [7];
    int chanPower [7];
    struct timeval prevtime;

    vrpn_Analog_Remote * joy_remote;
    q_matrix_type initMatrix, currentMatrix;


  

  public:
    vrpn_Tracker_JoyFly (const char * name, vrpn_Connection * c, 
		         const char * source, const char * config_file_name,
                         vrpn_Connection * sourceConnection = NULL);
    virtual ~vrpn_Tracker_JoyFly (void);

    virtual void mainloop (void);
    virtual void reset (void);

    void update (q_matrix_type &);

    static void handle_joystick (void *, const vrpn_ANALOGCB);
    static int handle_newConnection (void *, vrpn_HANDLERPARAM);
};
#endif

#endif















