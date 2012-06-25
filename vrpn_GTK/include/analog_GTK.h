#ifndef ANALOG_GTK_H
#define ANALOG_GTK_H

#include <vrpn_Analog.h>
#include <gtk/gtk.h>
#include <vector>

class analog_GTK : public vrpn_Analog {

  virtual void mainloop ();

  std::vector<GtkAdjustment *> d_analogs;

  static void analog_cb(GtkAdjustment *button, analog_GTK *obj);

 public:
  analog_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder);
};

#endif // defined(ANALOG_GTK_H)
