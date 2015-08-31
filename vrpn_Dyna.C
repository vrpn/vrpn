/* Modification to make it work with the SeerReal D4D Headtracker,
   needs to send "0" instead of "V" to set it to continuous mode.
   Untested if this still works with the Dynasight Tracker.*/
#ifdef	_WIN32
#include <io.h>
#else
#endif
#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for strlen

#include "vrpn_Dyna.h"
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval, etc
#include "vrpn_Types.h"                 // for vrpn_float64

class VRPN_API vrpn_Connection;

#define T_ERROR (-1)
#define T_OK 	(0)

/* return codes for status check    */
#define T_PDYN_SPEW_MODE   (-50)
#define T_PDYN_NO_DATA 	(-51)

#define lOOO_OOOO (0x80)
#define llll_OOOO (0xf0)
#define OOOO_OOll (0x03)
#define OOOO_llOO (0x0c)

vrpn_Tracker_Dyna::vrpn_Tracker_Dyna(
		      char *name, vrpn_Connection *c, int cSensors,
		      const char *port, long baud ) :
vrpn_Tracker_Serial(name,c,port,baud), cResets(0), cSensors(cSensors)
{
    if (cSensors>VRPN_DYNA_MAX_SENSORS) {
      fprintf(stderr, "\nvrpn_Tracker_Dyna: too many sensors requested ... only %d allowed (%d specified)", VRPN_DYNA_MAX_SENSORS, cSensors );
      cSensors = VRPN_DYNA_MAX_SENSORS;
    }
    fprintf(stderr, "\nvrpn_Tracker_Dyna: starting up ...");
    d_sensor = 0 ; // sensor id is always 0 (first sensor is 0);
}

vrpn_Tracker_Dyna::~vrpn_Tracker_Dyna() {
  fprintf(stderr, "vrpn_Tracker_Dyna:Shutting down...\n");
}


int vrpn_Tracker_Dyna::get_status()
{
    int	    	bytesRead;
    unsigned char    	statusBuffer[256];

    my_flush();

    /* send request for status record   */

    vrpn_write_characters(serial_fd,(const unsigned char *) "\021", 1);
    vrpn_drain_output_buffer(serial_fd);
    vrpn_SleepMsecs(1000.0*2);

    /* do non-blocking read of status record    */
    bytesRead = vrpn_read_available_characters(serial_fd, statusBuffer, 8);
    // T_PDYN_STATUS_RECORD_LENGTH =8;

    if ( bytesRead == 8 )
    {
       /* we have correct length-  check a few chars to make sure this is a valid 
	* record
	*/
       if ( ((statusBuffer[0] & lOOO_OOOO) != lOOO_OOOO) ||
	    ((statusBuffer[1] & lOOO_OOOO) != lOOO_OOOO) )
	 return(T_ERROR);       

       /* otherwise, all is well   */
       return(T_OK);
    }

    /* if we get here, we either got too much data or not enough	*/

    /* no data means it's probably disconnected or not turned on	*/
    if ( bytesRead == 0 )
    {
//       fprintf(stderr, "No data\n");
      return(T_PDYN_NO_DATA);
    }

    /* if we got too much data, chances are that it's in continuous mode	*/
    if ( bytesRead > 8)
    {
       fprintf(stderr, "3\n");
      return(T_PDYN_SPEW_MODE);
    }

    /* if we get here, i have no idea what's going on-  could be garbage on the
     *  serial line, wrong baud rate, or that the Dynasight is flaking out.
     */
    return(T_ERROR);

}	/* t_pdyn_get_status */

#define MAX_TRIAL 10

void vrpn_Tracker_Dyna::reset() {
    //static int numResets = 0;	// How many resets have we tried?;
  static const char T_PDYN_C_CTL_C[4] ="\003\003\003";
  static const int T_PDYN_RECORD_LENGTH = 8;

  vrpn_write_characters(serial_fd, (const unsigned char*)T_PDYN_C_CTL_C, strlen(T_PDYN_C_CTL_C));
  vrpn_write_characters(serial_fd,(const unsigned char *) "4", 1); // set to polling mode;
      
  /* pause 1 second to allow the Dynasight buffer to stabilize	*/
  vrpn_SleepMsecs(1000.0*1);

  status = get_status();
      
  if ( status != T_OK ) {

    /* if no data, tracker probably not connected.  just bag it.    */
    if ( status == T_PDYN_NO_DATA )
    {
      fprintf(stderr, "vrpn_Tracker_Dyna::reset(): no data (is tracker turned on?)\n"); 
      status = vrpn_TRACKER_RESETTING;
      return;

    } 
    
  }else {
    fprintf(stderr, "vrpn_Tracker_Dyna: return valid status report\n");
    reportLength = T_PDYN_RECORD_LENGTH;
    
    // set it to continues mode;
    /* clear any leftover data	*/
   my_flush();

   /* set the Dynasight to continuous mode    */
   vrpn_write_characters(serial_fd, (const unsigned char*)T_PDYN_C_CTL_C, strlen(T_PDYN_C_CTL_C));
   //vrpn_write_characters(serial_fd, (const unsigned char *)"V", 1);
   vrpn_write_characters(serial_fd, (const unsigned char *)"0", 1);
   //T_PDYN_C_CONTINUOUS = "V"
   vrpn_SleepMsecs(1000.0*1);
   //vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now;
   timestamp.tv_sec = -1;
   status = vrpn_TRACKER_SYNCING;	// We are trying for a new reading;
   return;
  }
				     
}

