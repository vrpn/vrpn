
#include <stdio.h>                      // for fprintf, printf, sprintf, etc
#include <string.h>                     // for strncmp, strlen, strncpy

#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Serial.h"                // for vrpn_close_commport, etc
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs, etc
#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_Tracker_NDI_Polaris.h"
#include "vrpn_Types.h"                 // for vrpn_float64

vrpn_Tracker_NDI_Polaris::vrpn_Tracker_NDI_Polaris(const char *name, 
												   vrpn_Connection *c,
												   const char *port,
												   int numOfRigidBodies,
												   const char** rigidBodyNDIRomFileNames) : vrpn_Tracker(name,c)
{
	/////////////////////////////////////////////////////////
	//STEP 1: open com port at NDI's default speed
	/////////////////////////////////////////////////////////
	
	serialFd=vrpn_open_commport(port,9600);
	if (serialFd==-1){
		fprintf(stderr,"vrpn_Tracker_NDI_Polaris: Can't open serial port: %s\n",port);    
	} else {
	
		printf("connected to NDI Polaris at default 9600 baud on device:%s.\n",port);
		
		//send a reset
#ifdef DEBUG
		printf("DEBUG: Reseting com port");
#endif
		vrpn_set_rts(serialFd);
		vrpn_SleepMsecs(100);
		vrpn_clear_rts(serialFd);
		printf("done\n");
		
        vrpn_SleepMsecs(100); //if the NDI was previously running at the higher rate, it will need some time to reset
		vrpn_flush_input_buffer(serialFd); //get rid of any reset message the NDI might have sent
		
		/////////////////////////////////////////////////////////
		//STEP 2: switch to a higher baud rate
		/////////////////////////////////////////////////////////
		switchToHigherBaudRate(port);
		
		/////////////////////////////////////////////////////////
		//STEP 3: INIT tracker
		/////////////////////////////////////////////////////////
		
		
		sendCommand("INIT ");
		readResponse();
#ifdef DEBUG
		printf("DEBUG:Init response: >%s<\n",latestResponseStr);	
#endif

		//0 = 20hz(default), 1= 30Hz, 2=60Hz
		sendCommand("IRATE 0"); //set the illumination to the default
        readResponse();
#ifdef DEBUG
		printf("DEBUG: IRATE response: >%s<\n",latestResponseStr);	
#endif
    
		
		/////////////////////////////////////////////////////////
		//STEP 4: SETUP EACH TOOL (i.e. rigid body)
		/////////////////////////////////////////////////////////

		this->numOfRigidBodies=numOfRigidBodies;
		
		//declare an array of filenames, one for each tool		
		for (int t=0; t<numOfRigidBodies; t++) {
			if (setupOneTool(rigidBodyNDIRomFileNames[t])<1) {
				fprintf(stderr,"vrpn_Tracker_NDI_Polaris: tool %s didn't init properly!\n",rigidBodyNDIRomFileNames[t]); 
			}
		}
		
		/////////////////////////////////////////////////////////
		//STEP 5: GO TO TRACKING MODE
		/////////////////////////////////////////////////////////
		
		sendCommand("TSTART 80"); //80 resets the frame counter to zero
		readResponse();
#ifdef DEBUG
		printf("DEBUG: Tstart response: >%s<\n",latestResponseStr);
#endif
	}
	
}

vrpn_Tracker_NDI_Polaris::~vrpn_Tracker_NDI_Polaris()
{
	vrpn_close_commport(serialFd);
}

void vrpn_Tracker_NDI_Polaris::mainloop()
{
	get_report();
	
	// Call the generic server mainloop, since we are a server
	server_mainloop();
	
	return;
}

int vrpn_Tracker_NDI_Polaris::get_report(void)
// returns 0 on fail
{
	vrpn_gettimeofday(&timestamp, NULL);
	
	sendCommand("TX ");
	readResponse();
	//printf("DEBUG: TX response: >%s %i<\n",latestResponseStr);
	
	//int numOfHandles=parse2CharIntFromNDIResponse(latestResponseStr);
	int TXResponseStrIndex=2;
	bool gotAtLeastOneReport=false;
	
	for (int t=0; t<numOfRigidBodies; t++) {
		//int handleNum=parse2CharIntFromNDIResponse(latestResponseStr,&TXResponseStrIndex);
		// check if the tool is present. Parse and print out the transform data if and only if
		// it's not missing
		if (strncmp("MISSING",(char *) &latestResponseStr[TXResponseStrIndex],7)!=0) {
			gotAtLeastOneReport=true; 
			d_sensor = t;
			
			// NDI gives qw first(qw,qx,qy,qz), whereas VRPN wants it last (qx,qy,qz,qw)
			d_quat[3]=parse6CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex); //qw
			d_quat[0]=parse6CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex); //qx
			d_quat[1]=parse6CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex); //qy
			d_quat[2]=parse6CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex); //qz
			
			pos[0]=parse7CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex); 
			pos[1]=parse7CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex);
			pos[2]=parse7CharFloatFromNDIResponse(latestResponseStr,&TXResponseStrIndex);
			
			send_report(); //call this once per sensor
		} 

		//seek to the new line character (since each rigid body is on its own line)
		while (latestResponseStr[TXResponseStrIndex]!='\n') {
			TXResponseStrIndex++;
		}
		TXResponseStrIndex++; // advance one more char as to be on the next line of text
		
	}// for  
	
	return(gotAtLeastOneReport); //return 1 if something was sent, 0 otherwise
}


