#include "vrpn_Android.h"
#include <sstream>
#include <string>
#include <bitset>

//////////// Callbacks used by the local clients ///////////////
void VRPN_CALLBACK button_press(void *, const vrpn_BUTTONCB b)
{
	fprintf(stderr, "Button_Server_Remote: button %i state %i\n", b.button, b.state);
}

void VRPN_CALLBACK handle_analog (void *userdata, const vrpn_ANALOGCB a)
{
    vrpn_int32 i;
    const char *name = (const char *)userdata;

    fprintf(stderr, "Analog:\n         %5.2f", a.channel[0]);
    for (i = 1; i < a.num_channel; i++) {
	fprintf(stderr, ", %5.2f\n", a.channel[i]);
    }
    fprintf(stderr, " (%d chans)\n", a.num_channel);
}
////////////////////////////////////////////////////////////////


vrpn_Android_Server::vrpn_Android_Server(vrpn_int32 num_analogs, vrpn_int32 * analog_sizes, vrpn_int32 num_buttons, vrpn_int32 port)
{
	fprintf(stderr, "C++: vrpn_Android_Server instantiated\n");
	this->port = port;
	this->initialize(num_analogs, analog_sizes, num_buttons);
}

void vrpn_Android_Server::initialize(vrpn_int32 num_analogs, vrpn_int32 * analog_sizes, vrpn_int32 num_buttons)
{
	ANALOG_SERVER_NAME = "Analog"; // Numbers added automatically
	BUTTON_SERVER_NAME = "Button0";
	
	this->num_analogs = num_analogs;
	this->analog_sizes = analog_sizes;
	
	fprintf(stderr, "vrpn_Android_Server: using port %i\n", port);

	fprintf(stderr, "vrpn_Android_Server: getting server connection...\n");
	connection = vrpn_create_server_connection(port);
	fprintf(stderr, (connection == 0) ? "vrpn_Android_Server: Failed to get connection\n" : "vrpn_Android_Server: Got connection\n");
	
	// Create analog servers
	analog_server = new vrpn_Analog_Server*[num_analogs];
	
	for (int i = 0; i < num_analogs; i++)
	{
		std::stringstream name;
		name << ANALOG_SERVER_NAME << i;
		analog_server[i] = new vrpn_Analog_Server(name.str().c_str(), connection, analog_sizes[i]);
		analog_server[i]->setNumChannels(analog_sizes[i]);
		fprintf(stderr, "vrpn_Android_Server: instantiated analog server %i\n", i);
	}
	
	// Create button server
	button_server = new vrpn_Button_Server(BUTTON_SERVER_NAME, connection, num_buttons);
	fprintf(stderr, "vrpn_Android_Server: instantiated button server\n");

	// Create analog clients
	analog_client = new vrpn_Analog_Remote*[num_analogs];
	
	for (int i = 0; i < num_analogs; i++)
	{
		std::stringstream name;
		name << ANALOG_SERVER_NAME << i;
		analog_client[i] = new vrpn_Analog_Remote(name.str().c_str(), connection);
		analog_client[i]->register_change_handler(const_cast <char*> (name.str().c_str()), handle_analog);
		
	}
	
	// Create button client
	button_client = new vrpn_Button_Remote(BUTTON_SERVER_NAME, connection);
	button_client->register_change_handler(const_cast <char*> (BUTTON_SERVER_NAME), button_press);

	fprintf(stderr, "C++: vrpn_Android_Server initialized\n");
}

void vrpn_Android_Server::mainloop()
{
	for (vrpn_int32 i = 0; i < num_analogs; i++)
	{
		analog_server[i]->mainloop();
		analog_client[i]->mainloop();
	}
	button_server->mainloop();
	button_client->mainloop();
	connection->mainloop();
}

void vrpn_Android_Server::set_button(vrpn_int32 button_id, vrpn_int32 state)
{
	button_server->set_button(button_id, state);
}

void vrpn_Android_Server::set_analog(vrpn_int32 analog_id, vrpn_int32 channel, vrpn_float64 val)
{
	vrpn_float64 * channels = analog_server[analog_id]->channels();
	channels[channel] = val;
}

void vrpn_Android_Server::report_analog_chg(vrpn_int32 analog_id)
{
	analog_server[analog_id]->report();
}

vrpn_Android_Server::~vrpn_Android_Server()
{
	for (vrpn_int32 i = 0; i < num_analogs; i++)
	{
		delete analog_server[i];
		delete analog_client[i];
	}
	delete button_server;
	delete button_client;
	delete connection;
	fprintf(stderr, "C++: vrpn_Android_Server destroyed\n");
}
