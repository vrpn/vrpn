/// This object has been superceded by the vrpn_Tracker_AnalogFly object.

#include <math.h>                       // for fabs
#include <string.h>                     // for memcpy

#include "quat.h"                       // for q_matrix_copy, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_JoyFly.h"
#include "vrpn_Types.h"                 // for vrpn_float64, vrpn_int32

vrpn_Tracker_JoyFly::vrpn_Tracker_JoyFly
        (const char * name, vrpn_Connection * c,
         const char * source, const char * set_config,
         vrpn_Connection * sourceConnection) :
    vrpn_Tracker (name, c)
{
  int i;

  joy_remote = new vrpn_Analog_Remote(source, sourceConnection);
  joy_remote->register_change_handler(this, handle_joystick);
  c->register_handler(c->register_message_type(vrpn_got_connection),
		      handle_newConnection, this);
  
  FILE * fp = fopen(set_config, "r");
  if (fp == NULL) {
    fprintf(stderr, "Can't open joy config file, using default..\n");
    fprintf(stderr, "Channel Accel 1.0, Power 1, Init Identity Matrix. \n");
    for (i=0; i< 7; i++) {
      chanAccel[i] = 1.0;
      chanPower[i] = 1;
    }
    for ( i =0; i< 4; i++) {
      for (int j=0; j< 4; j++) {
	initMatrix[i][j] = 0;
      }
    }
    initMatrix[0][0] =     initMatrix[2][2] =     
    initMatrix[1][1] =     initMatrix[3][3] = 1.0;
  } else {
    for (i=0; i< 7; i++) {
      if (fscanf(fp, "%lf %d", &chanAccel[i], &chanPower[i]) != 2) {
	fprintf(stderr,"Cannot read acceleration and power from file\n");
	fclose(fp);
	return;
      }
      fprintf(stderr, "Chan[%d] = (%lf %d)\n", i, chanAccel[i], chanPower[i]);
    }
    for (i =0; i< 4; i++) {
      for (int j=0; j< 4; j++) {
	if (fscanf(fp, "%lf", &initMatrix[i][j]) < 0) {
		perror("vrpn_Tracker_JoyFly::vrpn_Tracker_JoyFly(): Could not read matrix value");
		fclose(fp);
		return;
	}
      }
    }
    fclose(fp);
  }

  q_matrix_copy(currentMatrix, initMatrix);
}

vrpn_Tracker_JoyFly::~vrpn_Tracker_JoyFly()
{
  delete joy_remote;
}

void vrpn_Tracker_JoyFly::mainloop(void)
{
  server_mainloop();

  if (joy_remote !=  NULL)
    joy_remote->mainloop();
  if (status == vrpn_TRACKER_REPORT_READY) {
    // pack and deliver tracker report;
    fprintf(stderr, "Sending a report\n");

    char msgbuf[1000];
    vrpn_int32	    len = encode_to(msgbuf);
    if (d_connection->pack_message(len, timestamp,
				 position_m_id, d_sender_id, msgbuf,
				 vrpn_CONNECTION_LOW_LATENCY)) {
      fprintf(stderr,
	      "\nvrpn_Tracker_Flock: cannot write message ...  tossing");
    } else {
	fprintf(stderr,"\nvrpn_Tracker_Flock: No valid connection");
    }
    status = vrpn_TRACKER_SYNCING;
  } 	
}


#define ONE_SEC (1000000l)
// static
void vrpn_Tracker_JoyFly::handle_joystick
                         (void * userdata, const vrpn_ANALOGCB b)
{
  double tx, ty, tz, rx, ry, rz;
  double temp[7];
  int i,j;
  q_matrix_type newM;
  double deltaT;

  vrpn_Tracker_JoyFly * pts = (vrpn_Tracker_JoyFly *) userdata;

  printf("Joy total = %d,Chan[0] = %lf\n",
	 b.num_channel,b.channel[0]);

  if (pts->prevtime.tv_sec == -1) {
    deltaT = 1.0 ; // one milisecond;
  } else {
    deltaT = (pts->prevtime.tv_sec* ONE_SEC + pts->prevtime.tv_usec) -
      (b.msg_time.tv_sec *ONE_SEC + b.msg_time.tv_usec);
    deltaT /= 1000.0 ; // convert to millsecond;
  }
  
  memcpy(&(pts->prevtime), &b.msg_time, sizeof(struct timeval));

  for (i=0; i< 7; i++) {
    temp[i] = 1.0;
    //printf("chan[%d] = %lf\n", i, b.channel[i]);
    if (!(b.channel[i] >=-0.5 && b.channel[i] <= 0.5)) return;
    for (j=0; j< pts->chanPower[i]; j++)
      temp[i] *= b.channel[i];
    if (b.channel[i] > 0) {
      temp[i] *= pts->chanAccel[i];
    } else {
      temp[i] = -fabs(temp[i]) * pts->chanAccel[i];
    }
    temp[i] *= deltaT;
  }

  /* roll chan[3]  */
  rz = temp[3];

  /* pitch  Chan[4]*/
  rx = temp[4];
  
  /* yaw Chan[5]*/
  ry = temp[5];  


  /* translation, x chan[0], y: chane[2], z: chan[1] */
  tx = -temp[0]; // tx is NEGTIVE of power !!!  ;
  ty = temp[2];
  tz = temp[1];

  q_euler_to_col_matrix(newM, rz, ry, rx);
  newM[3][0] = tx; newM[3][1] = ty; newM[3][2] = tz;
  pts->update(newM);
}

void vrpn_Tracker_JoyFly::update(q_matrix_type & newM) {

  q_matrix_type final;
  q_xyz_quat_type xq;
  int i;

  q_matrix_mult(final, newM, currentMatrix);
  q_matrix_copy(currentMatrix, final);
  q_row_matrix_to_xyz_quat( & xq, currentMatrix);

  
  status = vrpn_TRACKER_REPORT_READY;
  for (i=0; i< 3; i++) {
    pos[i] = xq.xyz[i]; // position;
  }
  printf("(x, y, z)= %lf %lf %lf\n", pos[0],pos[1], pos[2]);
  for (i=0; i< 4; i++) {
    d_quat[i] = xq.quat[i]; // orientation. 
  }
}

 
// static
int vrpn_Tracker_JoyFly::handle_newConnection
                        (void * userdata, vrpn_HANDLERPARAM) {
     
  printf("Get a new connection, reset virtual_Tracker\n");
  ((vrpn_Tracker_JoyFly *) userdata)->reset();
  return 0;
}

void vrpn_Tracker_JoyFly::reset() {
  q_matrix_copy(currentMatrix, initMatrix);
  prevtime.tv_sec = -1;
  prevtime.tv_usec = -1;
}







