#include "analog_GTK.h"
#include "quat.h"
#include <iostream>
#include <vrpn_Shared.h>                // for vrpn_gettimeofday

analog_GTK::analog_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder) : vrpn_Analog(name, connection) {
  g_signal_connect (gtk_builder_get_object (builder, "analog"), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkAdjustment *analog = NULL;
  unsigned int index = 1;
  do {
    char name[128];
    sprintf(name, "v analog %d", index);
    analog = GTK_ADJUSTMENT(gtk_builder_get_object (builder, name));
    if (analog) {
      d_analogs.push_back(analog);
      g_signal_connect (analog, "value-changed", G_CALLBACK (analog_cb), this);
    }
    index ++;
  } while (analog != NULL);

  num_channel = d_analogs.size();

  for(unsigned int i = 0; i < num_channel; i++) {
    channel[i] = last[i] = 0;
  }
}

void analog_GTK::analog_cb(GtkAdjustment *analog, analog_GTK *obj) {
  vrpn_gettimeofday( &obj->timestamp, NULL );

  for (unsigned int i = 0 ; i  < obj->d_analogs.size() ; i++) {
    if (obj->d_analogs[i] == analog) {
      obj->channel[i] = gtk_adjustment_get_value(analog);
    }
  }
  obj->mainloop();
}

void analog_GTK::mainloop () {
  server_mainloop();

  report_changes();
}
