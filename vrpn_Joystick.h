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
 * Created On      : Tue Mar 17 15:58:37 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Tue Mar 24 21:03:10 1998
 * Update Count    : 12
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_Joystick.h,v $
 * $Date: 1998/03/25 02:15:25 $
 * $Author: ryang $
 * $Revision: 1.2 $
 * 
 * $Log: vrpn_Joystick.h,v $
 * Revision 1.2  1998/03/25 02:15:25  ryang
 * add button report function
 *
 * Revision 1.1  1998/03/18 23:12:23  ryang
 * new analog device and joystick channell
 * D
 *
 * SCCS Status     : %W%	%G%
 * static char rcsid[] = "$Id: vrpn_Joystick.h,v 1.2 1998/03/25 02:15:25 ryang Exp $";
 * HISTORY
 */

#ifndef VRPN_JOYSTICK
#define VRPN_JOYSTICK
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

class vrpn_Joystick :public vrpn_Serial_Analog, public vrpn_Button {
public:
  vrpn_Joystick(char * name, vrpn_Connection * c, char * portname,int
		baud, double);

  void mainloop();

protected:
  void get_report();
  void reset();
  void parse(int);
private:
  double resetval[vrpn_CHANNEL_MAX];
  long MAX_TIME_INTERVAL;
};


#endif
