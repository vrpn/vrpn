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
 * Created On      : Thu Jan 15 17:27:52 1998
 * Last Modified By: Ruigang Yang
 * Last Modified On: Sun Feb  8 17:17:27 1998
 * Update Count    : 45
 * 
 * $Source: /afs/unc/proj/stm/src/CVS_repository/vrpn/vrpn_Tracker_Fastrak.h,v $
 * $Date: 1998/09/11 19:47:24 $
 * $Author: hudson $
 * $Revision: 1.8 $
 * 
 * $Log: vrpn_Tracker_Fastrak.h,v $
 * Revision 1.8  1998/09/11 19:47:24  hudson
 * Version 4.02:  added filtering of log files to vrpn_Connection
 *
 * Revision 1.7  1998/06/05 19:30:48  taylorr
 * Slightly cleaner Fastrak driver.  It should work on SGIs as well as Linux.
 *
 * Revision 1.6  1998/06/01 20:12:12  kumsu
 * changed to ANSI to compile with aCC for hp
 *
 * Revision 1.5  1998/05/05 21:09:59  taylorr
 * This version works better with the aCC compiler on PixelFlow.
 *
 * Revision 1.4  1998/02/20 20:27:01  hudson
 * Version 02.10:
 *   Makefile:  split library into server & client versions
 *   Connection:  added sender "VRPN Control", "VRPN Connection Got/Dropped"
 *     messages, vrpn_ANY_TYPE;  set vrpn_MAX_TYPES to 2000.  Bugfix for sender
 *     and type names.
 *   Tracker:  added Ruigang Yang's vrpn_Tracker_Canned
 *
 * Revision 1.3  1998/02/11 20:35:41  ryang
 * canned class
 *
 * Revision 1.2  1998/01/24 19:12:14  ryang
 * read one report at a time, no matter how many sensors are on.
 *
 * Revision 1.1  1998/01/22 21:08:52  ryang
 * add vrpn_Tracker_Fastrak.C to data base
 *
 * SCCS Status     : %W%	%G%
 * 
 * HISTORY
 */

#ifndef INCLUDED_FASTRAK
#define INCLUDED_FASTRAK

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Tracker.h"

#ifndef VRPN_CLIENT_ONLY
//#ifndef _WIN32
#if defined(sgi) || defined(linux)


/*
 * endian-dependent routines
 *
 *  the current fastrak family is little-endian;  therefore, byte-reversing is
 *   necessary when it is connected to a machine (such as a sun2/3/4)
 *   that is big-endian.
 *
 * to see which one your machine is, do:
 *
 *  echo -n ab | od -x
 *
 * if the output is 6261, it's a little-endian machine;  if it's 6162, it's
 *	big-endian.
 *
 * (check the typedefs section for other machine-dependent definitions)
 */

/* return codes for status check    */
#define T_F_SPEW_MODE   (-70)
#define T_F_NO_DATA 	(-71)

/* how long to wait after software reset */
#define T_F_RESET_WAIT	    	9

/* current maximum as set by Polhemus	*/
#define T_F_MAX_NUM_STATIONS	4

/* byte offsets into Fastrak data record -- binary, non-continuous. */ 
/* report buffer constants  */
/* length of status bytes section of regular record */
#define T_F_STATUS_LENGTH   	    3

/* this is the string that precedes all reports;  '#' is replaced by the
 *  station number
 */
#define T_F_STATUS_BYTES    	    "0# "

/* length of system flags section of a status record	*/
#define T_F_SYSTEM_FLAGS_LENGTH	    3

/* length of status record   */
#define T_F_STATUS_RECORD_LENGTH    55
#define T_F_STATIONS_RECORD_LENGTH  13

#define T_F_DATA_RECORD	    	'0'
#define T_F_KEYPAD_RECORD	'1'
#define T_F_RETRIEVE_RECORD	'2'


/* this char is in the position T_F_RECORD_SUBTYPE in case of error */
#define T_F_ERROR_SUBTYPE       '*'
#define T_F_STATUS_SUBTYPE  	'S'
#define T_F_OUTPUT_LIST_SUBTYPE 'O'
#define T_F_STATIONS_SUBTYPE  	'l'

/* status bytes-  same for ascii and binary */
#define	T_F_RECORD_TYPE	     	     0
#define T_F_STATION_NUM		     1
#define T_F_RECORD_SUBTYPE	     2

