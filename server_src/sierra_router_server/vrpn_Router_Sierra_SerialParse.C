/**************************************************************************************/
// vrpn_Router_Sierra_SerialParse.C
/**************************************************************************************/
#include <string>
#include <iostream>
//#include <fstream>
#include <sstream>	
using namespace std;


//#include "vrpn_Router_Sierra_Server.h"
extern void	router_Sierra_SetState( int inputChannel, int outputChannel, int level );
extern const int minInputChannel;
extern const int maxInputChannel;
extern const int minOutputChannel;
extern const int maxOutputChannel;
extern const int minLevel;
extern const int maxLevel;


/**************************************************************************************/
// Parse string received by the server from the router hardware.
// These strings are replies to commands send down the serial line by the server.
// See http://www.sierravideo.com/generic.html for command format for the Sierra router. 
// The information of interest to us is contained in a series of phrases
// of the form "X01,02,03" separated by spaces.  
// "X01,02,03" means Output #1 is linked to Input #2 on Level #3.
// When a command to set a link is sent down to the hardware, a single
// X-phrase is sent back as confirmation.
// When a query-status command is sent down, 160 X-phrases are sent back,
// one for each output channel.  
void
router_Sierra_ParseReply( string &reply )
{
	// The phrases are separated by a single space, and
	// have no internal spaces.
	istringstream ireply( reply );
	string phrase;
	while( ireply >> phrase ) {
			//		cout << phrase << "...";
			//		if( phrase == "**")   cout << "START\n";
			//		if( phrase == "OK")   cout << "OK\n";
			//		if( phrase == "!")    cout << "END\n\n";

		// Parse an X-phrase.
		if( phrase[0] == 'X' ) {
				//	cout << '.';

			istringstream iphrase( phrase );
			char letter, c1, c2;
			int  inputChannel, outputChannel, level;
			iphrase >> letter >> outputChannel >> c1 >> inputChannel >> c2 >> level;
				//cout << outputChannel << " " << inputChannel << " " << level << "\n";
			if(			letter == 'X'  &&
						c1     == ','  &&
						c2     == ','  &&
						outputChannel >= minOutputChannel  &&
						outputChannel <  maxOutputChannel  &&
						inputChannel  >= minInputChannel   &&
						inputChannel  <  maxInputChannel   &&
						level         >= minLevel          &&
						level         <  maxLevel      ) {
				// Update the server's model of the router hardware's state
				// based on the information in the phrase just parsed.
				router_Sierra_SetState( inputChannel, outputChannel, level );
			}
			else {
				cout << "Error parsing X-phrase" << phrase << "\n";
			}

		}
		else if( phrase == "**")    ;
		else if( phrase == "OK")    ;
		else if( phrase == "!")     ;
		else                        cout << "\nUnknown phrase: " << phrase << "\n";
	}
}


/**************************************************************************************/
// Receive characters (one at a time) transmitted by the Sierra video router
// in response to commands sent by the server program running on the
// associated PC (currently DC-1-CS).  
// This routine assembles received characters into a complete response string.
// A response string from the Sierra router begins with '*'
// and ends with '!'.  A simple 2-state finite state machine is used
// to sync with the start of a reponse (SYNC state: waiting for '*')
// and to collect the characters of a response block (COLLECT state:
// collecting received chars into a string and waiting for '!').
// When a complete response has been received, process it.

// See discussion in vrpn_Router_Sierra_Server::mainloop() regarding
// how many chars come back from the queryStatusOfSierraHardwareViaSerialPort()
// command (1600) and how long this takes (1.6 sec).
void
router_Sierra_ReceiveChar( char c )
{
	enum receiveStateEnum {SYNC, COLLECT};
	static receiveStateEnum receiveState = SYNC;
	static string receivedString = "";


	// Append received char onto receivedString.
	receivedString += c;

	switch( receiveState ) {
	case SYNC:
		// wait for start ('*') of status block from router
		if( c == '*' )   receiveState = COLLECT;
		else             receivedString = "";
		break;

	case COLLECT:
		// keep appending received chars to string.
		// when end of status block detected ('!'), parse status block
		if( c == '!' ) {
			// Parse receivedString and take action as needed.
			router_Sierra_ParseReply( receivedString );
				//cout << "\n\nGOT IT: " << receivedString << "\n\n";

			// Clear received string and switch to SYNC state for next status block
			receivedString = "";
			receiveState = SYNC;
		}
		break;
	}
}


