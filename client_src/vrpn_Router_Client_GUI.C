/**************************************************************************************/
/**************************************************************************************/
// Client for Sierra Video Switch Server  
// Warren Robinett
// Computer Science Dept.
// University of North Carolina at Chapel Hill
// Jan 2000 -- initial version
// Jul 2000 -- integrated with VRPN 5.0, allow multiple simultaneous clients

/**************************************************************************************/
/**************************************************************************************/
// INCLUDE FILES
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>

#if defined(_WIN32) && !defined(GLUT)
#define GLUT
#endif
#include <GL/glut.h>


#include "vrpn_Router.h"



/**************************************************************************************/
/**************************************************************************************/
// TYPEDEFS


/**************************************************************************************/
/**************************************************************************************/
// FUNCTION PROTOYPES
void	error( char* errMsg );
void	assert( int condition, char* msg );
int		streq( char* str1, char* str2 );
void	showBitmapString( char* str );
void	showUI( void );
void	commonKeyboardFunc(unsigned char key, int x, int y);
void	displayFuncUI( void );
void	idleFuncUI( void );
int		main(     int argc, char *argv[]);
int		realMain( int argc, char *argv[]);
void	shutdown( void );
void	vrpn_Router_Remote_SetLink( int input, int output, int level );
void	sighandler (int signal);
void	install_signal_handler (void);
void	vrpn_Router_Remote_Init( void );
void	usage( void );
void	mouseFuncUI( int button, int state, int x, int y );
void	mouseMotionFuncUI( int x, int y );
void	reshapeWindowFuncUI( int newWindowWidth, int newWindowHeight );
void	calcMouseWorldLoc( int xMouse, int yMouse );
void	grabChannel( void );
void	attemptSetLink( void );
void	findNearestItem( void );
double	L1Distance( double x1, double y1, double x2, double y2 );
void	setMaterialColor( GLfloat r, GLfloat g, GLfloat b );
void	setColor( int colorIndex );
void	drawLine( double x1, double y1, double x2, double y2 );
void	drawRectangle( double width, double height );
double	xOutputPosition( int output );
double	yOutputPosition( int output );
double	xInputPosition(  int input );
double	yInputPosition(  int input );
void	printInputOutputLevel( int input_ch, int output_ch, int level );
void	handle_router_change_message( void *, const vrpn_ROUTERCB     routerData );
void	handle_router_name_message(   void *, const vrpn_ROUTERNAMECB routerData );
void	readAndParseRouterConfigFile( void );
void	showUITitle( void );
void	showRubberBandLine( void );
void	showInputs( void );
void	showOutputs( void );
void	showLinkLines( void );
void	showLevelButtons( void );
void	clearJustLinkedLine( void );
//void	setInputChannelName(  int levelNum, int channelNum, const char* channelNameCStr );
//void	setOutputChannelName( int levelNum, int channelNum, const char* channelNameCStr );


/**************************************************************************************/
/**************************************************************************************/
// #DEFINES
// colors
#define WHITE	0
#define RED		1
#define GREEN	2
#define BLUE	3
#define MAGENTA	4
#define YELLOW	5
#define CYAN	6
#define BLACK	7
#define GRAY	8

#define WINDOW_SIZE_TINY	1
#define WINDOW_SIZE_SMALL	2
#define WINDOW_SIZE_MEDIUM	3
#define WINDOW_SIZE_BIG		4

/**************************************************************************************/
/**************************************************************************************/
// GLOBAL VARIABLES
// mouse and cursor, window
int UIWindowID;
double UIwindowWidth  = 0.;
double UIwindowHeight = 0.;
double charWidthInWorld = 0;
double charHeightInWorld = 0;
double xMouseInWorld;	// mouse position in world coords
double yMouseInWorld;	// mouse position in world coords
int dragInProgress = 0;
int setLinkCommandInProgress = 0;
int currentColor = BLACK;
int windowSize = WINDOW_SIZE_MEDIUM;

// Video router stuff
//char routerName[100] = "Router0@Tin-CS"; 
char routerName[100] = "Router0@DC-1-CS";  // This is the PC in SN272, the video switch room
vrpn_Router_Remote *router;


const int minInputChannel  = vrpn_INPUT_CHANNEL_MIN;	// lowest channel #
const int maxInputChannel  = vrpn_INPUT_CHANNEL_MAX;	// one past highest channel #
const int minOutputChannel = vrpn_OUTPUT_CHANNEL_MIN;	// lowest channel #
const int maxOutputChannel = vrpn_OUTPUT_CHANNEL_MAX;	// one past highest channel #
const int minLevel         = vrpn_LEVEL_MIN;			// lowest level #
const int maxLevel         = vrpn_LEVEL_MAX;			// one past highest level #
int currentLevel = 1;

#define NULL_CHANNEL (-1)
int selectedInput  = NULL_CHANNEL;
int selectedOutput = NULL_CHANNEL;
int justLinkedInput  = NULL_CHANNEL;
int justLinkedOutput = NULL_CHANNEL;



/**************************************************************************************/
// MAIN AND TOP-LEVEL ROUTINES

