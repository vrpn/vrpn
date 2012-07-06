#include "tracker_GTK.h"
#include "quat.h"
#include <iostream>

tracker_GTK::tracker_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder, unsigned int numberSensors) : vrpn_Tracker(name, connection) {
  g_signal_connect (gtk_builder_get_object (builder, "tracker"), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  d_previous.tv_sec = 0;
  d_previous.tv_nsec = 0;

  d_sensors = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "target"));
  g_signal_connect (d_sensors, "value-changed", G_CALLBACK (sensors_cb), this);

  d_frequency_box = GTK_COMBO_BOX(gtk_builder_get_object (builder, "Frequency box"));
  d_frequency_model = gtk_combo_box_get_model (d_frequency_box);

  d_positionAdjustment[0] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "x position"));
  d_positionAdjustment[1] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "y position"));
  d_positionAdjustment[2] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "z position"));

  d_orientationAdjustment[0] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "x orientation"));
  d_orientationAdjustment[1] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "y orientation"));
  d_orientationAdjustment[2] = GTK_ADJUSTMENT(gtk_builder_get_object (builder, "z orientation"));

  for (unsigned int i = 0 ; i < 3 ; i++) {
    g_signal_connect (d_positionAdjustment[i], "value-changed", G_CALLBACK (adjustments_cb), this);
    g_signal_connect (d_orientationAdjustment[i], "value-changed", G_CALLBACK (adjustments_cb), this);
  }

  gtk_adjustment_set_lower(d_sensors, 0);
  if (numberSensors < UINT_MAX) {
    num_sensors = numberSensors;
    gtk_adjustment_set_upper(d_sensors, num_sensors);
  } else {
    num_sensors = gtk_adjustment_get_upper(d_sensors);
  }

  d_positions    = (float**) malloc(num_sensors * sizeof(float*));
  d_orientations = (float**) malloc(num_sensors * sizeof(float*));
  for (unsigned int i = 0 ; i < num_sensors ; i ++) {
    d_positions[i]    = (float*)malloc(3 * sizeof(float));
    d_orientations[i] = (float*)malloc(3 * sizeof(float));
    for (unsigned int j = 0 ; j < 3 ; j++) {
      d_positions   [i][j] = 0;
      d_orientations[i][j] = 0;
    }
  }

  g_idle_add(idle_cb, this);

  g_signal_connect (gtk_builder_get_object (builder, "reset"), "clicked", G_CALLBACK (reset_cb), this);
  


  d_sendValues = false;

  setPositionRange(-3.0, 3.0, ' ');

  register_server_handlers();

}

void tracker_GTK::setPositionRange(float min, float max, GtkAdjustment *component) {
  gtk_adjustment_set_lower(component, min);
  gtk_adjustment_set_upper(component, max);

  for (unsigned int i = 0 ; i < 3 ; i++) {
    workspace_min[i] = min;
    workspace_max[i] = max;
  }
}

void tracker_GTK::setPositionRange(float min, float max, char component) {
  switch (component) {
  case 'x' : setPositionRange(min, max, d_positionAdjustment[0]); break;
  case 'y' : setPositionRange(min, max, d_positionAdjustment[1]); break;
  case 'z' : setPositionRange(min, max, d_positionAdjustment[2]); break;
  default:
    setPositionRange(min, max, d_positionAdjustment[0]);
    setPositionRange(min, max, d_positionAdjustment[1]);
    setPositionRange(min, max, d_positionAdjustment[2]);
    break;
  }
}

void tracker_GTK::sensors(unsigned int newSensor) {
  for (unsigned int i = 0 ; i < 3 ; i++) {
    gtk_adjustment_set_value(d_positionAdjustment[i], d_positions[newSensor][i]);
    gtk_adjustment_set_value(d_orientationAdjustment[i], d_orientations[newSensor][i]);
  }
}

gboolean tracker_GTK::idle() {
  GtkTreeIter  iter;

  if (gtk_combo_box_get_active_iter (d_frequency_box, & iter)) {
    gfloat freq;
    gtk_tree_model_get (d_frequency_model, & iter, 1, &freq, -1);


    if (freq != 0) {
      long long int period = (long long int)(1000.0 / freq); // In millisecond
      struct timespec current;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &current);
      current.tv_nsec /= 1000000; // Transform tv_nsec to millisecond !

      unsigned long long int delta = (current.tv_sec - d_previous.tv_sec) * 1000 + current.tv_nsec - d_previous.tv_nsec;

      if (delta > period) {
	d_sendValues = true;
	d_previous.tv_sec = current.tv_sec;
	d_previous.tv_nsec = current.tv_nsec;
      }
    }
  }

  mainloop();

  return true;
}

void tracker_GTK::reset_cb(GtkWidget *widget, tracker_GTK *obj) {
  unsigned int sensor = gtk_adjustment_get_value(obj->d_sensors);
  if (sensor < obj->num_sensors) {
    for (unsigned int i = 0 ; i < 3 ; i++) {
      obj->d_positions[sensor][i] = 0.0;
      obj->d_orientations[sensor][i] = 0.0;
    }
    obj->sensors(sensor);
  }
}

void tracker_GTK::sensors_cb(GtkAdjustment *widget, tracker_GTK *obj) {
  unsigned int newSensor = gtk_adjustment_get_value(widget);
  if (newSensor < obj->num_sensors) {
    obj->sensors(newSensor);
  }
}

void tracker_GTK::adjustments_cb(GtkAdjustment *widget, tracker_GTK *obj) {
  unsigned int currentSensor = gtk_adjustment_get_value(obj->d_sensors);
  for (unsigned int i = 0 ; i < 3 ; i++) {
    if (obj->d_positionAdjustment[i] == widget) {
      obj->d_positions[currentSensor][i] = gtk_adjustment_get_value(widget);
      break;
    }
    if (obj->d_orientationAdjustment[i] == widget) {
      obj->d_orientations[currentSensor][i] = gtk_adjustment_get_value(widget);
      break;
    }
  }
}

gboolean tracker_GTK::idle_cb(gpointer data) {
  return ((tracker_GTK*)data)->idle();
}

void tracker_GTK::mainloop () {
  d_connection->mainloop();

  server_mainloop();
  
  if (d_sendValues) {

    struct timeval current_time;
    vrpn_gettimeofday(&current_time, NULL);

    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;


    char msgbuf[1000];
    unsigned int len;
    for (unsigned int i = 0 ; i < num_sensors ; i++) {
      d_sensor = i;

      q_from_euler (d_quat, d_orientations[i][2], d_orientations[i][1], d_orientations[i][0]);

      for (unsigned int j = 0 ; j < 3 ; j++) {
	pos[j] = d_positions[i][j];
      }

      // Même chose pour les quaternions !

      // Pack position report
      len = encode_to(msgbuf);
      if (d_connection->pack_message(len, timestamp,
				     position_m_id, d_sender_id, msgbuf,
				     vrpn_CONNECTION_LOW_LATENCY)) {
	fprintf(stderr,"NULL tracker: can't write message: tossing\n");
      }
    }
  }

  d_sendValues = false;

  // Send updates
  //vrpn_Tracker::report_changes();
}
