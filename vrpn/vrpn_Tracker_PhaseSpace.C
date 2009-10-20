#include "vrpn_Tracker_PhaseSpace.h"

#include <string.h>

#ifdef VRPN_INCLUDE_PHASESPACE

vrpn_Tracker_PhaseSpace::vrpn_Tracker_PhaseSpace(const char *name, vrpn_Connection *c, const char* device, float frequency,int readflag, int slave) 
  : vrpn_Tracker(name,c)
{
#ifdef DEBUG
  printf("Constructor: vrpn_Tracker_PhaseSpace( %s, %s, %f, %d)\n",name,device,frequency,readflag);
#endif

  this->slave = slave;
  
  if(d_connection)
    {
      // Register a handler for the update change callback
      if (register_autodeleted_handler(update_rate_id, handle_update_rate_request, this, d_sender_id))
        {
          fprintf(stderr,"vrpn_Tracker: Can't register workspace handler\n");            }      
    }
  
  memset(r2s_map, -1, sizeof(vrpn_int32)*vrpn_PhaseSpace_MAXRIGIDS);
  numRigids = 0;
  numMarkers = 0;
  memset(markers, 0, sizeof(OWLMarker)*vrpn_PhaseSpace_MAXMARKERS);
  memset(rigids, 0, sizeof(OWLMarker)*vrpn_PhaseSpace_MAXRIGIDS);

  this->frequency = frequency;

  int owlflag = 0;
  if(slave)
    {
      owlflag |= OWL_SLAVE ;
    }

  if(owlInit(device,owlflag) < 0)
    {
      owlRunning = false;      
      return;
    }
  else
    {
      owlRunning = true;
    }

  char msg[512];

  if(owlGetString(OWL_VERSION,msg))
    {
      printf("OWL version: %s\n",msg);
    }
  else
    {
      printf("Unable to query OWL version.\n");
    }

  if(!slave)
    {
      
      owlTrackeri(0, OWL_CREATE, OWL_POINT_TRACKER);  //The master point tracker is index 0.  So all rigid trackers will start from 1.
      if(!owlGetStatus())
        {
          fprintf(stderr,"Error: Unable to create main point tracker.\n");
          return;
        }  
    }
  else
    {

      printf("Ignoring tracker creation in slave mode.\n");
    }
  readMostRecent = readflag ? true : false;
  
}


vrpn_Tracker_PhaseSpace::~vrpn_Tracker_PhaseSpace() 
{

#ifdef DEBUG
  printf("Destructor: vrpn_Tracker_PhaseSpace\n");
#endif
  
  if(owlRunning)
    {
      owlDone();
    }
}





// This function should be called each time through the main loop
// of the server code. It polls for data from the OWL server and
// sends them if available.

void vrpn_Tracker_PhaseSpace::mainloop() 
{
  get_report();

  // Call the generic server mainloop, since we are a server
  server_mainloop();

  return;
}

bool vrpn_Tracker_PhaseSpace::addMarker(int sensor,int led_id) 
{
#ifdef DEBUG
  printf("vrpn_Tracker_PhaseSpace %s: addMarker( %d, %d)\n", d_servicename, sensor, led_id);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  owlMarkeri(MARKER(0,sensor),OWL_SET_LED,led_id);

  if(!owlGetStatus())
    {
      return false;
    }

  numMarkers++;

  return true;
}

/*
This function must only be called after startNewRigidBody has been called.  
*/
bool vrpn_Tracker_PhaseSpace::addRigidMarker(int sensor, int led_id, float x, float y, float z) 
{
#ifdef DEBUG
  printf("vrpn_Tracker_PhaseSpace %s: addRigidMarker( %d, %d, %f, %f, %f)\n", d_servicename, sensor, led_id, x, y, z);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  if(numRigids == 0)
    {
      fprintf(stderr,"Error: Attempting to add rigid body marker with no rigid body defined.");
      return false;
    }

  float xyz[3];
  xyz[0] = x;
  xyz[1] = y;
  xyz[2] = z;

  owlMarkeri(MARKER(numRigids,sensor),OWL_SET_LED,led_id);
  owlMarkerfv(MARKER(numRigids,sensor),OWL_SET_POSITION,&xyz[0]);

  if(!owlGetStatus())
    {
      return false;
    }

  numMarkers++;

  return true;
}

/*
Starts a new rigid body definition and creates a tracker for it on
the server.  Use addRigidMarker() to add markers to the tracker.
Trackers must be disabled (using enableTracker(false)) before being
modified or created.  
*/
bool vrpn_Tracker_PhaseSpace::startNewRigidBody(int sensor)
{
#ifdef DEBUG
  printf("vrpn_Tracker_PhaseSpace %s: startNewRigidBody( %d)\n", d_servicename, sensor);
#endif

  if(!owlRunning) return false;
  if(slave) return false;

  numRigids++;

  owlTrackeri(numRigids, OWL_CREATE, OWL_RIGID_TRACKER);

  if(!owlGetStatus())
    {
      numRigids--;
      return false;
    }

  //remember the sensor that this rigid tracker is mapped to.  
  r2s_map[numRigids] = sensor;

  return true;
}