void vrpn_Tracker_NDI_Polaris::send_report(void) // called from get_report()
{
	if (d_connection) 
    {
      char	msgbuf[VRPN_MSGBUFSIZE];
      int	len = encode_to(msgbuf);
      if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
                                     vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr,"vrpn_Tracker_NDI_Polaris: cannot write message: tossing\n");
      }
    }
}


// Send a command to the NDI tracker over the serial port.
// This assumes the serial port has already been opened.
// Some NDI commands require a white-space char at the end of the command, the
// call must be sure to add it.
// NDI commands end with a Carriage-return (\r) char. This function automatically adds it, and
// flushes the output buffer.
// commandString MUST be terminated with \0, since we don't know the string
// length otherwise.
// The NDI trackers have two different command syntaxes - this only supports the syntax
// WITHOUT CRC checksums

void vrpn_Tracker_NDI_Polaris::sendCommand(const char* commandString ){
	vrpn_write_characters(serialFd,(const unsigned char* )commandString,strlen(commandString));
	vrpn_write_characters(serialFd,(const unsigned char* )"\r",1); //send the CR
	vrpn_flush_output_buffer(serialFd);
}

// Read a fully formed responses from the NDI tracker, (including the 4-byte CRC at the end)
// and copies it to latestResponseString.
// Returns the number of characters in the response (not including the CR),
// or -1 in case of an error
// NDI responses are all terminated with a CR, which this function replace with a end-of-string char.
//
// This function blocks until the CR has been received.
// FIXME: add a timeout parameter, and timeout if it's been too long
int vrpn_Tracker_NDI_Polaris::readResponse(){
	//read in characters one-at-a-time until we come across a CR
	int charIndex=0;
	bool foundCR=false;
	unsigned char c;
	while (!foundCR) {
		if (vrpn_read_available_characters(serialFd,&c,1)>0) {
			latestResponseStr[charIndex]=c;
			if (c=='\r') { //found CR
				latestResponseStr[charIndex]='\0';
				foundCR=true;
			} else {
				charIndex++;
			}
		}
	}
	return (charIndex);
}