/**************************************************************************************/
// Main routine.
int
main( int argc, char *argv[])
{
	// Set up a signal handler to shut down cleanly on control-C.
	install_signal_handler();


	// Init video router.  Connect to video switch server. 
	vrpn_Router_Remote_Init();
		// VRPN 5.0 allows multiple clients to communicate with a single server.
		// So multiple clients for the video switch server may be active at one time.
		// These different clients may or may not have conflicting needs for
		// video channels.  Often they will not.  If video channel conflicts
		// become a problem, we can implement a locking mechanism using 
		// the Mutex primitive already implemented in VRPN.  

	// Deal with command line args to GLUT.
	glutInit(&argc, argv);
	static int dblBuf  = GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH;
	glutInitDisplayMode(dblBuf);


	// UI WINDOW (text)
	glutInitWindowSize( 600, 600 );
	glutInitWindowPosition( 0, 0 );
	UIWindowID = glutCreateWindow( "Video Router Client" );
	// set up window's callback routines
	glutDisplayFunc( displayFuncUI);
	glutIdleFunc(                idleFuncUI );
	glutKeyboardFunc(        commonKeyboardFunc );
	glutMouseFunc(    mouseFuncUI );
	glutMotionFunc(   mouseMotionFuncUI );
	glutReshapeFunc(  reshapeWindowFuncUI );


	// Other inits.
	//Print usage message at start-up.
	usage();


	// app's main loop, from which callbacks to above routines occur
	glutMainLoop();

	return 0;               /* ANSI C requires main to return int. */
}


/**************************************************************************************/
// This idle function marks both windows for redisplay, which will cause
// their display callbacks to be invoked.
void
idleFuncUI( void )
{
	// Call the callback funtions, as needed. 
	if( router )      router->mainloop();


	// sleep during top level so as not to eat 100% of CPU.  
	vrpn_SleepMsecs( 10 );


	// When client is starting up, request that the server send the client
	// info on the state of the router (for display to the user)
	// and also the channel name strings.  
	static int firstTime = 1;
	if( firstTime ) {
		firstTime = 0;

		// Request router state be sent to client.
		router->query_complete_state();
	}



	glutSetWindow( UIWindowID );		
	glutPostRedisplay();
}


/**************************************************************************************/
/**************************************************************************************/
// KEYBOARD

/**************************************************************************************/
// Keyboard callback for main window.
void
commonKeyboardFunc(unsigned char key, int x, int y)
{
	static char str[256] = "";
	static char garbageString[256] = "";
	int end;
	int numFound;
	int input;
	int output;
	
	// To respond to non-ASCII (special) key keypresses, see
		// http://reality.sgi.com/opengl/spec3/node54.html#1044
		// Section 7.9 (glutSpecialFunc) -- uses eg, GLUT_KEY_F1 
	x=x; y=y;


	// echo typed chars to screen.  
	printf( "%c", key );
	

	switch (key) {

//	case 'a':	router->set_router( "coons_ch1", "hmdlab_monitor_left" );	break;
//	case 'b':	router->set_router( "coons_ch2", "hmdlab_monitor_left" );	break;

	case 'u':	// request update
		router->query_complete_state();
		break;


	case '?':
	case 'h':
		usage();
		break;


	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case ' ':
		// add latest typed char to string being accululated
		end = strlen( str );
		str[ end   ] = key;
		str[ end+1 ] = 0;

		// echo typed chars to screen.  
//		printf( "%c", key );
		break;

	case '\r':  // Carriage return character  (ENTER)
				//printf( "str:%s\n", str );

		// Parse the input string and execute command (to set video router).
		numFound = sscanf( str, "%d %d %s", &input, &output, &garbageString );
		if( numFound == 2 ) {
				//printf( "\nParse successful.\n" );

			// Hilight the selected input and output on the GUI.
			selectedOutput = output;
			selectedInput  = input;

			// Send command to video router link the selected input channel
			// to the selected output channel.
			vrpn_Router_Remote_SetLink( selectedInput, selectedOutput, currentLevel );

			printf( "\n" );  // echo the carriage return
		}
		else {
			// Parse failed, so print usage message.
			printf( "\nUsage: <input channel> <output channel> ENTER\n" );
		}

		// clear input string for next input line
		str[0] = 0;
		break;


	// Exit the program.
	case 'q':	// quit
	case 27:	// Esc
	case  3:	// Ctl-C
		// Take care of any clean-up before shutting down app.
		shutdown();
		
		// Exit program.  
		exit(0);
		break;

	// Deal with unexpected chars.
	default:
		printf( " Unexpected character, discarding line.\n" );
		str[0] = 0;		// clear input string
		break;
	}

	glutPostRedisplay();	// in case something was changed
}


/************************************************************************************/
void
usage( void )
{
		printf( "\n" );
		printf( "Usage: <input channel> <output channel> ENTER\n" );
		printf( "    to set link in video router.\n" );
		printf( "Example: 1 2 ENTER\n" );
		printf( "    to route Input Channel #1 to Output Channel #2.\n" );
		printf( "\n" );
}


/**************************************************************************************/
/**************************************************************************************/
// VIDEO ROUTER STUFF

