#ifndef BUTTON_GTK_H
#define BUTTON_GTK_H

#include <vrpn_Button.h>
#include <gtk/gtk.h>
#include <vector>

class button_GTK : public vrpn_Button {

  virtual void mainloop ();
  std::vector<GtkToggleButton *> d_buttons;

  static void button_cb(GtkToggleButton *button, button_GTK *obj);

 public:
  button_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder);
};

#endif // defined(BUTTON_GTK_H)