/* the fastrak has 3 bytes in a status record that give its current state.
 *  after a reset of the fastrak, these should be equal to 
 *  T_F_DEFAULT_SYS_FLAGS.  note that this is the fastrak's default state 
 *  after reset, not the default state as set by trackerlib.   see
 *  the STATUS RECORD FORMAT section (page G-11) in the fastrak manual
 *  for details on the status record format.
 */
#define T_F_DEFAULT_SYS_FLAGS	    "208"

#define T_F_WORD_SIZE	    	sizeof(t_f_data_word)
#define T_F_FULL_WORD_SIZE  	sizeof(float)

/* binary BYTE offsets into fastrak data records   */
#define T_F_BIN_X_OFFSET	     0
#define T_F_BIN_Y_OFFSET	     (T_F_BIN_X_OFFSET+T_F_WORD_SIZE)
#define T_F_BIN_Z_OFFSET	     (T_F_BIN_Y_OFFSET+T_F_WORD_SIZE)

#ifdef SKIP
#define T_F_BIN_X_OFFSET	     T_F_STATUS_LENGTH
#define T_F_BIN_Y_OFFSET	     (T_F_BIN_X_OFFSET+T_F_WORD_SIZE)
#define T_F_BIN_Z_OFFSET	     (T_F_BIN_Y_OFFSET+T_F_WORD_SIZE)
#endif


#define T_F_BIN_MATRIX_OFFSET	     (T_F_BIN_Z_OFFSET+T_F_WORD_SIZE)

/* quaternion offsets:  assumes position x, y, z come first, then w,x,y,z 
 *   (aka s, v0, v1, vT_F_WORD_SIZE) for the quaternion
 */
#define T_F_BIN_QW_OFFSET	     (T_F_BIN_Z_OFFSET+T_F_WORD_SIZE)
#define T_F_BIN_QX_OFFSET	     (T_F_BIN_QW_OFFSET+T_F_WORD_SIZE)
#define T_F_BIN_QY_OFFSET	     (T_F_BIN_QX_OFFSET+T_F_WORD_SIZE)
#define T_F_BIN_QZ_OFFSET	     (T_F_BIN_QY_OFFSET+T_F_WORD_SIZE)

/* unit/station status   */
#define T_F_ON	    	    1
#define T_F_OFF	    	    0

/* mode constants;  the command strings to set these modes are below   */
#define T_F_M_POLLING 	    	80
#define T_F_M_CONTINUOUS    	81

#define T_F_M_DEFAULT_MODE	T_F_M_CONTINUOUS

/* ascii or binary mode;  trackerlib only properly supports binary  */
#define T_F_M_BINARY	    	84
#define T_F_M_ASCII 	    	85
#define T_F_M_DEFAULT_BIN_ASCII	T_F_M_BINARY

/* report in inches or cm;  trackerlib only properly supports meters. 
 *  there are no units in binary mode-  reported values are scaled to be
 *   -32,768 < x < 32768.  trackerlib scales these numbers using the
 *  constants defined below.
 *
 *  these are included here for other applications that might run
 *  in ascii mode (also used by t_f_length_units to set fastrak to default
 *  state, which uses inches)
 */
#define T_F_M_INCHES	    	90
#define T_F_M_CENTIMETERS    	91

/* the largest number of data items we ever expect to see in a single report */
#define T_F_MAX_DATA_ITEMS  	    50

#define T_F_MAX_CMD_STRING_LENGTH   80

/* this is the string returned by the fastrak when queried for the current
 *  output list when in xyz quat mode, as set by the command T_F_C_XYZ_QUAT.
 *  status bytes are not included, but should be "2xO", where x = station num.
 *  The first 2 bytes = " 2" for x,y,z;  the 2nd = "11" for quat.
 */
#define T_F_XYZ_QUAT_OUTPUT_LIST    " 211"


/* fastrak commands: command strings are prefixed with T_F_C_	*/
/* NOTE: use double quotes here, even for single-character commands, since
 *  they are treated as strings.
 */
#define T_F_C_GET_STATUS    	"S"
#define T_F_C_RESET    	    	"\031"