int vrpn_Tracker_Dyna::get_report(void) {
  int ret;
  if (status == vrpn_TRACKER_SYNCING) {
    if ((ret=vrpn_read_available_characters(serial_fd, buffer, 1)) !=  1 || 
	(buffer[0] & llll_OOOO) != lOOO_OOOO) {
      return 0;
    }
    vrpn_gettimeofday(&timestamp, NULL);
    status = vrpn_TRACKER_PARTIAL;
    bufcount= ret;
  }
  if (status == vrpn_TRACKER_PARTIAL) {
    ret=vrpn_read_available_characters(serial_fd, &(buffer[bufcount]),
		reportLength-bufcount);
    if (ret < 0) {
      fprintf(stderr,"%s@%d: Error reading\n", __FILE__, __LINE__);
      status = vrpn_TRACKER_FAIL;
      return 0;
    }
    bufcount += ret;
    if (bufcount < reportLength) {	// Not done -- go back for more
      return 0;
    }	
  }
  
  if (!valid_report()) {
    //fprintf(stderr,"no valid report");
    bufcount = 0;
    status = vrpn_TRACKER_SYNCING;
    return 0;
  }
  //else fprintf(stderr,"got valid report");
  decode_record();
  status = vrpn_TRACKER_SYNCING;
  bufcount=0;

  return 1;
}

int vrpn_Tracker_Dyna::valid_report() {

  /* Check to see that the first four bits of the *
    * first two bytes correspond to 1000.          */
  if ( ( (buffer[0] & llll_OOOO) != lOOO_OOOO) ||
        ( (buffer[1] & llll_OOOO) != lOOO_OOOO) )
    return (0);


   /* Make sure no other consecutive bytes have 1000 in the high nibble */
   for (unsigned i = 2; i <reportLength-1; i += 2)
   {
      if ( ( (buffer[i] & llll_OOOO) == lOOO_OOOO) &&
	   ( (buffer[i+1] & llll_OOOO) == lOOO_OOOO))
      {
	fprintf(stderr,
		"vrpn_Tracker_Dyna: found two more status bytes in the Dynasight's output list\n");
	return (0);
     }
   }

   /* Next, check that the Dynasight is tracking or in caution mode.  If  *
    * it is not, then we can't be sure of what data is being sent back,   *
    * copy the most recent tracked data into the buffer.                  */
   if ( ( (buffer[1] & OOOO_OOll) == 0 ) ||
        ( (buffer[1] & OOOO_OOll) == 1 ) )
   {
     return (1);
   }

   /* Also, for this passive Dynasight mode, we want to make sure that we *
    * only getting records from the single (00) sensor, not any active    *
    * sensors.                                                            */
   if ( (buffer[0] & OOOO_llOO) != 0)
   {
     fprintf(stderr, "trackerlib:  Invalid target number for passive Dynasight\n");
     return (0);
   }
   
   /* If we've made it this far, we have a target that is being tracked,   *
    * and it is the correct target.  So, copy this into the                *
    * lastTrackedData array.                                               
   for (i = 0; i < trackerPtr->reportLength; i++)
     lastTrackedData[i] = buffer[bufIndex + i];
     */
   return (1);
}


/*****************************************************************************
 *
   decode_record - decodes a continuous binary encoded record
    	    	    	    for one station.
 
    output:
    	decodedMatrix- represents the row_matrix to transform the receiver
	               coordinates to tracker coordinates

	- T_ERROR is returned if the record is munged, T_OK otherwise
 *
 *****************************************************************************/

int vrpn_Tracker_Dyna::decode_record()
//    q_vec_type   decoded_pos;
{
   unsigned	i;

   unsigned char exp;
   char	x_high, y_high, z_high;
   unsigned char x_low, y_low, z_low;
   long x, y, z;

   if ( (buffer[0] & lOOO_OOOO) == 0 )
   {
      fprintf(stderr, "bogus data to vrpn_Tracker_Dyna_decode_record:\n");
      for ( i = 0; i < reportLength; i++ )
    	fprintf(stderr, "%x ", buffer[i]);
      fprintf(stderr, "\n");
      return(T_ERROR);
   }

   /* Pull out the exponent to shift by */
   //exp = ( (buffer[0] & OOOO_OOlO)<<1 ) | (buffer[0] & OOOO_OOOl);
   exp = (unsigned char)( ( (buffer[0] & 0x2)<<1 ) | (buffer[0] & 0x1) );

   x_high = (char)buffer[2];
   x_low  = (char)buffer[3];
   y_high = (char)buffer[4];
   y_low  = (char)buffer[5];
   z_high = (char)buffer[6];
   z_low  = (char)buffer[7];

   /* Shift high order byte, combine with low order, and then *
    * shift by the exponent.                                  */
   x = (long)( ((short)x_high<<8) | ((short)x_low) ) << exp;
   y = (long)( ((short)y_high<<8) | ((short)y_low) ) << exp;
   z = (long)( ((short)z_high<<8) | ((short)z_low) ) << exp;
   //fprintf(stderr,"x,y,z: %i,%i,%i\n",x,y,z);

   /* Convert to meters -- 1 unit = 0.05mm = 0.00005m */
   pos[0] = (double)x * 0.00005;
   pos[1] = (double)y * 0.00005;
   pos[2] = (double)z * 0.00005;
   
   /* For the single-target Dynasight, we assume that the orientation *
    * the target is that same as that of the Dynasight.               */
   d_quat[0] = 0.0;
   d_quat[1] = 0.0;
   d_quat[2] = 0.0;
   d_quat[3] = 1.0;

   return(T_OK);

}	/* t_pdyn_decode_record */
