// Sensics streaming Arduino code to go along with the 
// vrpn_streaming_arduino device.
// Copyright 2015
// Author: Russell Taylor working through Reliasolve.com
// LICENSE: VRPN standard license

// This program is meant to be paired with the VRPN.org device
// vrpn_streaming_arduino, which will send the appropriate
// commands to control it and read back its results.  The program can also
// be run from the serial monitor or from any serial program as well.

// It prints the analog input from a set of analogs as quickly as possible,
// with the number of analogs read from the serial line at program start.
// It also lets the program request the insertion of additional markers,
// which it does by sending the marker number on the serial stream.

// INPUT: An initial ASCII number followed by a carriage return indicating
// how many analogs to be read.  The number must be between 1 and 8.
//   Optional: numeric marker commands, each followed by a carriage return
// indicating a host-side event to be correlated with the analog data.  These
// are inserted into the output stream and returned.  Markers must be larger
// than 0.

// OUTPUT: Streaming lines.  Each line consists of:
//   (1) A list of comma-separated ASCII values starting with Analog0
// and continuing up to the number of analogs requests (Analog1, Analog2,
// ... Analog[N-1].
//   (2) An optional list of numeric event markers that were sent across
// the serial line that were received since the last report.

// Initialize the number of analogs to an invalid value so the
// user has to specify this before we start.
int numAnalogs = 0;

// An array of markers that can be filled in and will be reported
// at the next sending event.
int  numMarkers = 0;
const int maxMarkers = 100;
int markers[maxMarkers];

//*****************************************************
void setup() 
//*****************************************************
{
  // Set the communication speed on the serial port to the fastest we
  // can.
  Serial.begin(115200);
  
  // Set the timeout on input-string parsing to 1ms, so we
  // don't waste a lot of time waiting for the completion of
  // numbers.
  Serial.setTimeout(1);
}

//*****************************************************
void readAndParseInput()
//*****************************************************
{
  // Nothing to do if no available characters.
  if (Serial.available() == 0) {
    return;
  }
  
  // If we don't have the number of analogs specified validly,
  // set it.
  if (numAnalogs <= 0) {
    numAnalogs = Serial.parseInt();
  }
  
  // We already have our analogs specified, so this is
  // a marker request.  Add it to the array of outstanding
  // requests.
  while (Serial.available() > 0) {
    int newMarker = Serial.parseInt();
    if (newMarker > 0) {
      if (numMarkers == maxMarkers) { return; }
      markers[numMarkers] = newMarker;
      numMarkers++;
    }
  }
}

//*****************************************************
void loop() 
//*****************************************************
{
  readAndParseInput();
  if (numAnalogs > 0) {
    Serial.print(analogRead(0));
    for (int i = 1; i < numAnalogs; i++) {
      Serial.print(",");
      Serial.print(analogRead(i));
    }
    int i;
    for (i = 0; i < numMarkers; i++) {
      Serial.print(",");
      Serial.print(markers[i]);
    }
    numMarkers = 0;
    Serial.print("\n");
  }
}//end loop