/*
Enables all the trackers and sets the streaming frequency.
Note: Trackers according to VRPN and Trackers as defined by OWL
are two entirely different things.
*/
bool vrpn_Tracker_PhaseSpace::enableTracker(bool enable) 
{
#ifdef DEBUG
  printf("vrpn_Tracker_PhaseSpace %s: enableTracker( %d)\n", d_servicename, enable);
#endif
  
  if(!owlRunning) return false;

  if(!slave)
    {

      int option = enable ? OWL_ENABLE : OWL_DISABLE;
      
      owlTracker(0,option); //enables/disables the point tracker
      
      if(!owlGetStatus())
        {
          return false;
        }
      
      //enable/disable the rigid trackers
      for(int i = 1; i <= numRigids; i++)
        {
          owlTracker(i,option);
          if(!owlGetStatus())
            {
              return false;
            }
        }
    }

  if(enable)
    {
      setFrequency(frequency);
      owlSetInteger(OWL_STREAMING,OWL_ENABLE);
      if(!owlGetStatus())
        {
          return false;
        }
      
      printf("Streaming enabled.\n");      
    } 
  else
    {
      owlSetInteger(OWL_STREAMING,OWL_DISABLE);
      if(!owlGetStatus())
        {
          return false;
        }
      
      printf("Streaming disabled.\n");
    }

  return true;
}
  
  
int vrpn_Tracker_PhaseSpace::get_report(void)
{
  if(!owlRunning) return 0;

  //get the OWL data
  int m = owlGetMarkers(markers,vrpn_PhaseSpace_MAXMARKERS);
  int r = owlGetRigids(rigids,vrpn_PhaseSpace_MAXRIGIDS);

  if(readMostRecent)
    {
      int lm = 0;
      int lr = 0;

      while(m > 0) 
        {
          lm = m;
          m = owlGetMarkers(markers,vrpn_PhaseSpace_MAXMARKERS);
        }
      while(r > 0)
        {
          lr = r;
          r = owlGetRigids(rigids,vrpn_PhaseSpace_MAXRIGIDS);
        }
      m = lm;
      r = lr;
    }

  for(int i = 0; i < m; i++)
    {
      if(markers[i].cond <= 0) continue;

      //set the sensor 
      d_sensor = INDEX(markers[i].id);
      
      //set the position
      pos[0] = markers[i].x;
      pos[1] = markers[i].y;
      pos[2] = markers[i].z;

      //raw positions have no rotation
      d_quat[0] = 0;
      d_quat[1] = 0;
      d_quat[2] = 0;
      d_quat[3] = 1;
     
      //send the report
      send_report();
    }
  for(int j = 0; j < r; j++)
    {
      if(rigids[j].cond <= 0) continue;

      //set the sensor
      d_sensor = r2s_map[rigids[j].id ];

      //set the position
      pos[0] = rigids[j].pose[0];
      pos[1] = rigids[j].pose[1];
      pos[2] = rigids[j].pose[2];

      //set the orientation quaternion
      //OWL has the scale factor first, whereas VRPN has it last.
      d_quat[0] = rigids[j].pose[4];
      d_quat[1] = rigids[j].pose[5];;
      d_quat[2] = rigids[j].pose[6];;
      d_quat[3] = rigids[j].pose[3];;
     
      //send the report
      send_report();
    }
   
  return m > 0 || r > 0 ? 1 : 0;

}
 
void vrpn_Tracker_PhaseSpace::send_report(void)
{
  if (d_connection) 
    {
      char	msgbuf[vrpn_PhaseSpace_MSGBUFSIZE];
      int	len = encode_to(msgbuf);
      if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
                                     vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr,"PhaseSpace: cannot write message: tossing\n");
      }
    }
}

void vrpn_Tracker_PhaseSpace::setFrequency(float freq)
{
#ifdef DEBUG
  printf("setFrequency: %f\n",freq);
#endif
  //if(slave) return;

  if(freq < 0 || freq > OWL_MAX_FREQUENCY)
    {
      fprintf(stderr,"Error:  Invalid frequency requested, defaulting to %f hz\n",OWL_MAX_FREQUENCY);
      freq = OWL_MAX_FREQUENCY;
    }
  frequency = freq;
  
  if(owlRunning)
    {
      float tmp = -1;
      owlSetFloat(OWL_FREQUENCY, frequency);
      owlGetFloatv(OWL_FREQUENCY, &tmp);
      if(!owlGetStatus() || tmp == -1)
        {
          fprintf(stderr,"Error: unable to set frequency\n");
        }
      
    }
  
  return;                  
}

int vrpn_Tracker_PhaseSpace::handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p)
{
#ifdef DEBUG
  printf("handle_update_rate_request\n");
#endif  

  vrpn_Tracker_PhaseSpace* thistracker = (vrpn_Tracker_PhaseSpace*)userdata;

  vrpn_float64 update_rate = 0;

  vrpn_unbuffer(&p.buffer,&update_rate);

  thistracker->setFrequency((float) update_rate);

  return 0;
}

#endif

