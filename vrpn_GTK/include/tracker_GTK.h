#ifndef TRACKER_GTK_H
#define TRACKER_GTK_H

#include <vrpn_Tracker.h>
#include <gtk/gtk.h>

class tracker_GTK : public vrpn_Tracker {
  GtkComboBox * d_frequency_box;
  GtkTreeModel * d_frequency_model;
  GtkAdjustment *d_sensors;
  GtkAdjustment *d_positionAdjustment[3];
  GtkAdjustment *d_orientationAdjustment[3];

  float **d_positions;
  float **d_orientations;

  bool d_sendValues;

  timespec d_previous;

  virtual void mainloop ();

  gboolean idle();
  static gboolean idle_cb(gpointer data);

  void sensors(unsigned int newSensor);
  static void sensors_cb(GtkAdjustment *widget, tracker_GTK *obj);

  static void reset_cb(GtkWidget *widget, tracker_GTK *obj);
  static void adjustments_cb(GtkAdjustment *widget, tracker_GTK *obj);

  void setPositionRange(float min, float max, GtkAdjustment *component);

 public:
  tracker_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder, unsigned int numberSensors = UINT_MAX);
  void setPositionRange(float min, float max, char component = ' ');
};

#endif // defined(TRACKER_GTK_H)
