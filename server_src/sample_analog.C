#include <vrpn_Connection.h>
#include <vrpn_Analog.h>

#include <stdio.h>  // fprintf()
#include <stdlib.h>  // exit()

#include <math.h>  // sin(), cos() for debugging

// sample_analog
// Tom Hudson, February 1999
//
// Server program that opens up a port and reports over it
// any changes to the array of doubles exposed as
// vrpn_Analog_Server::channels().  The size of this array is
// defined in vrpn_Analog.h;  currently 128.
//
// init_* and do_* are sample routines - put whatever you need there.
//
// Sample routines should cause two rising ramps, with video rate
// 5x audio rate.

void init_audio_throughput_magic (int, char **) {

}
void do_audio_throughput_magic (double * channels) {
  static int done = 0;

#if 1
  if (!done) {
    channels[0] = 0.0;
    done = 1;
  } else
    channels[0] += 0.5;
#else
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  channels[0] = sin(((double) now.tv_usec) / 1000000L);
#endif

//fprintf(stderr, "Audio channel %.2g\n", channels[0]);

}
void init_video_throughput_magic (int, char **) {

}
void do_video_throughput_magic (double * channels) {
  static int done = 0;

#if 1
  if (!done) {
    channels[0] = 0.0;
    done = 1;
  } else
    channels[0] += 2.5;
#else
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  channels[0] = cos(((double) now.tv_usec) / 1000000L);
#endif

//fprintf(stderr, "Video channel %.2g\n", channels[0]);

}

int main (int argc, char ** argv) {

  vrpn_Connection * c;
  vrpn_Analog_Server * ats;
  vrpn_Analog_Server * vts;

  struct timeval delay;

  c = vrpn_create_server_connection();
  ats = new vrpn_Analog_Server ("audio_throughput", c);
  ats->setNumChannels(1);
  vts = new vrpn_Analog_Server ("video_throughput", c);
  vts->setNumChannels(1);

  printf("Services named audio-throughput and video-throughput now listening on port %d.\n", vrpn_DEFAULT_LISTEN_PORT_NO);

  delay.tv_sec = 1L;
  delay.tv_usec = 0L;

  init_audio_throughput_magic(argc, argv);
  init_video_throughput_magic(argc, argv);

  while (1) {
    do_audio_throughput_magic(ats->channels());
    ats->report_changes();
    ats->mainloop();
    do_video_throughput_magic(vts->channels());
    vts->report_changes();
    vts->mainloop();
    c->mainloop(&delay);
fprintf(stderr, "while():  a = %.2g, v = %.2g.\n", ats->channels()[0],
vts->channels()[0]);
  }
}