/**************************************************************************************/
// Constructor for router client.  
// Open the router client for the named router server.
// Register VRPN message types that the client understands.
void vrpn_Router_Remote_Init( void )
{       
	if( streq( routerName, "" ) ) {
		printf( "No router server specified.\n" );
		router = NULL;
	}
	else {
		// Open the named router.
		printf( "Attempting to open vrpn_Router_Remote for \"%s\"\n", routerName );
		router = new vrpn_Router_Remote( routerName );

		// Register handler routine for change messages from the router server.
		router->register_change_handler( NULL, handle_router_change_message );
			// Registered callback routines are called during the client's
			// mainloop routine (actually by d_connection->mainloop)
			// when a message of the appropriate type has been received.

		// Register handler routine for name messages from the router server.
		router->register_name_handler( NULL, handle_router_name_message );
	}
}


/**************************************************************************************/
// Update client's name table with info received in name message.
void handle_router_name_message( void *, const vrpn_ROUTERNAMECB cp )
{       
	// Unpack info sent in the "name message" from the server.
 	int channelType  = cp.channelType;
	int channelNum   = cp.channelNum;
	int levelNum     = cp.levelNum;
	int nameLength   = cp.nameLength;
	const char* name = &(cp.name[0]);


	// Update client's name table with info received in name message.
	if( channelType == vrpn_CHANNEL_TYPE_INPUT ) {
		router->setInputChannelName(  levelNum, channelNum, name );
	}
	else if( channelType == vrpn_CHANNEL_TYPE_OUTPUT ) {
		router->setOutputChannelName( levelNum, channelNum, name );
	}
	else {
		printf( "Error: unknown channelType: %d.\n", channelType );
	}


	printf( "*" );
		//printf( "Received name message: %s\n", name );
}


/**************************************************************************************/
// This callback function is invoked on the client side in response to
// the function "report_channel_status" being called on the server side.  
// The purpose is to send info on the status of one output channel to the client.
// The info sent is: the output's level and channel # and the 
// input channel that is linked to it.
// Example call on server side: 
//		report_channel_status(input_ch, output_ch, level);
void handle_router_change_message( void *, const vrpn_ROUTERCB routerData )
{       
	// Unpack info sent in the "change message" from the server.
 	int input_ch  = routerData.input_channel;
	int output_ch = routerData.output_channel;
	int level     = routerData.level;

	// Has the channel changed?
	int channelChangedFlag = 
		( router->router_state[ output_ch ][ level ] != vrpn_UNKNOWN_CHANNEL )  &&    
		( router->router_state[ output_ch ][ level ] != input_ch );

	// Update client's model of router state with info received in change message.
	router->router_state[ output_ch ][ level ] = input_ch;
				// Print the state of the video router on the client side
				// in response to change messages from the server.
				//	if( (routerData.output_channel % 8) == 0 )   printf( "\n" );
				//	if( routerData.output_channel == vrpn_UNKNOWN_CHANNEL )   printf( "\nVideo Router State:\n" );
				//	printf( "#%2d:%2d   ", routerData.output_channel, routerData.input_channel );
				//	if( routerData.output_channel == maxOutputChannel-1 )  printf( "\n\n" );

	
	// Print to this client's console window when changes occur
	// (initiated by this client, other clients, 
	// or manually at the Sierra router console).
	// Print only if this information was not already known to the client.
	// (Another client may request a full state report.)
	if( channelChangedFlag ) {
		printInputOutputLevel( input_ch, output_ch, level );
	}
	else {
		printf( "." );
	}


	// Make the GUI's just-linked (red) line revert to not just-linked (ie, white)
	// if a more recemt change has occurred.
	if(		justLinkedInput  != NULL_CHANNEL  &&
			justLinkedOutput != NULL_CHANNEL  &&
			level == currentLevel  &&
			(input_ch != justLinkedInput  ||  output_ch != justLinkedOutput)  ) {
		// Clear the previous just-made link, since a more recent change
		// has occurred to the router state (initiated elsewhere).
		clearJustLinkedLine();
	}
}


/**************************************************************************************/
void
printInputOutputLevel( int input_ch, int output_ch, int level )
{
	printf( "<time>  Level %2d: input #%2d routed to output #%2d\n", 
					level, input_ch, output_ch );
}


/**************************************************************************************/
// Send a command across the LAN to a VRPN server to set the Sierra
// video router to a new setting: 
// Route video channel # "input_ch" to video channel # "output_ch".  
void
vrpn_Router_Remote_SetLink( int input_ch, int output_ch, int level )
{

	// Check the ranges of input and output arguments.
	if( input_ch < minInputChannel  ||  input_ch >= maxInputChannel ) {
		printf(	"\nError: input channel (%d) out of range [%d,%d]\n", 
				input_ch, minInputChannel, maxInputChannel-1 );
		return;
	}
	else if( output_ch < minOutputChannel  ||  output_ch >= maxOutputChannel ) {
		printf(	"\nError: output channel (%d) out of range [%d,%d]\n", 
				output_ch, minOutputChannel, maxOutputChannel-1 );
		return;
	}
	else if( level < minLevel  ||  level >= maxLevel ) {
		printf(	"\nError: level (%d) out of range [%d,%d]\n", 
				level, minLevel, maxLevel-1 );
		return;
	}
	else {
			  //printf( "   Attempting to set link..." );
	}


	// Send command across VRPN connection to link "input_ch" to "output_ch"
	// on level "level" in the Sierra video router.
	router->set_link( output_ch, input_ch, level );


	// Set variable to indicate a pending
	// SetLink command, shown as a red dotted line in the GUI.
	// When confirmation is received from the server, this var is cleared.
	// Save input and output channel numbers of just-made connection
	// for display in the GUI.
	setLinkCommandInProgress = 1;
	justLinkedInput  = selectedInput;
	justLinkedOutput = selectedOutput;

	// Leave just the output channel selected.  
	// This will highlight the output channel name
	// and the name of the linked input channel.
	selectedInput    = NULL_CHANNEL;
}


