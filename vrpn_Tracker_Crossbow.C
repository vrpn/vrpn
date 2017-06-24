// vrpn_Tracker_Crossbow.h
//	This file contains the implementation for a Crossbow RGA300CA Tracker.

#include <math.h>                       // for cos, sin
#include <stdio.h>                      // for fprintf, stderr, NULL
#include <stdlib.h>                     // for realloc, free
#include <string.h>                     // for memset

#include "quat.h"                       // for q_from_euler, Q_W, Q_X, Q_Y, etc
#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Serial.h"
#include "vrpn_Tracker_Crossbow.h"

// Conversion multiplier from Gs to meters-per-second-per-second (~9.8 m/s^2 on Earth)
#define MPSS_PER_G (9.80665)

vrpn_Tracker_Crossbow::vrpn_Tracker_Crossbow(const char *name, vrpn_Connection *c, const char *port, long baud, 
	float g_range, float ar_range)
  : vrpn_Tracker_Serial(name, c, port, baud)
  , lin_accel_range(g_range)
  , ang_accel_range(ar_range)
  , device_serial(0)
  , device_version(0)
  , just_read_something(0)
{
}

vrpn_Tracker_Crossbow::~vrpn_Tracker_Crossbow() {
	if (device_version) {
		free(device_version);
		device_version = 0;
	}
}

// Retrieves a raw_packet from an incoming byte array, and even flips endianness as necessary.
void vrpn_Tracker_Crossbow::unbuffer_packet(raw_packet &dest, unsigned char *buffer) {
	vrpn_unbuffer(&buffer, &dest.header);
	vrpn_unbuffer(&buffer, &dest.roll_angle);
	vrpn_unbuffer(&buffer, &dest.pitch_angle);
	vrpn_unbuffer(&buffer, &dest.yaw_rate);
	vrpn_unbuffer(&buffer, &dest.accel_x);
	vrpn_unbuffer(&buffer, &dest.accel_y);
	vrpn_unbuffer(&buffer, &dest.accel_z);
	vrpn_unbuffer(&buffer, &dest.timer);
	vrpn_unbuffer(&buffer, &dest.temp_voltage);
	vrpn_unbuffer(&buffer, &dest.part_number);
	vrpn_unbuffer(&buffer, &dest.status);
	vrpn_unbuffer(&buffer, &dest.checksum);
}

int vrpn_Tracker_Crossbow::validate_packet(const raw_packet &packet) {

	// Allow accessing the packet as a string of bytes
	union {
		raw_packet packet;
		vrpn_uint8 bytes[sizeof(raw_packet)];
	} aligned;
	aligned.packet = packet;

	// Check the header for the magic number
	if (packet.header != 0xAA55) {
		fprintf(stderr, "vrpn_Tracker_Crossbow: Received packet with invalid header $%02X%02X (should be $AA55)\n",
			aligned.bytes[0], aligned.bytes[1]);
		return 1;
	}

	// Now calculate the expected checksum
	vrpn_uint16 checksum = 0;
	for (int i = 2; i < 22; i++)
		checksum += aligned.bytes[i];

	// And compare the two checksum values.
	if (checksum != packet.checksum) {
		fprintf(stderr, "vrpn_Tracker_Crossbow: Received packet with invalid checksum $%04X (should be $%04X)\n",
			packet.checksum, checksum);
		return 1;
	}

	return 0;
}

int vrpn_Tracker_Crossbow::get_report() {
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000; // Half a second

	switch (status) {
		case vrpn_TRACKER_AWAITING_STATION:
			fprintf(stderr, "vrpn_Tracker_Crossbow: sanity: should never enter AWAITING_STATION state\n");
			return 0;
			
		case vrpn_TRACKER_SYNCING: {
			int rv;

			// Quit early if we just got a packet
			if (just_read_something) {
				just_read_something = 0;
				return 0;
			}

			// Request a packet from the device
			unsigned char echo = 'G';
			vrpn_write_characters(serial_fd, &echo, 1);

			rv = vrpn_read_available_characters(serial_fd, buffer, 1, &timeout);
			// Return early if no characters are available
			if (!rv)
				return 0;

			// Return early if we aren't at the start of a packet header
			if (*buffer != 0xAA) {
				return 0;
			}

			bufcount = 1;

			status = vrpn_TRACKER_PARTIAL;

			// Fall through to next state
		}

		case vrpn_TRACKER_PARTIAL: {
			int rv;

			if (bufcount == 1) {
				rv = vrpn_read_available_characters(serial_fd, buffer + 1, 1, &timeout);
				if (!rv || (buffer[1] != 0x55)) {
					buffer[0] = 0;
					bufcount = 0;
					status = vrpn_TRACKER_SYNCING;
					return 0;
				}
				bufcount++;
			}

			// Try to read the rest of the packet. Return early if we haven't got a full packet yet.
			rv = vrpn_read_available_characters(serial_fd, buffer + bufcount, 24 - bufcount, &timeout);
			bufcount += rv;
			if (bufcount < 24)
				return 0;

			raw_packet new_data;
			unbuffer_packet(new_data, &buffer[0]);

			// Ensure the packet is valid
			if (validate_packet(new_data)) {
				vrpn_flush_input_buffer(serial_fd);
				status = vrpn_TRACKER_SYNCING;
				return 0;
			}

			// Prepare the new report
			process_packet(new_data);

			bufcount = 0;
			memset(buffer, 0, 24);
			status = vrpn_TRACKER_SYNCING;
			just_read_something = 1;
			return 1;
		}

		default:
			fprintf(stderr, "vrpn_Tracker_Crossbow: sanity: unknown tracker state\n");
			return 0;
	}
}

