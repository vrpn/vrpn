/**************************************************************************************/
// vrpn_Router_Sierra_Server.h
/**************************************************************************************/

#ifndef VRPN_ROUTER_SIERRA_SERVER
#define VRPN_ROUTER_SIERRA_SERVER

#include "vrpn_Router.h"

/**************************************************************************************/
// Server for Sierra router.
// Does not use VRPN_API because it is an app-specified class, not a library class.
class vrpn_Router_Sierra_Server: public vrpn_Router {
public:
	vrpn_Router_Sierra_Server(const char * name, vrpn_Connection * c );
	virtual void mainloop();

protected:
	// The timestamp field within the parent structure is used for timing

	static int VRPN_CALLBACK vrpn_handle_set_link_message(void *userdata, vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK vrpn_handle_set_named_link_message(void *userdata, vrpn_HANDLERPARAM p);
};


/**************************************************************************************/
// GLOBAL VARIABLES
extern vrpn_Synchronized_Connection	c;			// VRPN connection
extern vrpn_Synchronized_Connection *	connection;		// ptr to VRPN connection
extern vrpn_Router *			router;			// ptr to video router server

extern const int minInputChannel;	// lowest channel #
extern const int maxInputChannel;	// one past highest channel #
extern const int minOutputChannel;	// lowest channel #
extern const int maxOutputChannel;	// one past highest channel #
extern const int minLevel;		// lowest level #
extern const int maxLevel;		// one past highest level #



/**************************************************************************************/
// FUNCTION PROTOYPES for the Server for the Sierra Video Router
int	main (unsigned argc, char *argv[]);
void	router_Sierra_SetState( int inputChannel, int outputChannel, int level );
int	streq( char* str1, char* str2 );
void	searchNameTables( char* name, int* pMatch, 
			int* pChannelType, int* pChannelNum, int* pLevelNum );
int	parseSetNamedLinkCommand( char* commandString, 
				int* input_ch, int* output_ch, int* level );
unsigned long	duration(struct timeval t1, struct timeval t2);
void	router_setInputChannelName(  int levelNum, 
				int channelNum, const char* channelNameCStr );
void	router_setOutputChannelName( int levelNum, 
				int channelNum, const char* channelNameCStr );

void	openSierraHardwareSerialPort( void );
void	queryStatusOfSierraHardwareViaSerialPort( void );
void	receiveStatusOfSierraHardwareViaSerialPort( void );
void	setLinkInSierraHardwareViaSerialPort( int outputIndex, int inputIndex, int level );


#endif // VRPN_ROUTER_SIERRA_SERVER