/************************************************************************************/
// Install the signal handler.  
void install_signal_handler (void) 
{

#ifndef WIN32
#ifdef sgi

  sigset( SIGINT, sighandler );
  sigset( SIGKILL, sighandler );
  sigset( SIGTERM, sighandler );
  sigset( SIGPIPE, sighandler );

#else

  signal( SIGINT, sighandler );
  signal( SIGKILL, sighandler );
  signal( SIGTERM, sighandler );
  signal( SIGPIPE, sighandler );

#endif  // sgi
#endif  // WIN32

}


/************************************************************************************/
// Catch control-C and shut down our network connections nicely.
void sighandler (int signal) 
{
	// Do cleanup prior to exiting.
	shutdown();
	
	// Exit the program. 
	exit(0);
}


/************************************************************************************/
// Stuff to do before quitting the app.
void
shutdown( void )
{
	printf( "Quitting app...\n" );
}


/**************************************************************************************/
/**************************************************************************************/
// GUI WINDOW

/**************************************************************************************/
void
setMaterialColor( GLfloat r, GLfloat g, GLfloat b )
{
	glColor3f( r, g, b );
}


/**************************************************************************************/
void
setColor( int colorIndex )
{
	currentColor = colorIndex;

	switch( colorIndex ) {
	case WHITE:		setMaterialColor(1.0, 1.0, 1.0); break;
	case RED:		setMaterialColor(1.0, 0.0, 0.0); break;
	case GREEN:		setMaterialColor(0.0, 1.0, 0.0); break;
	case BLUE:		setMaterialColor(0.0, 0.0, 1.0); break;
	case MAGENTA:	setMaterialColor(1.0, 0.0, 1.0); break;
	case YELLOW:	setMaterialColor(1.0, 1.0, 0.0); break;
	case CYAN:		setMaterialColor(0.0, 1.0, 1.0); break;
	case BLACK:		setMaterialColor(0.0, 0.0, 0.0); break;
	case GRAY:		setMaterialColor(0.5, 0.5, 0.5); break;
	}
}


/**************************************************************************************/
void
drawLine( double x1, double y1, double x2, double y2 )
{
	// draw line between the two points
	glBegin( GL_LINE_STRIP );  // see p43 Woo 3rd ed
	glVertex3f( x1, y1,  0. );
	glVertex3f( x2, y2,  0. );
	glEnd();
}


/**************************************************************************************/
void
drawRectangle( double width, double height )
{
	glPushMatrix();

	glBegin( GL_POLYGON );

	glVertex3f(-width/2., -height/2.,  0.);
	glVertex3f( width/2., -height/2.,  0.);
	glVertex3f( width/2.,  height/2.,  0.);
	glVertex3f(-width/2.,  height/2.,  0.);

	glEnd();

	glPopMatrix();
}


/**************************************************************************************/
// Display graphics in the UI (User Interface) window.
void displayFuncUI( void ) 
{
	// Enable UI window.
	glutSetWindow( UIWindowID );

	// Setup OpenGL state.
	glShadeModel(GL_FLAT);
	glDisable( GL_LIGHTING );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_DEPTH_TEST);

	// Set projection matrix to orthoscopic projection 
	// with the view direction straight down the Z-axis looking at the XY plane,
	// with the unit square ([0,1] x [0,1])  filling the window.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(  0.,   1.,		// X range
		      0.,   1.,		// Y range
		     -1.,   1. );	// Z range

	// Set up modeling matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear window background and depth buffer.
	glClearColor(0.6f, 0.6f, 0.6f, 0.0);   // background color (medium gray)
	glClearDepth(1.0);

	// Display User Interface (UI) widgets.
	showUI();

	// end of display frame, so flip buffers
	glutSwapBuffers();
}


/**************************************************************************************/
double xOutputPosition( int output )   {return 0.60;}
double yOutputPosition( int output )   {return 0.88 + (-0.025 * output);}
double xInputPosition(  int input )    {return 0.40;}
double yInputPosition(  int input )    {return 0.88 + (-0.025 * input);}
double xButtonPosition( int b )	       {return 0.30 + (0.12 * b);}
double yButtonPosition( int b )        {return 0.95;}


/**************************************************************************************/
// Display the user interface (UI) graphical entities (lines, text, boxes, etc.).
void
showUI( void )
{
	showUITitle();	
	showLevelButtons();
	
	showInputs();
	showOutputs();
	showLinkLines();

	showRubberBandLine();
}