void vrpn_Tracker_Crossbow::reset() {
	const	char *cmd;
	unsigned char recv_buf[8];
	struct timeval timeout;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
#if 0 // doesn't help
	// First, take the comm port offline for a second
	vrpn_close_commport(serial_fd);
	vrpn_SleepMsecs(1000);
	serial_fd = vrpn_open_commport(portname, baudrate);
#endif

	vrpn_flush_output_buffer(serial_fd);
	vrpn_flush_input_buffer(serial_fd);

	// Try resetting by toggling the RTS line of the serial port
	vrpn_set_rts(serial_fd);
	vrpn_SleepMsecs(750);
	vrpn_clear_rts(serial_fd);
	vrpn_SleepMsecs(250);
	vrpn_gettimeofday(&timestamp, NULL);

	vrpn_flush_input_buffer(serial_fd);

	cmd = "P";
	vrpn_write_characters(serial_fd, reinterpret_cast<const unsigned char*> (cmd), 1);
	vrpn_SleepMsecs(50); // Sleep long enough to stop receiving data
	vrpn_flush_input_buffer(serial_fd);

	cmd = "RSv";
	vrpn_write_characters(serial_fd, reinterpret_cast<const unsigned char*> (cmd), 3);
	vrpn_drain_output_buffer(serial_fd);
	if (vrpn_read_available_characters(serial_fd, recv_buf, 8, &timeout) != 8) {
		fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Crossbow not responding to stimulus\n");
		status = vrpn_TRACKER_FAIL;
		return;
	}

	if ((recv_buf[0] != 'H') || (recv_buf[1] != 255) || (recv_buf[7] != 255)) {
		fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Crossbow gave unexpected ping response\n");
		status = vrpn_TRACKER_FAIL;
		return;
	}

	if (recv_buf[6] != ((recv_buf[2] + recv_buf[3] + recv_buf[4] + recv_buf[5]) & 0xFF)) {
		fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Crossbow gave invalid serial number checksum\n");
		status = vrpn_TRACKER_FAIL;
		return;
	}

	const char *bufptr = reinterpret_cast<const char *>(&recv_buf[2]);
	vrpn_unbuffer(&bufptr, &device_serial);

	if (0) do {
		if (!vrpn_read_available_characters(serial_fd, recv_buf, 1, &timeout)) {
			fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Crossbow not responding to stimulus\n");
			status = vrpn_TRACKER_FAIL;
			return;
		}
	} while (*recv_buf != 255);

	int curSize = 4, curLen = 0;
	device_version = (char *) realloc(device_version, curSize * sizeof(char));
	if (device_version == NULL) {
		fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Out of memory\n");
		status = vrpn_TRACKER_FAIL;
		return;
	}
	do {
		if (!vrpn_read_available_characters(serial_fd, recv_buf, 1, &timeout)) {
			fprintf(stderr, "vrpn_Tracker_Crossbow::reset: Crossbow not responding to stimulus\n");
			status = vrpn_TRACKER_FAIL;
			return;
		}
		if (*recv_buf != '$')
			device_version[curLen++] = *recv_buf;

		if (curLen == curSize)
			device_version = (char *) realloc(device_version, curSize *= 2);
	} while (*recv_buf != '$');

	// Now null-terminate the version string, expanding it one last time if necessary
	if (curLen == curSize)
		device_version = (char *) realloc(device_version, ++curSize);

	device_version[curLen] = 0;

	//printf("Serial %u\tVersion '%s'\n", device_serial, device_version);

	just_read_something = 0;
	status = vrpn_TRACKER_SYNCING;

}

void vrpn_Tracker_Crossbow::ping() {
	unsigned char buffer = 'R';
	struct timeval timeout;

	vrpn_write_characters(serial_fd, &buffer, 1);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	while (vrpn_read_available_characters(serial_fd, &buffer, 1, &timeout) == 1) {
		if (buffer == 'H')
			return;
	}

	fprintf(stderr, "vrpn_Tracker_Crossbow: Crossbow device not responding to ping\n");
}

