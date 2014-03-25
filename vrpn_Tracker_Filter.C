// Internal Includes
#include "vrpn_Tracker_Filter.h"

#include "vrpn_HumanInterface.h"        // for vrpn_HidInterface, etc
#include "vrpn_OneEuroFilter.h"         // for OneEuroFilterQuat, etc
#include "vrpn_SendTextMessageStreamProxy.h"  // for operator<<, etc

// Library/third-party includes
// - none

// Standard includes
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for char_traits, basic_string, etc
#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for fprintf, NULL, stderr
#include <string.h>                     // for memset

void VRPN_CALLBACK vrpn_Tracker_FilterOneEuro::handle_tracker_update(void *userdata, const vrpn_TRACKERCB info)
{
  // Get pointer to the object we're dealing with.
  vrpn_Tracker_FilterOneEuro *me = static_cast<vrpn_Tracker_FilterOneEuro *>(userdata);

  // See if this sensor is within our range.  If not, we ignore it.
  if (info.sensor >= me->d_channels) {
    return;
  }

  // Filter the position and orientation and then report the filtered value
  // for this channel.  Keep track of the delta-time, and update our current
  // time so we get the right one next time.
  double dt = vrpn_TimevalDurationSeconds(info.msg_time, me->d_last_report_times[info.sensor]);
  if (dt <= 0) { dt = 1; }  // Avoid divide-by-zero in case of fluke.
  vrpn_float64 pos[3];
  vrpn_float64 quat[4];
  memcpy(pos, info.pos, sizeof(pos));
  memcpy(quat, info.quat, sizeof(quat));
  const vrpn_float64 *filtered = me->d_filters[info.sensor].filter(dt, pos);
  q_vec_copy(me->pos, filtered);
  const double *q_filtered = me->d_qfilters[info.sensor].filter(dt, quat);
  q_normalize(me->d_quat, q_filtered);
  me->timestamp = info.msg_time;
  me->d_sensor = info.sensor;
  me->d_last_report_times[info.sensor] = info.msg_time;

  // Send the filtered report.
  char msgbuf[512];
  int len = me->encode_to(msgbuf);
  if (me->d_connection->pack_message(len, me->timestamp, me->position_m_id,
          me->d_sender_id, msgbuf,vrpn_CONNECTION_LOW_LATENCY)) {
    fprintf(stderr, "vrpn_Tracker_FilterOneEuro: cannot write message: tossing\n");
  }
}

vrpn_Tracker_FilterOneEuro::vrpn_Tracker_FilterOneEuro(const char * name, vrpn_Connection * con,
                                                       const char *listen_tracker_name,
                                                       unsigned channels, vrpn_float64 vecMinCutoff,
                                                       vrpn_float64 vecBeta, vrpn_float64 vecDerivativeCutoff,
                                                       vrpn_float64 quatMinCutoff, vrpn_float64 quatBeta,
                                                       vrpn_float64 quatDerivativeCutoff)
  : vrpn_Tracker(name, con)
  , d_channels(channels)
{
  // Allocate space for the times.  Fill them in with now.
  d_last_report_times = new struct timeval[channels];
  if (d_last_report_times == NULL) {
    fprintf(stderr,"vrpn_Tracker_FilterOneEuro::vrpn_Tracker_FilterOneEuro(): Out of memory\n");
    d_channels = 0;
    return;
  }
  
  vrpn_gettimeofday(&timestamp, NULL);

  // Allocate space for the filters.
  d_filters = new vrpn_OneEuroFilterVec[channels];
  d_qfilters = new vrpn_OneEuroFilterQuat[channels];
  if ( (d_filters == NULL) || (d_qfilters == NULL) ) {
    fprintf(stderr,"vrpn_Tracker_FilterOneEuro::vrpn_Tracker_FilterOneEuro(): Out of memory\n");
    d_channels = 0;
    return;
  }

  // Fill in the parameters for each filter.
  for (int i = 0; i < static_cast<int>(channels); ++i) {
    d_filters[i].setMinCutoff(vecMinCutoff);
    d_filters[i].setBeta(vecBeta);
    d_filters[i].setDerivativeCutoff(vecDerivativeCutoff);

    d_qfilters[i].setMinCutoff(quatMinCutoff);
    d_qfilters[i].setBeta(quatBeta);
    d_qfilters[i].setDerivativeCutoff(quatDerivativeCutoff);
  }

  // Open and set up callback handler for the tracker we're listening to.
  // If the name starts with the '*' character, use the server
  // connection rather than making a new one.
  if (listen_tracker_name[0] == '*') {
    d_listen_tracker = new vrpn_Tracker_Remote(&(listen_tracker_name[1]),
      d_connection);
  } else {
    d_listen_tracker = new vrpn_Tracker_Remote(listen_tracker_name);
  }
  d_listen_tracker->register_change_handler(this, handle_tracker_update);
}

vrpn_Tracker_FilterOneEuro::~vrpn_Tracker_FilterOneEuro()
{
  d_listen_tracker->unregister_change_handler(this, handle_tracker_update);
  delete d_listen_tracker;
  if (d_qfilters) { delete [] d_qfilters; d_qfilters = NULL; }
  if (d_filters) { delete [] d_filters; d_filters = NULL; }
  if (d_last_report_times) { delete [] d_last_report_times; d_last_report_times = NULL; }
}

void vrpn_Tracker_FilterOneEuro::mainloop()
{
  // See if we have anything new from our tracker.
  d_listen_tracker->mainloop();

  // server update
  vrpn_Tracker::server_mainloop();
}