/**************************************************************************************/
void
showUITitle( void )
{
	char str[256];

	// count frames and display count (to show the client is alive)
	static int frameCount = 0;
	frameCount++;

	// Display title at top left for this client app.
	if( windowSize >= WINDOW_SIZE_BIG ) {
		setColor( WHITE );
		glPushMatrix();
		glTranslatef(   0.f, 0.975f, 0.f );
		sprintf( str, "Server: %s", routerName ); showBitmapString( str );
		sprintf( str, "Uptime: %5d frames.", frameCount ); showBitmapString( str );
		glPopMatrix();
	}
}


/**************************************************************************************/
void
clearJustLinkedLine( void )
{
	// Clear the previous just-made link.
	justLinkedInput  = NULL_CHANNEL;
	justLinkedOutput = NULL_CHANNEL;
}


/**************************************************************************************/
void
showRubberBandLine( void )
{
	// Check whether pending Setlink command has been confirmed successful by server.
	if(     setLinkCommandInProgress  &&
			selectedInput != NULL_CHANNEL  &&
		    selectedOutput != NULL_CHANNEL  &&
		    router->router_state[ selectedOutput ][ currentLevel ] == selectedInput ) {
		// SetLink command succeeded, so erase dotted red rubber band line.
		setLinkCommandInProgress = 0;
	}

	// Draw red dotted line between input and output channels for a 
	// pending SetLink command.
	glPushMatrix();
	glLineStipple( 1, 0xFF00 );		// stippled (dotted) line: see p53, Woo 3e
	glEnable( GL_LINE_STIPPLE );

	if(     setLinkCommandInProgress == 1  &&
			justLinkedInput  != NULL_CHANNEL  &&  
		    justLinkedOutput != NULL_CHANNEL    ) {
		setColor( RED );
		drawLine( 
			xInputPosition( justLinkedInput),  yInputPosition( justLinkedInput),
			xOutputPosition(justLinkedOutput), yOutputPosition(justLinkedOutput)   );
	}

	// Draw rubber band line from selected channel (if any) to mouse cursor
	// during drag operation.
	if( dragInProgress  &&  selectedInput != NULL_CHANNEL ) {
		setColor( YELLOW );
		drawLine( 
			xInputPosition(selectedInput), yInputPosition(selectedInput),
			xMouseInWorld, yMouseInWorld   );
	}
	else if( dragInProgress  &&  selectedOutput != NULL_CHANNEL ) {
		setColor( YELLOW );
		drawLine( 
			xOutputPosition(selectedOutput), yOutputPosition(selectedOutput),
			xMouseInWorld, yMouseInWorld   );
	}
	glDisable( GL_LINE_STIPPLE );
	glPopMatrix();
}


/**************************************************************************************/
void
showInputs( void )
{
	double iconWidth  = 0.03;
	double iconHeight = 0.02;
	char str[256];

	// Heading for column of inputs.
	if( windowSize >= WINDOW_SIZE_MEDIUM ) {
		setColor( YELLOW );
		char * inputHeading = "SOURCES";
		glPushMatrix();
		glTranslatef( xInputPosition(0) - (strlen(inputHeading) +2 ) * charWidthInWorld, 
					  yInputPosition(0) - 0.005, 
					  0. );
		sprintf( str, inputHeading ); 
		showBitmapString( str );
		glPopMatrix();
	}
		// Display column of input channel numbers and names.
	for( int input=minInputChannel; input<maxInputChannel; input++ ) {

		// Set color of input channel icons.
		setColor( BLUE );
		if( input == selectedInput)  setColor( WHITE );
		if( selectedOutput != NULL_CHANNEL ) {
			int linked_input = router->router_state[ selectedOutput ][ currentLevel ];
			if( input == linked_input)  setColor( WHITE );
		}
		if( input == justLinkedInput )  setColor( RED );

		// Display input channel icons (little squares).
		glPushMatrix();
		glTranslatef( xInputPosition(input), yInputPosition(input), 0. );
		drawRectangle( iconWidth, iconHeight );
		glPopMatrix();


		// Display input channel numbers.
		double xStartOffset;
		double yStartOffset;
		if( windowSize >= WINDOW_SIZE_BIG ) {
			if(      currentColor == WHITE )   setColor( BLACK );
			else if( currentColor == RED   )   setColor( WHITE );
			else                               setColor( BLACK );
			xStartOffset = - iconWidth/2.  + 0.0 * charWidthInWorld;
			yStartOffset = - iconHeight/2. + 0.2 * charHeightInWorld;
			glPushMatrix();
			glTranslatef( xInputPosition(input) + xStartOffset, 
						  yInputPosition(input) + yStartOffset, 
						  0.1f );
			sprintf( str, "%2d", input ); 
			showBitmapString( str );
			glPopMatrix();
		}

		
		// Set color of input channel names.
		setColor( BLACK );
		if( input == selectedInput)  setColor( WHITE );
		if( selectedOutput != NULL_CHANNEL ) {
			int linked_input = router->router_state[ selectedOutput ][ currentLevel ];
			if( input == linked_input)  setColor( WHITE );
		}

		// Display input channel names.
		char * inputChannelName = router->input_channel_name[ currentLevel ][ input ];
		int nameWidthInChars = strlen( inputChannelName );
		double nameWidthInWorld = nameWidthInChars * charWidthInWorld;
		xStartOffset = - iconWidth/2.  - 1.0 * charWidthInWorld;
		yStartOffset = - iconHeight/2. + 0.2 * charHeightInWorld;

		glPushMatrix();
		glTranslatef( xInputPosition(input) + xStartOffset - nameWidthInWorld, 
			          yInputPosition(input) + yStartOffset, 
					  0. );
		sprintf( str, "%s", inputChannelName ); 
		showBitmapString( str );
		glPopMatrix();
	}
}


