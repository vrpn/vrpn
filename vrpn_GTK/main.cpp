#include <gtk/gtk.h>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <vrpn_Tracker.h>
#include <string.h>
#include "tracker_GTK.h"
#include "button_GTK.h"
#include "analog_GTK.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef GTK_BUILDER_FILE
#    define GTK_BUILDER_FILE vrpn_GTK.xml
#endif

const char usage[] = " [-serverName name] [-vrpnPort portNumber] [-numberSensors numberSensors] [-min_max min_max_values]";

int main (int   argc, char *argv[]) {
  const char* name = "GTK";
  int port = vrpn_DEFAULT_LISTEN_PORT_NO;
  int numberSensors = UINT_MAX;
  float min_position = -3.0;
  float max_position = 3.0;

  for (int i = 1; i < argc; i++) {        
    if (strcmp(argv[i], "-serverName") == 0) {
      name = argv[++i];
    } else if (strcmp(argv[i], "-vrpnPort") == 0) {
      char *endptr;
      port = strtol(argv[++i], &endptr, 0);
      if ((*endptr) != '\0') {
	std::cout << "Error parsing command line.\n" << "Usage: " << argv[0] << usage << std::endl;
	port = vrpn_DEFAULT_LISTEN_PORT_NO;
      }
      //port = QString(argv[++i]).toInt();
    } else if (strcmp(argv[i], "-numberSensors") == 0) {
      char *endptr;
      numberSensors = strtol(argv[++i], &endptr, 0);
      if ((*endptr) != '\0') {
	std::cout << "Error parsing command line.\n" << "Usage: " << argv[0] << usage << std::endl;
	numberSensors = UINT_MAX;
      }
      //port = QString(argv[++i]).toInt();
    } else if (strcmp(argv[i], "-min_max") == 0) {
      char *endptr;
      float value = strtod(argv[++i], &endptr);
      if ((*endptr) != '\0') {
	std::cout << "Error parsing command line.\n" << "Usage: " << argv[0] << usage << std::endl;
      } else {
	if (value > 0) {
	  max_position = value;
	  min_position = -value;
	} else {
	  min_position = value;
	  max_position = -value;
	}
      }
      //port = QString(argv[++i]).toInt();
    } else {
	std::cout << "Error parsing command line.\n" << "Usage: " << argv[0] << usage << std::endl;
    }
  }

  vrpn_Connection* connection = vrpn_create_server_connection(port);

  gtk_init (&argc, &argv);

  setlocale(LC_NUMERIC,"C"); // GTK update "locales" ...

  /* Construct a GtkBuilder instance and load our UI description */
  GtkBuilder *builder = gtk_builder_new ();
  GError  *p_err = NULL;
  gtk_builder_add_from_file (builder, QUOTE(GTK_BUILDER_FILE) , &p_err);

  if (p_err != NULL) {
    std::cerr << "GTK error : " << p_err->message << std::endl;
    return -1;
  }

  /* Connect signal handlers to the constructed widgets. */
  tracker_GTK tracker(connection, name, builder, numberSensors);
  tracker.setPositionRange(min_position, max_position, ' ');
  button_GTK button(connection, name, builder);
  analog_GTK analog(connection, name, builder);
  
  gtk_main ();

  return 0;
}
