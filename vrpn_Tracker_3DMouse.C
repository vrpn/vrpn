#include <ctype.h>                      // for isprint
#include <math.h>                       // for cos, sin
#include <stdio.h>                      // for fprintf, stderr, perror

#include "vrpn_Serial.h"                // for vrpn_write_characters, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, timeval
#include "vrpn_Tracker_3DMouse.h"
#include "vrpn_Types.h"                 // for vrpn_float64

class VRPN_API vrpn_Connection;

// max time between reports (usec)
#define MAX_TIME_INTERVAL       (2000000) 

vrpn_Tracker_3DMouse::vrpn_Tracker_3DMouse(const char *name, vrpn_Connection *c, 
		      const char *port, long baud, int filtering_count):
    vrpn_Tracker_Serial(name, c, port, baud),
    vrpn_Button_Filter(name, c),
    _filtering_count(filtering_count),
    _numbuttons(5),
    _count(0)
{
	vrpn_Button::num_buttons = _numbuttons;
	clear_values();
}

void vrpn_Tracker_3DMouse::clear_values()
{
	int i;
	for (i=0; i <_numbuttons; i++)
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
}

vrpn_Tracker_3DMouse::~vrpn_Tracker_3DMouse()
{
}

void vrpn_Tracker_3DMouse::reset()
{
	int ret, i;

	clear_values();

	fprintf(stderr, "Resetting the 3DMouse...\n");
	if (vrpn_write_characters(serial_fd, (const unsigned char*)"*R", 2) == 2)
	{
		fprintf(stderr,".");
		vrpn_SleepMsecs(1000.0*2);  // Wait after each character to give it time to respond
	}
	else
	{
		perror("3DMouse: Failed writing to 3DMouse");
		status = vrpn_TRACKER_FAIL;
		return;
	}

	fprintf(stderr,"\n");

	// Get rid of the characters left over from before the reset
	vrpn_flush_input_buffer(serial_fd);

	// Make sure that the tracker has stopped sending characters
	vrpn_SleepMsecs(1000.0*2);

	if ( (ret = vrpn_read_available_characters(serial_fd, _buffer, 80)) != 0)
	{
		fprintf(stderr, "Got >=%d characters after reset\n", ret);
		for (i = 0; i < ret; i++)
		{
			if (isprint(_buffer[i])) fprintf(stderr,"%c",_buffer[i]);
			else fprintf(stderr,"[0x%02X]",_buffer[i]);
		}
		fprintf(stderr, "\n");
		vrpn_flush_input_buffer(serial_fd);		// Flush what's left
	}

	// Asking for tracker status
	if (vrpn_write_characters(serial_fd, (const unsigned char *) "*\x05", 2) == 2)
		vrpn_SleepMsecs(1000.0*1); // Sleep for a second to let it respond
	else
	{
		perror("  3DMouse write failed");
		status = vrpn_TRACKER_FAIL;
		return;
	}

	// Read Status
	bool success = true;

	ret = vrpn_read_available_characters(serial_fd, _buffer, 2);
	if (ret != 2) fprintf(stderr, "  Got %d of 5 characters for status\n",ret);

	fprintf(stderr, "	Control Unit test       : ");
	if (_buffer[0] & 1) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	Processor test          : ");
	if (_buffer[0] & 2) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	EPROM checksum test     : ");
	if (_buffer[0] & 4) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	RAM checksum test       : ");
	if (_buffer[0] & 8) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	Transmitter test        : ");
	if (_buffer[0] & 16) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	Receiver test           : ");
	if (_buffer[0] & 32) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	Serial Port test        : ");
	if (_buffer[1] & 1) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	fprintf(stderr, "	EEPROM test             : ");
	if (_buffer[0] & 2) fprintf(stderr, "success\n");
	else
	{
		fprintf(stderr, "fail\n");
		success = false;
	}

	if (!success)
	{
		fprintf(stderr, "Bad status report from 3DMouse, retrying reset\n");
		status = vrpn_TRACKER_FAIL;
		return;
	}
	else
	{
		fprintf(stderr, "3DMouse gives status (this is good)\n");
	}

	// Set filtering count if the constructor parameter said to.
	if (_filtering_count > 1)
	{
		if (!set_filtering_count(_filtering_count)) return;
	}
	fprintf(stderr, "Reset Completed (this is good)\n");
	status = vrpn_TRACKER_SYNCING;	// We're trying for a new reading
}

bool vrpn_Tracker_3DMouse::set_filtering_count(int count)
{
	char sBuf[16];

	sBuf[0] = 0x2a;
	sBuf[1] = 0x24;
	sBuf[2] = 2;
	sBuf[3] = 7;
	sBuf[4] = count;

	if (vrpn_write_characters(serial_fd, (const unsigned char*)sBuf, 5) == 5)
	{
		vrpn_SleepMsecs(1000.0*1);
	}
	else
	{
		perror("  3DMouse write filtering count failed");
		status = vrpn_TRACKER_FAIL;
		return false;
	}

	return true;
}