/**************************************************************************************/
void
showOutputs( void )
{
	double iconWidth  = 0.03;
	double iconHeight = 0.02;
	char str[256];

	if( windowSize >= WINDOW_SIZE_MEDIUM ) {
		// Heading for column of inputs.
		setColor( YELLOW );
		glPushMatrix();
		glTranslatef( xOutputPosition(0) + 0.025, yOutputPosition(0) - 0.005, 0. );
		sprintf( str, "SINKS" ); 
		showBitmapString( str );
		glPopMatrix();
	}

	// Display column of output channel numbers and names.
	for( int output=minOutputChannel; output<maxOutputChannel; output++ ) {
		int linked_input = router->router_state[ output ][ currentLevel ];

		// Set color of output channel icons.
		setColor( BLUE );
		if( output       == selectedOutput) setColor( WHITE );
		if( linked_input == selectedInput)  setColor( WHITE );
		if( output == justLinkedOutput )  setColor( RED );

		// Display output channel icons (little squares).
		glPushMatrix();
		glTranslatef( xOutputPosition(output), yOutputPosition(output), 0. );
		drawRectangle( iconWidth, iconHeight );
		glPopMatrix();


		// Display output channel numbers.
		double xStartOffset;
		double yStartOffset;
		if( windowSize >= WINDOW_SIZE_BIG ) {
			if(      currentColor == WHITE )   setColor( BLACK );
			else if( currentColor == RED   )   setColor( WHITE );
			else                               setColor( BLACK );
			glPushMatrix();
			xStartOffset = - iconWidth/2.  + 0.0 * charWidthInWorld;
			yStartOffset = - iconHeight/2. + 0.2 * charHeightInWorld;
			glTranslatef( xOutputPosition(output) + xStartOffset, 
						  yOutputPosition(output) + yStartOffset, 
						  0.1f );
			sprintf( str, "%2d", output ); 
			showBitmapString( str );
			glPopMatrix();
		}

		// Set color of output channel names.
		setColor( BLACK );
		if( output       == selectedOutput) setColor( WHITE );
		if( linked_input == selectedInput)  setColor( WHITE );

		// Display output channel names.
		char * outputChannelName = router->output_channel_name[ currentLevel ][ output ];
		glPushMatrix();
		xStartOffset = + iconWidth/2.  + 1.0 * charWidthInWorld;
		yStartOffset = - iconHeight/2. + 0.2 * charHeightInWorld;
		glTranslatef( xOutputPosition(output) + xStartOffset, 
			          yOutputPosition(output) + yStartOffset, 
					  0. );
		sprintf( str, "%s", outputChannelName ); 
		showBitmapString( str );
		glPopMatrix();
	}
}


/**************************************************************************************/
void
showLinkLines( void )
{
	// Draw lines indicating which are routed to which outputs.
	// Loop through outputs because each output receives input from one input channel,
	// whereas an input can go to multiple output channels. 
	for( int output=minOutputChannel; output<maxOutputChannel; output++ ) {
		// Get the input channel routed to this output channel.
		int linked_input = router->router_state[ output ][ currentLevel ];

		// If it is unknown (to the client) what input is linked
		// to this output channel, then don't draw a line for this output channel.  
		if( linked_input == vrpn_UNKNOWN_CHANNEL )   continue;

		// Draw line from output to whatever input it is linked to.
		setColor( BLACK );
		if( output       == selectedOutput) setColor( WHITE );
		if( linked_input == selectedInput)  setColor( WHITE );
		if( output == justLinkedOutput  &&  linked_input == justLinkedInput ) setColor( RED );

		drawLine( 
			xInputPosition(linked_input), yInputPosition(linked_input),
			xOutputPosition(output), yOutputPosition(output)   );
	}
}


/**************************************************************************************/
void
showLevelButtons( void )
{
	char str[256];

	// Display level buttons.
	char levelName1[maxLevel][256] = {"", "RGB",  "NTSC", "Sound", "unused", "unused" };
	char levelName2[maxLevel][256] = {"", "video","video", "",     "sound",  "video" };
	for( int b=minLevel; b<maxLevel; b++ ) {
		glPushMatrix();
		glTranslatef( xButtonPosition(b), yButtonPosition(b), 0. );
		setColor( (b == currentLevel) ? RED : GRAY );
		drawRectangle( 0.11, 0.09 );
		glPopMatrix();

		setColor( (b == currentLevel) ? WHITE : BLACK );
		glPushMatrix();
		if( windowSize >= WINDOW_SIZE_BIG ) {
			glTranslatef( xButtonPosition(b) - 0.04, yButtonPosition(b) + 0.025, 0.1f );
			sprintf( str, "Level %d", b ); showBitmapString( str );
			sprintf( str, "%s", levelName1[b] ); showBitmapString( str );
			sprintf( str, "%s", levelName2[b] ); showBitmapString( str );
		}
		else {
			glTranslatef( xButtonPosition(b) - 0.00, yButtonPosition(b) + 0.00, 0.1f );
			sprintf( str, "%d", b ); showBitmapString( str );
		}
		glPopMatrix();
	}
}


