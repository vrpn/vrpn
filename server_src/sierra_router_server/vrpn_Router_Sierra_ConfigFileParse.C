/**************************************************************************************/
/**************************************************************************************/
// vrpn_Router_Sierra_ConfigFileParse.C  
/**************************************************************************************/
/**************************************************************************************/
// INCLUDE FILES
#ifdef _WIN32
  #include <iostream>
  #include <fstream>
  #include <strstream>	
  using namespace std;
#else
  #include <iostream.h>
  #include <fstream.h>
  #include <strstream.h>	// sstream (newest) vs strstream: sstream not supported by cygwin
#endif

#include <string>
#include <vector>
using namespace std;


//#include "vrpn_Router_Sierra_Server.h"
extern void	router_setInputChannelName(  int levelNum, int channelNum, const char* channelNameCStr );
extern void	router_setOutputChannelName( int levelNum, int channelNum, const char* channelNameCStr );
extern const int minInputChannel;
extern const int maxInputChannel;
extern const int minOutputChannel;
extern const int maxOutputChannel;
extern const int minLevel;
extern const int maxLevel;


/**************************************************************************************/
// Take the information extracted by parsing the config file
// regarding channel names and associated physical channel numbers
// and enter this into the server's name table arrays:
// input_channel_name[][] and output_channel_name[][].  
void 
installChannelName( int levelNum, string &channelType, int channelNum, 
				   string channelName )
{
//	cout << "Received: " << levelNum << " " << channelType << " " 
//		 << channelNum << " " << channelName << endl;

	if(     levelNum < minLevel  ||  
			levelNum >= maxLevel ) { 
		cout << "Bad level #." << endl; 
	}

	else if( channelType != "input"  &&
			 channelType != "output" ) {
		cout << "Bad channel type." << endl;
	}

	else if( channelType == "input"  &&  (
			 channelNum <  minInputChannel  ||  
			 channelNum >= maxInputChannel 
				                         )   ) {   
		cout << "Bad input channel #.";
	}

	else if( channelType == "output"  &&  (
			 channelNum <  minOutputChannel  ||  
			 channelNum >= maxOutputChannel 
				                         )   ) {   
		cout << "Bad output channel #.";
	}

//		else if( channelName.length() == 0 ) {
//			cout << "Channel name length is zero." << endl;
//		}

	else {
//			cout << "Accepting: " << levelNum << " " << channelType << " " 
//				<< channelNum << " " << channelName << endl;
//			cout << endl;


		const char * name_c_str = channelName.c_str();
		if( channelType == "input" ) { 
			router_setInputChannelName( levelNum, channelNum, name_c_str );
		}
		if( channelType == "output" ) { 
			router_setOutputChannelName( levelNum, channelNum, name_c_str );
		}


	}

}


/**************************************************************************************/
// Parse the configuration file "router.cfg".
// This file specifies the name strings of the router's input and output channels.
// The config file lives in the same directory as the router server executable.  
void 
readAndParseRouterConfigFile( void )
{
	// Specify name of router configuration file.
	// This file contains the names of the input and output channels
	// which are used by user code to refer to the channels
	// (as opposed to using physical channel numbers).
	string configFileName = "router.cfg";

	// Open config file.
	ifstream inputFile( configFileName.c_str() );
	if( !inputFile ) {
		cout << "Can't open congifuration file \"" << configFileName << "\"" << endl;
		return;
	}
	else {
//		cout << "Opened congifuration file \"" << configFileName << "\"" << endl;  
	}
	
	// Read one line at a time until end of file.
	int lineNum = 0;
	while( ! inputFile.eof() ) {
		// Count lines (used in error messages).
		lineNum++;
		
		// Get one newline-terminated (or EOF-terminated) line from file.
		string line;
		getline( inputFile, line, '\n' );
				//cout << "line:  " << line << endl;

		// Skip blank lines and comment lines (which start with '#').
		if( line == "" )       continue;
		if( line[0] == '#' )   continue;

		// Treat the line as a stream: parse into whitespace-separated words.
		istrstream lineStream( line.c_str() );

		// Parse lines of the form:
		//     level   <#>   input    <#>   <name>
		//     level   <#>   output   <#>   <name>
		string levelLiteral, channelType, channelName;
		int levelNum = -1;
		int channelNum = -1;

		// Get "level" literal from input line.
		lineStream >> levelLiteral;
		if( levelLiteral != "level" )   {cout << "Missing keyword: "; goto parseError;}


		// Get level number.
		if( lineStream.eof() )          {cout << "Missing level #: "; goto parseError;}
		lineStream >> levelNum;
		if( levelNum < minLevel  ||  
			levelNum >= maxLevel )          
										{cout << "Level # out of range: "; goto parseError;}

		// Get channel type ("input" or "output").
		if( lineStream.eof() )          {cout << "Missing channel type: "; goto parseError;}
		lineStream >> channelType;
		if( channelType != "input"  &&  channelType != "output" )          
										{cout << "Channel type wrong: "; goto parseError;}

		// Get channel number.
		if( lineStream.eof() )          {cout << "Missing channel #: "; goto parseError;}
		lineStream >> channelNum;
		if( channelType == "input" ) {
			if( channelNum <  minInputChannel  ||  
				channelNum >= maxInputChannel )          
										{cout << "Channel # out of range: "; goto parseError;}
		}
		else if( channelType == "output" ) {
			if( channelNum <  minOutputChannel  ||  
				channelNum >= maxOutputChannel )          
										{cout << "Channel # out of range: "; goto parseError;}
		}

		// Get channel name.
		if( lineStream.eof() )          {channelName = "";}
		lineStream >> channelName;
//		if( channelName.length() == 0 )   cout << "Warning: zero-length name." << endl;
		if( channelName.length() > 255 )  {cout << "Name too long: "; goto parseError;}


//		cout << "Parsed: " << levelNum << " " << channelNum << " " << channelName << endl;

		// Associate a name with a physical channel number and level
		// in the name data tables.  
		installChannelName( levelNum, channelType, channelNum, channelName );
		
		
		continue;


	parseError:
		char quote = '"';
		cout << endl;
		cout << "Parse error in configuration file " << quote << configFileName << quote
				    << " at line #" << lineNum << ":" << endl;
		cout << "    " << line << endl;
		cout << "Parsing of configuration file terminated." << endl;
		return;
	}
	cout << "Parsed congifuration file \"" << configFileName << "\"" << endl;  
	return;
}