// Given a filename of a binary .rom file, this reads the file and returns
// a string with the contents of the file as ascii encoded hex: For each byte of
// the file, this returns two ascii characters, each of which are ascii representation
// of a HEX digit (0-9 & a-f). Hex letters are returned as lower case.
// The string is padded with zeros to make it's length a multiple of 128
// characters( which is 64 bytes of the original binary file).
// asciiEncodedHexStr must be allocated before calling, be
// MAX_NDI_FROM_FILE_SIZE_IN_BYTES * 2 characters long.
//
// RETURNS the number of bytes represented in the string (which is half the number of ASCII characters)
// , or -1 on failure
int vrpn_Tracker_NDI_Polaris::convertBinaryFileToAsciiEncodedHex(const char* filename, char *asciiEncodedHexStr)
{
	FILE* fptr=fopen(filename,"rb");
	if (fptr==NULL) {
		fprintf(stderr,"vrpn_Tracker_NDI_Polaris: can't open NDI .rom file %s\n",filename);
		return (-1);
	}
	
	// obtain file size:
	fseek(fptr , 0 , SEEK_END);
	long int fileSizeInBytes = ftell(fptr);
	rewind(fptr);
	
	if (fileSizeInBytes>MAX_NDI_ROM_FILE_SIZE_IN_BYTES) {
		fprintf(stderr,"vrpn_Tracker_NDI_Polaris: file is %ld bytes long - which is larger than expected NDI ROM file size of %d bytes.\n",
			fileSizeInBytes,MAX_NDI_ROM_FILE_SIZE_IN_BYTES);
		fclose(fptr);
		return (-1);
	}
	
	// allocate memory to contain the whole file:
        unsigned char* rawBytesFromRomFile;
        try { rawBytesFromRomFile = new unsigned char[fileSizeInBytes]; }
        catch (...) {
          fprintf(stderr, "vrpn_Tracker_NDI_Polaris: Out of memory!\n");
          fclose(fptr);
          return(-1);
        }
	
	// copy the file into the buffer:
	size_t result = fread (rawBytesFromRomFile,1,fileSizeInBytes,fptr);
	if (result != (unsigned int) fileSizeInBytes) {
		fprintf(stderr,"vrpn_Tracker_NDI_Polaris: error while reading .rom file!\n");
                try {
                  delete [] rawBytesFromRomFile;
                } catch (...) {
                  fprintf(stderr, "vrpn_Tracker_NDI_Polaris::convertBinaryFileToAsciiEncodedHex(): delete failed\n");
                  return -1;
                }
		fclose(fptr);
		return(-1);
	}
	fclose(fptr);

	// init array with "_" for debugging
	for (int i=0; i<MAX_NDI_ROM_FILE_SIZE_IN_BYTES; i++) {
		asciiEncodedHexStr[i]='_';
	}
	int byteIndex;
	for (byteIndex=0; byteIndex<fileSizeInBytes; byteIndex++) {
		sprintf(&(asciiEncodedHexStr[byteIndex*2]),"%02x ",rawBytesFromRomFile[byteIndex]);
	}
	
	// pad the length to make it a multiple of 64
	int numOfBytesToPadRemaining= NDI_ROMFILE_CHUNK_SIZE-(fileSizeInBytes% NDI_ROMFILE_CHUNK_SIZE);
	if (numOfBytesToPadRemaining== NDI_ROMFILE_CHUNK_SIZE) {numOfBytesToPadRemaining=0;}
	
	int paddedFileSizeInBytes=fileSizeInBytes+numOfBytesToPadRemaining;
	
	while (numOfBytesToPadRemaining>0) {
		asciiEncodedHexStr[byteIndex*2]='0';
		asciiEncodedHexStr[byteIndex*2+1]='0';
		byteIndex++;
		numOfBytesToPadRemaining--;
	}
	
	asciiEncodedHexStr[byteIndex*2]='\0'; //end of string marker, which is used just for debugging
	
	//printf("DEBUG: >>%s<<\n",asciiEncodedHexStr);
        try {
          delete [] rawBytesFromRomFile;
        } catch (...) {
          fprintf(stderr, "vrpn_Tracker_NDI_Polaris::convertBinaryFileToAsciiEncodedHex(): delete failed\n");
          return -1;
        }
	
	return paddedFileSizeInBytes;
}

// NDI response strings often encode ints as a two ascii's WITHOUT any separator behind it
// this returns the value as an int.
// The caller passes in the string, and a pointer to an (int) index into that string (which will be advanced
// to the end of the value we just parsed.
// The caller must make sure the string is at least two characters long
unsigned int vrpn_Tracker_NDI_Polaris::parse2CharIntFromNDIResponse(unsigned char* str, int* strIndexPtr) {
	unsigned intVal;
	if (strIndexPtr==NULL){ //if no index was passed in, start at the beginning of the string, and don't
		//advance the index
		sscanf((char*) str,"%2u",&intVal);
	} else { 
		sscanf((char*) &(str[*strIndexPtr]),"%2u",&intVal);
		*strIndexPtr+=2;
	} 
	return (intVal);
}

// NDI TX response strings often encode floats as a size ascii's WITHOUT any separator behind it
// this returns the value as an float. The last 4 digits are implicitly to the right of the decimal point
// (the decimal point itself is not in the string)
// The caller passes in the string, and a pointer to an (int) index into that string (which will be advanced
// to the end of the value we just parsed.
// The caller must make sure the string is at least six characters long
float vrpn_Tracker_NDI_Polaris::parse6CharFloatFromNDIResponse(unsigned char* str,  int* strIndexPtr) {
	int intVal;
	sscanf((char*) &(str[*strIndexPtr]),"%6d",&intVal);
	*strIndexPtr+=6;
    return (intVal/10000.0f); 
}

// NDI TX response strings often encode floats as a size ascii's WITHOUT any separator behind it
// this returns the value as an float. The last 2 digits are implicitly to the right of the decimal point
// (the decimal point itself is not in the string)
// The caller passes in the string, and a pointer to an (int) index into that string (which will be advanced
// to the end of the value we just parsed.
// The caller must make sure the string is at least seven characters long
float vrpn_Tracker_NDI_Polaris::parse7CharFloatFromNDIResponse(unsigned char* str,  int* strIndexPtr) {
	int intVal;
	sscanf((char*) &(str[*strIndexPtr]),"%7d",&intVal);
	*strIndexPtr+=7;
    return (intVal/100.0f); 
}

