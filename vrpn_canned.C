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
 * Created On      : Sun Feb  8 16:35:09 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Sun Feb  8 19:01:35 1998
 * Update Count    : 24
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/Attic/vrpn_canned.C,v $
 * $Date: 1998/02/11 20:35:42 $
 * $Author: ryang $
 * $Revision: 1.1 $
 * 
 * $Log: vrpn_canned.C,v $
 * Revision 1.1  1998/02/11 20:35:42  ryang
 * canned class
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

static char rcsid[] = "$Id: vrpn_canned.C,v 1.1 1998/02/11 20:35:42 ryang Exp $";


#include "vrpn_canned.h"


#define COPY(s1, s2) memcpy(&(s1), &(s2), sizeof(struct timeval));

vrpn_Tracker_Canned::vrpn_Tracker_Canned(char *name, vrpn_Connection
					 *c, char * datafile) 
  : vrpn_Tracker(name, c) {
    fp =fopen(datafile,"r");
    if (fp == NULL) {
      perror("can't open datafile:");
      return;
    } else {
      fread(&totalRecord, sizeof(int), 1, fp);
      fread(&t, sizeof(vrpn_TRACKERCB), 1, fp);
      printf("pos[0]=%.4f, time = %ld usec\n", t.pos[0],
	     t.msg_time.tv_usec);
      printf("sizeof trackcb = %d, total = %d\n", sizeof(t), totalRecord);
      current =1;
      copy();
    }
}


vrpn_Tracker_Canned::~vrpn_Tracker_Canned() {
  if (fp != NULL) fclose(fp);
}


void vrpn_Tracker_Canned::mainloop() {
  // Send the message on the connection;
  if (connection) {
    char	msgbuf[1000];
    int	len = encode_to(msgbuf);
    if (connection->pack_message(len, timestamp,
				 position_m_id, my_id, msgbuf,
				 vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,"Tracker: cannot write message: tossing\n");
    }
  } else {
    fprintf(stderr,"Tracker canned: No valid connection\n");
    return;
  }

  if (fread(&t, sizeof(vrpn_TRACKERCB), 1, fp) < 1) {
    reset();
    return;
  }
  struct timeval timeout;
  timeout.tv_sec =0;
  timeout.tv_usec = (t.msg_time.tv_sec * 1000000 + t.msg_time.tv_usec)
    - (timestamp.tv_sec *  1000000 + timestamp.tv_usec);
  if (timeout.tv_usec > 1000000) timeout.tv_usec = 0;
  
  //printf("pos[0]=%.4f, time = %ld usec\n", pos[0], timeout.tv_usec);
  select(0, 0, 0, 0, & timeout);  // wait for that long;
  current ++;
  copy();

}

void vrpn_Tracker_Canned::copy() {
  memcpy(&(timestamp), &(t.msg_time), sizeof(struct timeval));
  sensor = t.sensor;
  pos[0] = t.pos[0];  pos[1] = t.pos[1];  pos[2] = t.pos[2];
  quat[0] = t.quat[0];  quat[1] = t.quat[1];  
  quat[2] = t.quat[2];  quat[3] = t.quat[3];
}

void vrpn_Tracker_Canned::reset() {
  fprintf(stderr, "Resetting!");
  if (fp == NULL) return;
  fseek(fp, sizeof(int), SEEK_SET);
  fread(&t, sizeof(vrpn_TRACKERCB), 1, fp);
  copy();
  current = 1;
}