/**************************************************************************************/
// draw char string as bitmap, starting at current origin. 
// String has constant size in pixels -- is not 3D object
// See p102 Angel.
void
showBitmapString( char* str )
{
	glRasterPos2f( 0., 0. );
	
	glPushMatrix();
	for( char* s = str; *s!=0; s++ ) {
		glRasterPos2f( 0., 0. );
		glTranslatef( charWidthInWorld, 0., 0. );
		glutBitmapCharacter( GLUT_BITMAP_8_BY_13, *s );
	}
	glPopMatrix();

	double textLineSeparation = - charHeightInWorld;
	glTranslatef( 0., textLineSeparation, 0. );
}


/**************************************************************************************/
/**************************************************************************************/
// UTILITY ROUTINES

/**************************************************************************************/
void
error( char* errMsg )
{
	printf( "\nError: %s\n", errMsg );
	exit( 1 );
}


/**************************************************************************************/
// if the condition is not true, invoke error routine with the message msg.
//	Sample call: assert( 0, "test" );
void
assert( int condition, char* msg )
{
	if( ! condition )   error( msg );
}


/**************************************************************************************/
// Test 2 character strings for equality.  
int
streq( char* str1, char* str2 )
{
	// strcmp does lexicographic comparison.  Result=0 means strings are the same.
	return   (strcmp( str1, str2 ) == 0);
}


/**************************************************************************************/
/**************************************************************************************/
// MOUSE AND CURSOR


/**************************************************************************************/
// Callback routine: called for mouse button events (pressed and/or released).
// button: GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, GLUT_MIDDLE_BUTTON
// state:  GLUT_UP (released), GLUT_DOWN (depressed)
// x,y:    cursor loc at mouse event in window coords
// see p658 Woo 3rd ed
void
mouseFuncUI( int button, int state, int x, int y )
{
	// Clear the previous just-made link.
	clearJustLinkedLine();

	// Convert mouse cursor location (in pixel units) to world coords.
	calcMouseWorldLoc( x, y );

	switch( button ) {
	case GLUT_LEFT_BUTTON: 
		// A down-click is assumed to be the beginning of a drag operation,
		// so try to select an input or output channel.
		if(      state == GLUT_DOWN ) {
			dragInProgress = 1;
			grabChannel();
		}

		// An up-click is assumed to be the end of a drag, so try to
		// select an input or output channel, and if we have one of each,
		// send a SetLink command to the video router to link them.
		else if( state == GLUT_UP ) {
			dragInProgress = 0;
			attemptSetLink();
		}
		break;
	case GLUT_RIGHT_BUTTON: 
		if(      state == GLUT_DOWN )	{}
		else if( state == GLUT_UP )		{}
		break;
	}

	glutPostRedisplay();
}


/**************************************************************************************/
// Try to use the mouse cursor location to select an input or output channel.
// Called on a left-mouse-button down-click (presumably the start of a drag operation).
void
grabChannel( void )
{
	// Clear selected channels. 
	selectedInput  = NULL_CHANNEL;
	selectedOutput = NULL_CHANNEL;

	// Use cursor location to attempt to select an input or output channel.
	// If one is selected, a rubber band line is drawn from the selected channel
	// to the mouse cursor while the drag is in progress.
	findNearestItem();
}


/**************************************************************************************/
// Try to use the mouse cursor location to select another input or output channel.
// Called on a left-mouse-button up-click (presumably the end of a drag operation).
void
attemptSetLink( void )
{
	// Use cursor location to attempt to select
	// an input or output channel.
	// (The idea was to drag an input to an output, or vice versa.)
	findNearestItem();

	// If an input channel and an output channel were both selected
	// (ie, the front end and back end of the drag both got a channel),
	// then link them.
	if(     selectedInput  != NULL_CHANNEL  &&
		    selectedOutput != NULL_CHANNEL  ) {
		// Send command to video router link the selected input channel
		// to the selected output channel.
		vrpn_Router_Remote_SetLink( selectedInput, selectedOutput, currentLevel );
	}
}


/**************************************************************************************/
// Calculate distance between two 2d points (x1,y1) and (x2,y2)
// using the L1 norm (diamond-shaped equi-distance contours).
double 
L1Distance( double x1, double y1, double x2, double y2 )
{
	return fabs(x1 - x2) + fabs(y1 - y2);
}