int vrpn_Tracker_NDI_Polaris::setupOneTool(const char* NDIToolRomFilename)
{
	// For each tool (i.e. rigid body) that we want to track, we must first need to initialize a port handle (on the NDI machine)
	// for that tool. This function does that. the caller must call it for each tool.
	// This returns -1 on failure, or the tool number (i.e. port handle), which is 1 or greater (VRPN 
	// sensors start at 0, whereas NDI port handles start at 1).
	
	// We use the command
	// sequence PHRQ -> PVWR -> PINIT -> PENA; we must do this for each tool we want tracked. 
	
	// sending the PHRQ command first; this will assign a port handler number to the tracking device. the
	// first 8 digits are the hardware device ID, but one can just use asterisks as wild cards. the 9th digit
	// is system type, 0 for polaris or just *. the 10th digit needs to be a 1 with the passive tracking
	// balls that we are using in this setup (0 for an active wired LED setup). 11th & 12th digits are to
	// request a specific port number (we may as well do that here), and the last two are "reserved".
	char commandStr[MAX_NDI_RESPONSE_LENGTH];
	
	sendCommand("PHRQ *********1****");
	readResponse();
#ifdef DEBUG
	printf("DEBUG: PHRQ response: >%s<\n",latestResponseStr);
#endif	
	if (strncmp("ERROR",(char *) latestResponseStr,5)==0) return (-1);
	
	unsigned int portHandleNum = parse2CharIntFromNDIResponse(latestResponseStr);
	
	//convert to ASCII-encoded hex, write to array of chars
	char asciiEncodedHex[MAX_NDI_ROM_FILE_SIZE_IN_BYTES*2]; //each byte needs two digits, so we multiply by 2
	int numOfFileBytes=convertBinaryFileToAsciiEncodedHex(NDIToolRomFilename,asciiEncodedHex);
	
	//feed that array of chars to NDI, but we must first divide it up into 64 byte (128 ascii char)
	//"chunks", and send on chunk at a time.
	int numOfChunks=numOfFileBytes/NDI_ROMFILE_CHUNK_SIZE;
	for (int chunkIndex=0; chunkIndex<numOfChunks; chunkIndex++) {
		char chunk[129]; //64*2 +1 for the end of string
                vrpn_strcpy(chunk,&(asciiEncodedHex[chunkIndex*NDI_ROMFILE_CHUNK_SIZE*2]));
		
		int NDIAddress=chunkIndex*NDI_ROMFILE_CHUNK_SIZE; //the memory offset (in the NDI machine, not this PC)
		// where this chunk will start
		sprintf(commandStr,"PVWR %02u%04x%s",portHandleNum, NDIAddress,chunk);
		//printf("DEBUG: >>%s<<\n",commandStr);
		sendCommand(commandStr);
		readResponse();
#ifdef DEBUG
		printf("DEBUG: PVWR response: >%s<\n",latestResponseStr);
#endif
	}
	sprintf(commandStr,"PINIT %02u",portHandleNum);  // initializes the port handle
	sendCommand(commandStr);
	readResponse();
#ifdef DEBUG
	printf("DEBUG: PINIT response: >%s<\n",latestResponseStr); //this will be an error if PVWR hasn't been sent
#endif
	if (strncmp("ERROR",(char *) latestResponseStr,5)==0) return (-1);
	
	sprintf(commandStr,"PENA %02uD",portHandleNum); //enables the port handle as a dynamic tool (expect motion)
	sendCommand(commandStr); 
	readResponse();
#ifdef DEBUG
	printf("DEBUG: PENA response: >%s<\n",latestResponseStr); //this will be an error if PINIT hasn't been sent
#endif
	//FIXME: PENA returns warnings if the tool geometry has problems, Parse these warnings and return them
	return(portHandleNum);
}

void vrpn_Tracker_NDI_Polaris::switchToHigherBaudRate(const char *port) {
	printf("vrpn_Tracker_NDI_Polaris: Switching NDI to higher baud rate, and then reopening com port at higher rate...");
	sendCommand("COMM 70000"); // 1.2Mbit (1228739 baud), which requires the NDI USB 2.0 adapter
	readResponse();
#ifdef DEBUG
	printf("DEBUG: COMM response: >%s<\n",latestResponseStr);
#endif
	//if the response is "RESET", try again
	if (strncmp("RESET",(char *) latestResponseStr,5)==0) {
		//we got a reset, which means the track reset itself (without being commanded too?)
		sendCommand("COMM 70000"); // 1.2Mbit, which requires the NDI USB 2.0 adapter
		readResponse();
		//printf("DEBUG: COMM response: >%s<\n",latestResponseStr);
	}
	vrpn_SleepMsecs(100); //we should wait 100 msec after getting the OKAY from the NDI before changing the PC's comm rate 
	
	vrpn_close_commport(serialFd);
#ifdef _WIN32   
	serialFd=vrpn_open_commport(port,19200); //19.2k is aliased to 1.2Mbit in the Window's version of the NDI USB virtual com port driver 
#else
	serialFd=vrpn_open_commport(port,1228739); //should work on linux (UNTESTED!) 
#endif
	printf("done\n");
}