/* this is actually an sprintf control string for t_f_set_hemisphere;
 * the '%d's are replaced by the station number, then the axis info, including
 *  sign if nec.  examples:  H1,1,0,0\r sets to default of +X axis;
 *  	    	    	     H2,0,-1,0\r sets to -Y axis
 * if this changes, check t_f_set_hemisphere() as well
 */
#define T_F_C_SET_HEMISPHERE    "H%d,%d,%d,%d\r"

#define T_F_C_RETRIEVE_STATIONS	    "l1\r"

/* '#' is replaced by station number	*/
#define T_F_C_RETRIEVE_OUTPUT_LIST  "O#\r"

/* poll command-  initiates data record when in poll mode   */
#define T_F_C_POLL    	    	"P"

/* sets fastrak to report x,y,z and azimuth, elevation, and roll angles    */
#define T_F_C_XYZ_ANGLES	"O2,4\r"

/* x,y, z and 4 quat elements
 * 2 = xyz, 11 = quat;  '#' is replaced by station number
 *  t_f_set_data_format() depends on the format of this
 */
#define T_F_C_XYZ_QUAT 	    	"O#,2,11\r"

#define T_F_C_MATRIX	    	"O#,2,5,6,7,1\r"

/* puts fastrak in binary/ascii mode	*/
#define T_F_C_BINARY  	    	"f"
#define T_F_C_ASCII   	    	"F"

/* non-continuous mode (must poll to get report)/cont mode    */
#define T_F_C_POLLING	  	"c"
#define T_F_C_CONTINUOUS	"C"

/* set units to centimeters/inches  */
#define T_F_C_CENTIMETERS	"u"
#define T_F_C_INCHES  	    	"U"

/* enable station '#'-  the char '#' is replaced by a number from 1-4; the
 *  % field is replaced with a 0 for turning it off and a 1 to turn it on
 */
#define T_F_C_STATION_TOGGLE  "l#,%\r"

/* 
 * fastrak port parameters
 */
/* set for communication at 19200 baud 	*/
#define T_F_BAUD_RATE	T_DEFAULT_BAUD_RATE


/* used for scaling binary position values, since they run from 0-> +/-32767 */
#define T_F_METERS_PER_INCH	(0.0254)
#define T_F_INCH_XYZ_RANGE    	(65.48)
#define T_F_CM_XYZ_RANGE    	(T_F_INCH_XYZ_RANGE * 2.54)
#define T_F_METER_XYZ_RANGE    	(T_F_CM_XYZ_RANGE / 100.0)
#define T_F_XYZ_RANGE    	T_F_METER_XYZ_RANGE

/* this value should be changed if we ever get more than 2 bytes for binary
 *  data values
 */
#define T_F_DATA_MAX_RANGE  	(32768.0)

/* matrix values are in the range -1 < x < 1	*/
#define T_F_MATRIX_SCALE_FACTOR	(1/T_F_DATA_MAX_RANGE)

/* scaling factors for various units-  currently only use meters    */
#define T_F_XYZ_INCH_SCALE    	(T_F_INCH_XYZ_RANGE / T_F_DATA_MAX_RANGE)
#define T_F_XYZ_CM_SCALE      	(T_F_CM_XYZ_RANGE / T_F_DATA_MAX_RANGE)
#define T_F_XYZ_METER_SCALE     (T_F_METER_XYZ_RANGE / T_F_DATA_MAX_RANGE)
#define T_F_XYZ_SCALE	    	T_F_XYZ_METER_SCALE

/* report lengths (for binary mode only):  lengths
 * come from the Fastrak manual
 */
/* x, y and z + 3x3 matrix : 3 2-byte ints + 9 2-byte ints + status bytes */
#define T_F_XYZ_MATRIX_LENGTH   (T_F_STATUS_LENGTH+24)

/* 3 4-byte coordinates + 4 4-byte quat elements + status bytes	*/
#define T_F_XYZ_QUAT_LENGTH 	(T_F_STATUS_LENGTH+28)

/* return codes	*/
#define T_OK	    	    	0
#define T_ERROR     	    	(-1)
#define T_NULL_FUNC_PTR	    	(-2)
#define T_UNRECOGNIZED_COMMAND	(-3)
/* i/o function called does not support this tracker's comm. interface    */
#define T_UNSUPPORTED_DEVICE	(-4)
/* t_add_unit called on already added unit, or t_remove_unit on removed */
#define T_DUPL_UNIT             (-5)