void vrpn_Tracker_Crossbow::recalibrate(vrpn_uint16 num_samples) {
	if (num_samples < 100) {
		fprintf(stderr, "vrpn_Tracker_Crossbow: Must recalibrate using at least 100 samples\n");
		return;
	}
	else if (num_samples > 25599) {
		fprintf(stderr, "vrpn_Tracker_Crossbow: Capping recalibration at 25,500 samples\n");
		num_samples = 25500;
	}

	vrpn_drain_output_buffer(serial_fd);
	vrpn_flush_input_buffer(serial_fd);

	// Prepare zero command
	unsigned char buffer[2];
	buffer[0] = 'z';
	buffer[1] = (unsigned char) (num_samples / 100);

	vrpn_write_characters(serial_fd, buffer, 2);
	vrpn_drain_output_buffer(serial_fd);
	vrpn_SleepMsecs(50);

	// Wait for affirmative response.
	// Allow two minutes before timing out.
	// Even 25500 samples should make it with a few seconds to spare.
	struct timeval timeout;
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;
	if (!vrpn_read_available_characters(serial_fd, buffer, 1, &timeout) || *buffer != 'Z') {
		fprintf(stderr, "vrpn_Tracker_Crossbow: Failed to recalibrate device\n");
	}
}

vrpn_uint32 vrpn_Tracker_Crossbow::get_serial_number() {
	if (!device_serial)
		reset();
	return device_serial;
}

const char *vrpn_Tracker_Crossbow::get_version_string() {
	if (!device_version)
		reset();
	return device_version;
}


void vrpn_Tracker_Crossbow::mainloop() {
	vrpn_Tracker_Serial::mainloop();
}

// Utility function to convert packed inputs into decoded floating-point equivalents.
float vrpn_Tracker_Crossbow::convert_scalar(vrpn_int16 data, float scale) const {
	return data * scale / 32768; // 2^15
}

void vrpn_Tracker_Crossbow::process_packet(const raw_packet &packet) {
	// Use the current time for a timestamp.
	vrpn_gettimeofday(&timestamp, NULL);

	// Clear the position record.
	memset(pos, 0, sizeof(pos));
	
	// Calculate the current orientation. (We don't know yaw, so report 0.)
	double pitch = convert_scalar(packet.pitch_angle, 180.0f) * VRPN_DEGREES_TO_RADIANS;
	double roll = convert_scalar(packet.roll_angle, 180.0f) * VRPN_DEGREES_TO_RADIANS;
	xb_quat_from_euler(d_quat, pitch, roll);

	// Clear the linear velocity; we don't know it.
	memset(vel, 0, sizeof(vel));

	// Calculate the current angular velocity from yaw rate
	// It's in degrees per second, so convert to radians per second.
	q_from_euler(vel_quat, convert_scalar(packet.yaw_rate, 1.5f * ang_accel_range) * VRPN_DEGREES_TO_RADIANS, 0, 0);
	vel_quat_dt = 1;

	// Calculate the current acceleration vector
	acc[0] = convert_scalar(packet.accel_x, 1.5f * lin_accel_range) * MPSS_PER_G;
	acc[1] = convert_scalar(packet.accel_y, 1.5f * lin_accel_range) * MPSS_PER_G;
	acc[2] = convert_scalar(packet.accel_z, 1.5f * lin_accel_range) * MPSS_PER_G;

	//printf("RawAccel = %hd\tG-Range = %f\tDerivedZ = %f\n", packet.accel_z, lin_accel_range, acc[2]);

	// The angular acceleration vector is nil. (0001 / 1)
	acc_quat[0] = acc_quat[1] = acc_quat[2] = 0;
	acc_quat[3] = acc_quat_dt = 1;
}

void vrpn_Tracker_Crossbow::send_report(void) {
    // Send the message on the connection
    if (d_connection) {
		char msgbuf[1000];
		int	len = encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}

		len = encode_acc_to(msgbuf);
		if (d_connection->pack_message(len, timestamp, accel_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}

		len = encode_vel_to(msgbuf);
		if (d_connection->pack_message(len, timestamp, velocity_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"Tracker: cannot write message: tossing\n");
		}
    } else {
	    fprintf(stderr,"Tracker: No valid connection\n");
    }
}

// Creates orientation quaternion from pitch and roll Euler angles.
// Crossbow angles are given such that p' = yaw(pitch(roll(p))),
// but quatlib angles assume p' = roll(pitch(yaw(p))).
// Hence the apparent reimplementation of q_from_euler.
void vrpn_Tracker_Crossbow::xb_quat_from_euler(q_type destQuat, double pitch, double roll) const {
	double sinP = sin(pitch / 2.0);
	double cosP = cos(pitch / 2.0);
	double sinR = sin(roll / 2.0);
	double cosR = cos(roll / 2.0);

	destQuat[Q_X] = sinP * cosR;
	destQuat[Q_Y] = cosP * sinR;
	destQuat[Q_Z] = -sinP * sinR;
	destQuat[Q_W] = cosP * cosR;
}

