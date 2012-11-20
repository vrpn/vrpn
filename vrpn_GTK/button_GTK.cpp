#include "button_GTK.h"
#include "quat.h"
#include <iostream>
#include <vrpn_Shared.h>                // for vrpn_gettimeofday

button_GTK::button_GTK(vrpn_Connection *connection, const char *name, GtkBuilder *builder) : vrpn_Button(name, connection) {
  g_signal_connect (gtk_builder_get_object (builder, "button"), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkToggleButton *button = NULL;
  unsigned int index = 1;
  do {
    char name[128];
    sprintf(name, "button %d", index);
    button = GTK_TOGGLE_BUTTON(gtk_builder_get_object (builder, name));
    if (button) {
      d_buttons.push_back(button);
      g_signal_connect (button, "clicked", G_CALLBACK (button_cb), this);
    }
    index ++;
  } while (button != NULL);

  num_buttons = d_buttons.size();

  for(unsigned int i = 0; i < num_buttons; i++) {
    buttons[i] = lastbuttons[i] = 0;
  }
}

void button_GTK::button_cb(GtkToggleButton *button, button_GTK *obj) {
  vrpn_gettimeofday( &obj->timestamp, NULL );

  for (unsigned int i = 0 ; i  < obj->d_buttons.size() ; i++) {
    if (obj->d_buttons[i] == button) {
      obj->buttons[i] = (gtk_toggle_button_get_active (button) ? 1 : 0);
    }
  }
  obj->mainloop();
}

void button_GTK::mainloop () {
  server_mainloop();

  report_changes();
}
