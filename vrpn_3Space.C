#include <ctype.h>                      // for isprint
#include <math.h>                       // for sqrt
#include <stdio.h>                      // for fprintf, stderr, perror, etc

#include "quat.h"                       // for Q_W, Q_X, Q_Y, Q_Z
#include "vrpn_3Space.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, etc
#include "vrpn_Tracker.h"               // for vrpn_TRACKER_FAIL, etc
#include "vrpn_Types.h"                 // for vrpn_int16, vrpn_float64

// This constant turns the tracker binary values in the range -32768 to
// 32768 to meters.
#define T_3_DATA_MAX            (32768.0)
#define T_3_INCH_RANGE          (65.48)
#define T_3_CM_RANGE            (T_3_INCH_RANGE * 2.54)
#define T_3_METER_RANGE         (T_3_CM_RANGE / 100.0)
#define T_3_BINARY_TO_METERS    (T_3_METER_RANGE / T_3_DATA_MAX)

void vrpn_Tracker_3Space::reset()
{
   int i,resetLen,ret;
   unsigned char reset[10];

   // Send the tracker a string that should reset it.  The first time we
   // try this, just do the normal ^Y reset.  Later, try to reset
   // to the factory defaults.  Then toggle the extended mode.
   // Then put in a carriage return to try and break it out of
   // a query mode if it is in one.  These additions are cumulative: by the
   // end, we're doing them all.
   resetLen = 0;
   d_numResets++;		  	// We're trying another reset
   if (d_numResets > 1) {	// Try to get it out of a query loop if its in one
   	reset[resetLen++] = (char) (13); // Return key -> get ready
   }
   if (d_numResets > 7) {
	reset[resetLen++] = 'Y'; // Put tracker into tracking (not point) mode
   }
   if (d_numResets > 3) {	// Get a little more aggressive
   	if (d_numResets > 4) { // Even more aggressive
      	reset[resetLen++] = 't'; // Toggle extended mode (in case it is on)
   }
   reset[resetLen++] = 'W'; // Reset to factory defaults
   reset[resetLen++] = (char) (11); // Ctrl + k --> Burn settings into EPROM
   }
   reset[resetLen++] = (char) (25); // Ctrl + Y -> reset the tracker
   send_text_message("Resetting", timestamp, vrpn_TEXT_ERROR, d_numResets);
   for (i = 0; i < resetLen; i++) {
	if (vrpn_write_characters(serial_fd, &reset[i], 1) == 1) {
		vrpn_SleepMsecs(1000*2);  // Wait 2 seconds each character
   	} else {
		send_text_message("Failed writing to tracker", timestamp, vrpn_TEXT_ERROR, d_numResets);
		perror("3Space: Failed writing to tracker");
		status = vrpn_TRACKER_FAIL;
		return;
	}
   }
   vrpn_SleepMsecs(1000.0*10);	// Sleep to let the reset happen

   // Get rid of the characters left over from before the reset
   vrpn_flush_input_buffer(serial_fd);

   // Make sure that the tracker has stopped sending characters
   vrpn_SleepMsecs(1000.0*2);
   unsigned char scrap[80];
   if ( (ret = vrpn_read_available_characters(serial_fd, scrap, 80)) != 0) {
     fprintf(stderr,"  3Space warning: got >=%d characters after reset:\n",ret);
     for (i = 0; i < ret; i++) {
      	if (isprint(scrap[i])) {
         	fprintf(stderr,"%c",scrap[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",scrap[i]);
         }
     }
     fprintf(stderr, "\n");
     vrpn_flush_input_buffer(serial_fd);		// Flush what's left
   }

   // Asking for tracker status
   if (vrpn_write_characters(serial_fd, (const unsigned char *) "S", 1) == 1) {
      vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
   } else {
	perror("  3Space write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   }

   // Read Status
   unsigned char statusmsg[56];
   if ( (ret = vrpn_read_available_characters(serial_fd, statusmsg, 55)) != 55){
  	fprintf(stderr, "  Got %d of 55 characters for status\n",ret);
   }
   if ( (statusmsg[0]!='2') || (statusmsg[54]!=(char)(10)) ) {
     int i;
     statusmsg[55] = '\0';	// Null-terminate the string
     fprintf(stderr, "  Tracker: status is (");
     for (i = 0; i < 55; i++) {
      	if (isprint(statusmsg[i])) {
         	fprintf(stderr,"%c",statusmsg[i]);
         } else {
         	fprintf(stderr,"[0x%02X]",statusmsg[i]);
         }
     }
     fprintf(stderr, ")\n  Bad status report from tracker, retrying reset\n");
     return;
   } else {
     send_text_message("Got status (tracker back up)!", timestamp, vrpn_TEXT_ERROR, 0);
     d_numResets = 0; 	// Success, use simple reset next time
   }

   // Set output format to be position,quaternion
   // These are a capitol 'o' followed by comma-separated values that
   // indicate data sets according to appendix F of the 3Space manual,
   // then followed by character 13 (octal 15).
   if (vrpn_write_characters(serial_fd, (const unsigned char *)"O2,11\015", 6) == 6) {
	vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
   } else {
	perror("  3Space write failed");
	status = vrpn_TRACKER_FAIL;
	return;
   }

   // Set data format to BINARY mode
   vrpn_write_characters(serial_fd, (const unsigned char *)"f", 1);

   // Set tracker to continuous mode
   if (vrpn_write_characters(serial_fd,(const unsigned char *) "C", 1) != 1)
   	perror("  3Space write failed");
   else {
   	fprintf(stderr, "  3Space set to continuous mode\n");
   }

   fprintf(stderr, "  (at the end of 3Space reset routine)\n");
   vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
   status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}


int vrpn_Tracker_3Space::get_report(void)
{
   int ret;

   // The reports are each 20 characters long, and each start with a
   // byte that has the high bit set and no other bytes have the high
   // bit set.  If we're synching, read a byte at a time until we find
   // one with the high bit set.
   if (status == vrpn_TRACKER_SYNCING) {
      // Try to get a character.  If none, just return.
      if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) {
      	return 0;
      }

      // If the high bit isn't set, we don't want it we
      // need to look at the next one, so just return
      if ( (buffer[0] & 0x80) == 0) {
      	send_text_message("Syncing (high bit not set)", timestamp, vrpn_TEXT_WARNING);
      	return 0;
      }

      // Got the first character of a report -- go into PARTIAL mode
      // and say that we got one character at this time.
      bufcount = 1;
      vrpn_gettimeofday(&timestamp, NULL);
      status = vrpn_TRACKER_PARTIAL;
   }

   // Read as many bytes of this 20 as we can, storing them
   // in the buffer.  We keep track of how many have been read so far
   // and only try to read the rest.  The routine that calls this one
   // makes sure we get a full reading often enough (ie, it is responsible
   // for doing the watchdog timing to make sure the tracker hasn't simply
   // stopped sending characters).
   ret = vrpn_read_available_characters(serial_fd, &buffer[bufcount],
		20-bufcount);
   if (ret == -1) {
	send_text_message("Error reading, resetting", timestamp, vrpn_TEXT_ERROR);
	status = vrpn_TRACKER_FAIL;
	return 0;
   }
   bufcount += ret;
   if (bufcount < 20) {	// Not done -- go back for more
	return 0;
   }

   { // Decode the report
	unsigned char decode[17];
	int i;

	const unsigned char mask[8] = {0x01, 0x02, 0x04, 0x08,
					0x10, 0x20, 0x40, 0x80 };
	// Clear the MSB in the first byte
	buffer[0] &= 0x7F;

	// Decode the 3Space binary representation into standard
	// 8-bit bytes.  This is done according to page 4-4 of the
	// 3Space user's manual, which says that the high-order bits
	// of each group of 7 bytes is packed into the 8th byte of the
	// group.  Decoding involves setting those bits in the bytes
	// iff their encoded counterpart is set and then skipping the
	// byte that holds the encoded bits.
	// We decode from buffer[] into decode[] (which is 3 bytes
	// shorter due to the removal of the bit-encoding bytes).

	// decoding from buffer[0-6] into decode[0-6]
	for (i=0; i<7; i++) {
		decode[i] = buffer[i];
		if ( (buffer[7] & mask[i]) != 0) {
			decode[i] |= (unsigned char)(0x80);
		}
	}

	// decoding from buffer[8-14] into decode[7-13]
	for (i=7; i<14; i++) {
		decode[i] = buffer[i+1];
		if ( (buffer[15] & mask[i-7]) != 0) {
			decode[i] |= (unsigned char)(0x80);
		}
	}

	// decoding from buffer[16-18] into decode[14-16]
	for (i=14; i<17; i++) {
		decode[i] = buffer[i+2];
		if ( (buffer[19] & mask[i-14]) != 0) {
			decode[i] |= (unsigned char)(0x80);
		}
	}

	// Parse out sensor number, which is the second byte and is
	// stored as the ASCII number of the sensor, with numbers
	// starting from '1'.  We turn it into a zero-based unit number.
	d_sensor = decode[1] - '1';

	// Position
	unsigned char * unbufPtr = &decode[3];
	pos[0] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr) * T_3_BINARY_TO_METERS;
	pos[1] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr) * T_3_BINARY_TO_METERS;
	pos[2] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr) * T_3_BINARY_TO_METERS;

	// Quarternion orientation.  The 3Space gives quaternions
	// as w,x,y,z while the VR code handles them as x,y,z,w,
	// so we need to switch the order when decoding.  Also the
	// tracker does not normalize the quaternions.
	d_quat[Q_W] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr);
	d_quat[Q_X] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr);
	d_quat[Q_Y] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr);
	d_quat[Q_Z] = vrpn_unbuffer_from_little_endian<vrpn_int16>(unbufPtr);

	//Normalize quaternion
	double norm = sqrt (  d_quat[0]*d_quat[0] + d_quat[1]*d_quat[1]
			    + d_quat[2]*d_quat[2] + d_quat[3]*d_quat[3]);
	for (i=0; i<4; i++) {
		d_quat[i] /= norm;
	}

	// Done with the decoding, set the report to ready
	// Ready for another report
	status = vrpn_TRACKER_SYNCING;
      	bufcount = 0;
   }

   return 1;	// Got a report.
#ifdef VERBOSE
      print_latest_report();
#endif
}