/**************************************************************************************/
// For both the input channel icons and output channel icons, 
// and level buttons,
// find the one nearest the mouse cursor location.
// If within a distance threshold, set nearestInput to
// indicate the channel number of the nearest input channel icon
// (a value of NULL_CHANNEL indicates no icon was within threshold of cursor).
// Do the same for the output channel icons, setting nearestOutput.
// A single call to this routine will set at most one of 
// nearestInput, nearestOutput, and currentLevel.  
void
findNearestItem( void )
{
	double nearestDist;
	double thresholdDist = 0.05;	// 5% of screen width
	double dist;


	// Find the level button nearest mouse cursor (if within threshold)
	int nearestLevelButton  = -1;
	nearestDist = 1000000.;
	for( int b=minLevel; b<maxLevel; b++ ) {
		// Calc distance from cursor to button.
		dist = L1Distance( xMouseInWorld, yMouseInWorld, 
			               xButtonPosition(b), yButtonPosition(b) );

		// Keep track of nearest one found so far (that is within threshold distance).
		if( dist < nearestDist  &&  dist < thresholdDist ) {
			nearestDist     = dist;
			nearestLevelButton   = b;
		}
	}
	if( nearestLevelButton != -1 ) {
		currentLevel = nearestLevelButton;
		return;
	}
	
	

	// Find the input channel icon nearest mouse cursor
	int nearestInput  = NULL_CHANNEL;
	nearestDist = 1000000.;
	for( int input=minInputChannel; input<maxInputChannel; input++ ) {
		// Calc distance from cursor to channel icon.
		dist = L1Distance( xMouseInWorld, yMouseInWorld, 
			               xInputPosition(input), yInputPosition(input) );

		// Keep track of nearest one found so far (that is within threshold distance).
		if( dist < nearestDist  &&  dist < thresholdDist ) {
			nearestDist     = dist;
			nearestInput   = input;
		}
	}
	if( nearestInput != NULL_CHANNEL ) {
		selectedInput = nearestInput;
		return;
	}

	
	// Find the output channel icon nearest mouse cursor.
	int nearestOutput  = NULL_CHANNEL;
	nearestDist = 1000000.;
	for( int output=minOutputChannel; output<maxOutputChannel; output++ ) {
		// Calc distance from cursor to channel icon.
		dist = L1Distance( xMouseInWorld, yMouseInWorld, 
			               xOutputPosition(output), yOutputPosition(output) );

		// Keep track of nearest one found so far (that is within threshold distance).
		if( dist < nearestDist  &&  dist < thresholdDist ) {
			nearestDist     = dist;
			nearestOutput   = output;
		}
	}
	if( nearestOutput != NULL_CHANNEL ) {
		selectedOutput = nearestOutput;
		return;
	}
}


/**************************************************************************************/
// Callback routine: called when mouse is moved while a button is down.
// Only called when cursor loc changes.
// x,y:    cursor loc in window coords
// see p658 Woo 3rd ed
void
mouseMotionFuncUI( int x, int y )
{
	// Map mouse cursor window coords to world coords.
	// Since we're using an orthoscopic projection parallel to the Z-axis,
	// we can map (x,y) in window coords to (x,y,0) in world coords.
	calcMouseWorldLoc( x, y );
}


/**************************************************************************************/
// Callback routine: called when window is resized by user.
// Also called once when window is created.
void
reshapeWindowFuncUI( int newWindowWidth, int newWindowHeight )
{
	// Save current window dimensions.
	UIwindowWidth  = newWindowWidth;
	UIwindowHeight = newWindowHeight;

	// Set size of characters in world units for font GLUT_BITMAP_8_BY_13.
	charWidthInWorld  =  8. / UIwindowWidth;
	charHeightInWorld = 13. / UIwindowHeight;

	// If the window is too small, don't display channel numbers.
	if(     UIwindowWidth < 150 || UIwindowHeight < 150 ) windowSize = WINDOW_SIZE_TINY;
	else if(UIwindowWidth < 300 || UIwindowHeight < 300 ) windowSize = WINDOW_SIZE_SMALL;
	else if(UIwindowWidth < 500 || UIwindowHeight < 500 ) windowSize = WINDOW_SIZE_MEDIUM;
	else                                                  windowSize = WINDOW_SIZE_BIG;

		// viewport covers whole window
	glViewport( 0, 0, (int)UIwindowWidth, (int)UIwindowHeight );
}


/**************************************************************************************/
// Calculate where the mouse cursor maps to in world coordinates, 
// based on the window width and height and the edges of
// the frustum of the orthoscopic projection.
void
calcMouseWorldLoc( int xMouse, int yMouse ) 
{
	// Get cursor loc in window coords (passed in as args).
	int xMouseInWindow = xMouse;
	int yMouseInWindow = yMouse;	

	// calculate normalized cursor position in window: [0,1]
	double xMouseNormalized = xMouseInWindow / UIwindowWidth;
	double yMouseNormalized = yMouseInWindow / UIwindowHeight;

	// invert normalized Y due to up being - in mouse coords, but + in ortho coords.
	yMouseNormalized = 1. - yMouseNormalized;

	// Calculate cursor position in ortho frustum's XY plane.
	// This window looks at the unit square x:[0,1] and y:[0,1],
	// so normalized coords and world coords are the same.
	xMouseInWorld = xMouseNormalized;
	yMouseInWorld = yMouseNormalized;	
}