int vrpn_Tracker_3DMouse::get_report(void)
{
   int ret;		// Return value from function call to be checked
   timeval waittime;
   waittime.tv_sec = 2;
   waittime.tv_usec = 0;

   if (status == vrpn_TRACKER_SYNCING) {
	unsigned char tmpc;

	if (vrpn_write_characters(serial_fd, (const unsigned char*)"*d", 2) !=2)
	{
		perror("  3DMouse write command failed");
		status = vrpn_TRACKER_RESETTING;
		return 0;
	}
	ret = vrpn_read_available_characters(serial_fd, _buffer+_count, 16-_count, &waittime);
	if (ret < 0)
	{
		perror("  3DMouse read failed (disconnected)");
		status = vrpn_TRACKER_RESETTING;
		return 0;
	}

	_count += ret;
	if (_count < 16) return 0;
	if (_count > 16)
	{
		perror("  3DMouse read failed (wrong message)");
		status = vrpn_TRACKER_RESETTING;
		return 0;
	}

	_count = 0;
	
	tmpc = _buffer[0];
	if (tmpc & 32)
	{
		//printf("port%d: Out of Range\n", i);
		//ret |= 1 << (3-i);
	} else
	{
		long ax, ay, az;           // integer form of absolute translational data
		short arx, ary, arz;       // integer form of absolute rotational data
		float p, y, r;

		ax = (_buffer[1] & 0x40) ? 0xFFE00000 : 0;
		ax |= (long)(_buffer[1] & 0x7f) << 14;
		ax |= (long)(_buffer[2] & 0x7f) << 7;
		ax |= (_buffer[3] & 0x7f);

		ay = (_buffer[4] & 0x40) ? 0xFFE00000 : 0;
		ay |= (long)(_buffer[4] & 0x7f) << 14;
		ay |= (long)(_buffer[5] & 0x7f) << 7;
		ay |= (_buffer[6] & 0x7f);

		az = (_buffer[7] & 0x40) ? 0xFFE00000 : 0;
		az |= (long)(_buffer[7] & 0x7f) << 14;
		az |= (long)(_buffer[8] & 0x7f) << 7;
		az |= (_buffer[9] & 0x7f);

		pos[0] = static_cast<float>(ax / 100000.0 * 2.54);
		pos[2] = static_cast<float>(ay / 100000.0 * 2.54);
		pos[1] = -static_cast<float>(az / 100000.0f * 2.54);

		arx  = (_buffer[10] & 0x7f) << 7;
		arx += (_buffer[11] & 0x7f);

		ary  = (_buffer[12] & 0x7f) << 7;
		ary += (_buffer[13] & 0x7f);

		arz  = (_buffer[14] & 0x7f) << 7;
		arz += (_buffer[15] & 0x7f);

		p = static_cast<float>(arx / 40.0);		// pitch
		y = static_cast<float>(ary / 40.0);		// yaw
		r = static_cast<float>(arz / 40.0);		// roll

		p = static_cast<float>(p * VRPN_PI / 180);
		y = static_cast<float>(y * VRPN_PI / 180);
		r = static_cast<float>((360-r) * VRPN_PI / 180);

		float cosp2 = static_cast<float>(cos(p/2));
		float cosy2 = static_cast<float>(cos(y/2));
		float cosr2 = static_cast<float>(cos(r/2));
		float sinp2 = static_cast<float>(sin(p/2));
		float siny2 = static_cast<float>(sin(y/2));
		float sinr2 = static_cast<float>(sin(r/2));

		d_quat[0] = cosr2*sinp2*cosy2 + sinr2*cosp2*siny2;
		d_quat[1] = sinr2*cosp2*cosy2 + cosr2*sinp2*siny2;
		d_quat[2] = cosr2*cosp2*siny2 + sinr2*sinp2*cosy2;
		d_quat[3] = cosr2*cosp2*cosy2 + sinr2*sinp2*siny2;

	}

	buttons[0] = tmpc & 16;		// Mouse stand button
	buttons[1] = tmpc & 8;		// Suspend button
	buttons[2] = tmpc & 4;		// Left button
	buttons[3] = tmpc & 2;		// Middle button
	buttons[4] = tmpc & 1;		// Right button
    }

    vrpn_Button::report_changes();

    status = vrpn_TRACKER_SYNCING;
    bufcount = 0;

#ifdef VERBOSE2
      print_latest_report();
#endif

   return 1;
}

void vrpn_Tracker_3DMouse::mainloop()
{
	server_mainloop();

	switch(status)
	{
		case vrpn_TRACKER_AWAITING_STATION:
		case vrpn_TRACKER_PARTIAL:
		case vrpn_TRACKER_SYNCING:
			if (get_report()) send_report();
			break;
		case vrpn_TRACKER_RESETTING:
			reset();
			break;
		case vrpn_TRACKER_FAIL:
			fprintf(stderr, "3DMouse failed, trying to reset (Try power cycle if more than 4 attempts made)\n");
			if (serial_fd >= 0)
			{
				vrpn_close_commport(serial_fd);
				serial_fd = -1;
			}
			if ( (serial_fd=vrpn_open_commport(portname, baudrate)) == -1)
			{
				fprintf(stderr,"vrpn_Tracker_3DMouse::mainloop(): Cannot Open serial port\n");
				status = vrpn_TRACKER_FAIL;
				return;
			}
			status = vrpn_TRACKER_RESETTING;
			break;
		default:
			break;
	}
}