/* report format options    */
#define T_XYZ_QUAT  	    	40
#define T_PPHIGS_MATRIX	    	(T_XYZ_QUAT+1)
#define T_XFORM_MATRIX	    	(T_PPHIGS_MATRIX+1)
#define T_DEFAULT_DATA_FORMAT	(T_PPHIGS_MATRIX)

/* size of buffer on host for reading serial data.  note that on some
 *  systems (like the Vax) this is the real size of the read buffer;  on
 *  others (like the SGI), there is apparently no real buffer size, so
 *  we just deal in chunks of this size and make it look as if there
 *  is a read buffer this size.  This constant should not be changed unless
 *  the real read buffer size on your machine is smaller than this, in
 *  which case things might work & they might not.
 */
#define T_READ_BUFFER_SIZE	    256

/* extra big buffer for various operations  */
#define T_MAX_READ_BUFFER_SIZE      (2*T_READ_BUFFER_SIZE)

/*****************************************************************************
 *
    filtering definitions-  see manual for interpretation
 *
 *****************************************************************************/

#define T_F_C_KILL_POSITION_FILTER  	"x0,1,1,1\r"
#define T_F_C_KILL_ORIENTATION_FILTER  	"v0,1,1,1\r"

/* got these values from polhemus, but they may change when we switch from
 *  the beta unit to the production unit
 */
#define T_F_C_FULL_POSITION_FILTER  	"x0.2,0.2,0.8,0.8\r"
#define T_F_C_FULL_ORIENTATION_FILTER  	"v0.2,0.2,0.8,0.8\r"

#define T_F_FULL_FILTER 

#ifdef T_F_FULL_FILTER
#   define T_F_C_SET_POSITION_FILTER   	T_F_C_FULL_POSITION_FILTER
#   define T_F_C_SET_ORIENTATION_FILTER	T_F_C_FULL_ORIENTATION_FILTER
#else
#   define T_F_C_SET_POSITION_FILTER   	T_F_C_KILL_POSITION_FILTER
#   define T_F_C_SET_ORIENTATION_FILTER	T_F_C_KILL_ORIENTATION_FILTER
#endif /* T_F_NO_FILTER */


#define T_FALSE		0
#define T_TRUE 	1

#define Q_X 	0
#define Q_Y 	1
#define Q_Z 	2
#define Q_W 	3

class vrpn_Tracker_Fastrak: public vrpn_Tracker_Serial {
  int foo[10];
  int reportLength;
  int totalReportLength;
  int numUnits;
  int stationArray[T_F_MAX_NUM_STATIONS];
  int dataFormat;
  int mode;
  float f_pos[T_F_MAX_NUM_STATIONS][3];
  float f_quat[T_F_MAX_NUM_STATIONS][4];
  
private:
  void my_flush() {
    // clear the input data buffer 
    unsigned char foo[128];
    //fprintf(stderr, "flush...");
    while (read_available_characters(foo, 1) > 0) {
      //fprintf(stderr, "X");
    } ;
    //fprintf(stderr, "\n");
  }
  int set_all_units();
  int set_unit(int, int);
  int get_units(int stationVector[T_F_MAX_NUM_STATIONS]);
  int verify_unit_status(int whichUnit, int unitOn);

  int poll_mode();
  int get_status();
  void cont_mode();
  void ms_sleep(int s); // sleep   s ms ;
  int bin_ascii(int);
  int set_total_report_length();
  int filter();

  int enable(int);
  int disable(int);
  int set_data_format(int);
  int verify_output_list(int);
  int get_output_list(int, char *);
  void replace_station_char(char *, int, int);
  void set_hemisphere(int,int,int);
  int xyz_quat_interpret();
  int checkSubType(int, int);
  int valid_report();

  void printBuffer();
  void printChar(char *, int);
 public:
  
  vrpn_Tracker_Fastrak(char *name, vrpn_Connection *c, 
		       char *port = "/dev/ttyS1", long baud = 9600) :
    vrpn_Tracker_Serial(name,c,port,baud) {};
    
  virtual void mainloop(void);
  
  //int get_status(void) { return status;}
    
 protected:

    virtual void get_report(void);

    virtual void reset();

};

#endif  // defined(sgi) || defined(linux)
//#endif  // #ifndef _WIN32
#endif  // VRPN_CLIENT_ONLY
#endif  // INCLUDED_FASTRAK
