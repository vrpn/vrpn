// NmeaParser.cpp: Implementation of the NMEA-0183 2.0 parser class.
//
// Written by Jason Bevins in 1998.  File is in the public domain.
//

#ifdef sgi
#include <stdlib.h>
#include <string.h>
#else
#include <cstdlib>                      // for atof, atoi, atol
#endif
#include "nmeaParser.h"

// This constant define the length of the date string in a sentence.
const int DATE_LEN = 6;

/////////////////////////////////////////////////////////////////////////////
// NMEAData construction/destruction

NMEAData::NMEAData ()
{
  // The constructor for the NMEAData object clears all data and resets the
  // times of last acquisition to 0.
  reset ();

  // No coordinate has ever been valid since this object was created.
  hasCoordEverBeenValid = false;
}


/////////////////////////////////////////////////////////////////////////////
// NMEAData methods

void NMEAData::reset()
  // Purpose:
  //  This function resets all the data in this object to its defaults.
{
  // Reset the data.
  lat       = 0.0;
  lon       = 0.0;
  altitude  = 0.0;
  speed     = 0.0;
  track     = 0.0;
  magVariation = 0.0;
  hdop      = 1.0;
  nStd      = 1.0;
  eStd      = 1.0;
  zStd      = 1.0;
  hStd      = 1.0;
  numSats   = 0;
  UTCYear   = 94;
  UTCMonth  = 6;
  UTCDay    = 1;
  UTCHour   = 0;
  UTCMinute = 0;
  UTCSecond = 0;

  // All data stored by this object is currently invalid.
  isValidLat      = false;
  isValidLon      = false;
  isValidAltitude = false;
  isValidSpeed    = false;
  isValidDate     = false;
  isValidTime     = false;
  isValidTrack    = false;
  isValidMagVariation = false;
  isValidHdop     = false;
  isValidNStd     = false;
  isValidEStd     = false;
  isValidZStd     = false;
  isValidHStd     = false;
  isValidSatData  = false;
  isValidRangeResidualData = false;
  isValidHStd     = false;
  isValidFixQuality = false;

  // Last fix was invalid.
  lastFixQuality = FIX_AUTONOMOUS;

  // Still waiting for the frame to start...
  waitingForStart = true;

  // Clear what messages have been seen
  seenZDA = false;
  seenGGA = false;
  seenGLL = false;
  seenRMC = false;
  seenGSV = false;
  seenGST = false;
  seenVTG = false;
  seenRRE = false;
}

/////////////////////////////////////////////////////////////////////////////
// NMEAParser construction/destruction

NMEAParser::NMEAParser () //sj: log4cpp::Category& l) : logger(l)
{
  m_data = new NMEAData();
  // by default, we'll look for "RMC" message to signify a new sequence of messages
  strcpy(startSentence, "RMC");

  // Defaults for the GSV sentence parser.
  m_lastSentenceNumber = 1;
  m_numSentences       = 1;
  m_numSatsExpected    = 0;
  m_numSatsLeft        = 0;
  m_satArrayPos        = 0;
}

NMEAParser::NMEAParser (NMEAData* data)//sj: log4cpp::Category& l, NMEAData* data) : logger(l)
{
  m_data = data;
  // Defaults for the GSV sentence parser.
  m_lastSentenceNumber = 1;
  m_numSentences       = 1;
  m_numSatsExpected    = 0;
  m_numSatsLeft        = 0;
  m_satArrayPos        = 0;
}

NMEAParser::~NMEAParser ()
{
  delete m_data;
  m_data = 0;
}

void NMEAParser::parseZDA(const char* sentence)
{
  m_data->seenZDA=true;
  char field[255];
  uint_ currentPos = 0;
  bool isMore = true; // More strings to parse?
  
  // Skip past the '$xxZDA'
  if ((isMore = getNextField (field, sentence, currentPos)) == false)
    {
      return;
    }
  // UTC time.
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateTime (field);
}

void NMEAParser::parseGGA (const char* sentence)
  // Purpose:
  //  This function parses a GGA sentence; all data will be stored in the
  //  NMEAData object within this class.  This function will correctly parse
  //  a partial GGA sentence.
  // Pre:
  //  The string to parse must be a valid GGA sentence.
{
  m_data->seenGGA=true;
  char field[255];
  char hemisphereField[32];
  char hemisphereChar;
  char unitField[32];
  char unitChar;
  uint_ currentPos = 0;
  bool isMore = true; // More strings to parse?

  // Skip past the '$xxGGA'
  if ((isMore = getNextField (field, sentence, currentPos)) == false)
    {
      return;
    }
  
  // UTC time.
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateTime (field);
  if (isMore == false) return;

  // Latitude.
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLat (field, hemisphereChar);
  if (isMore == false) return; 

  // Longitude.
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLon (field, hemisphereChar);
  if (isMore == false) return; 

  // Quality of GPS fix.
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateFixQuality (field);
  if (isMore == false) return; 

  // Skip number of sats tracked for now.
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Horizontal dilution of precision (HDOP).
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateHdop (field);
  if (isMore == false) return;
	
  // Altitude.
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (unitField, sentence, currentPos);
  if (strlen (unitField) != 0) {
    unitChar = unitField[0];
  } else {
    unitChar = ' ';
  }
  parseAndValidateAltitude (field, unitChar);
  if (isMore == false) return; 

  // Everything else (geoid height and DGPS info) is ignored for now.
  return;
}

void NMEAParser::parseGLL (const char* sentence)
  // Purpose:
  //  This function parses a GLL sentence; all data will be stored in the
  //  NMEAData object within this class.  This function will correctly parse
  //  a partial GLL sentence.
  // Pre:
  //  The string to parse must be a valid GLL sentence.
{
  m_data->seenGLL=true;
  char field[255];
  char hemisphereField[32];
  char hemisphereChar;
  uint_ currentPos = 0;
  bool isMore = true; // More strings to parse?

  // Count commas in this sentence to see if it's valid.
  if (countChars (sentence, ',', SENTENCE_GLL_COMMAS) < 0) return;

  // Skip past the '$xxGLL'
  if ((isMore = getNextField (field, sentence, currentPos)) == false) {
    return;
  }
	
  // Latitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLat (field, hemisphereChar);
  if (isMore == false) return; 

  // Longitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLon (field, hemisphereChar);
  if (isMore == false) return; 

  // UTC time
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateTime (field);
  if (isMore == false) return;
}


void NMEAParser::parseGSV (const char* sentence)
  // Purpose:
  //  This function parses a GSV sentence; all data will be stored in the
  //  NMEAData object within this class.  This function will correctly parse
  //  a partial GSV sentence.
  // Pre:
  //  The string to parse must be a valid GSV sentence.
  // Notes:
  //  All GSV sentences from a single packet (a collection of NMEA sentences
  //  sent from the GPS) must be processed before satellite information in the
  //  NMEAData object is updated.  There is data for only four satellites
  //  in each GSV sentence, so multiple sentences must be processed.  For
  //  example, if your GPS was tracking eight satellites, two GSV sentences is
  //  sent from your GPS in each packet; both sentences must be parsed before
  //  the NMEAData object is updated with the satellite information.
{
  m_data->seenGSV=true;
  char field[255];
  uint_ numSats;
  int numSentences;
  int sentenceNumber;
  uint_ currentPos = 0;
  bool isMore = true;  // More strings to parse?

  // Count commas in this sentence to see if it's valid.
  if (countChars (sentence, ',', SENTENCE_GSV_COMMAS) < 0) return;

  // Skip past the '$xxGSV'
  if ((isMore = getNextField (field, sentence, currentPos)) == false) {
    return;
  }

  // Determine the number of sentences that will make up the satellite data.
  isMore = getNextField (field, sentence, currentPos);
  numSentences = atoi (field);
  if (isMore == false) return; 

  // Which sentence is this?.
  isMore = getNextField (field, sentence, currentPos);
  sentenceNumber = atoi (field);
  if (isMore == false) return; 

  // How many satellites are in total?
  isMore = getNextField (field, sentence, currentPos);
  numSats = atoi (field);
  if (isMore == false) return;

  // Is this the first sentence?  If so, reset the satellite information.
  if (sentenceNumber == 1) {
    m_lastSentenceNumber = 1;
    m_numSentences = numSentences;
    m_numSatsExpected = numSats;
    m_numSatsLeft = numSats;
    m_satArrayPos = 0;
  } else {
    // Make sure the satellite strings are being sent in order.  If not,
    // then you're screwed.
    if (sentenceNumber != m_lastSentenceNumber + 1) return;

    // BUGFIX:
    // Added by Clarke Brunt 20001112
    m_lastSentenceNumber = sentenceNumber;
  }

  // parse the satellite string.  There are four satellite info fields per
  // sentence.
  int i;
        
  for (i = 0; i < 4; i++) {
    getNextField (field, sentence, currentPos);
    if (strlen (field) != 0) {
      m_tempSatData[m_satArrayPos].prn = atoi (field);
      getNextField (field, sentence, currentPos);
      if (strlen (field) != 0) {
        m_tempSatData[m_satArrayPos].elevation = atoi (field);
      }
      getNextField (field, sentence, currentPos);
      if (strlen (field) != 0) {
        m_tempSatData[m_satArrayPos].azimuth = atoi (field);
      }
      getNextField (field, sentence, currentPos);
      if (strlen (field) != 0) {
        m_tempSatData[m_satArrayPos].strength = atoi (field);
      }
      --m_numSatsLeft;
      ++m_satArrayPos;
    } else {
      // Jump past the next three fields.
      for (int j = 0; j < 3; j++)
        getNextField (field, sentence, currentPos);
    }
  }

  // If all the satellite information has been received, then update the
  // NMEAData object with the new satellite data.
  if (m_numSatsLeft == 0) {
    for (i = 0; i < m_numSatsExpected; i++) {
      m_data->satData[i] = m_tempSatData[i];
    }
    m_data->numSats = m_numSatsExpected;
    m_data->isValidSatData = true;
  }
}

void NMEAParser::parseRRE (const char* sentence)
{
  m_data->seenRRE=true;
  bool isMore;
  char field[255];
  uint_ currentPos = 0;
  // Skip past the '$xxRMC'
  if ((isMore = getNextField (field, sentence, currentPos)) == false)
    {
      return;
    }
  isMore = getNextField (field, sentence, currentPos);
  // I assume the RRE numSats is the same as the RRE numSats!
  m_data->numSats = atoi(field);

  // Skip the intermediate stuff
  for (uint_ i = 0; i < m_data->numSats; i++)
    {
      isMore = getNextField (field, sentence, currentPos);
      if (isMore == false)
        {
          //sj: logger.error("only read %d out of %d entries", m_data->numSats, i);
          return;
        }
      m_data->rrData[i].prn = atoi(field);
      isMore = getNextField (field, sentence, currentPos);
      if (isMore == false)
        {
          //sj: logger.error("only read %d out of %d entries", m_data->numSats, i);
          return;
        }
      m_data->rrData[i].residual = atof(field);
    }
  m_data->isValidRangeResidualData = true;

  // Now read the horizontal 
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false)
    {
      // sj: logger.error("no fields left to read hStd");
      return;
    }
  m_data->hStd = atof(field);
  m_data->isValidHStd = true;
  
  // Now read the vertical 
  isMore = getNextField (field, sentence, currentPos);

  m_data->zStd = atof(field);
  m_data->isValidZStd = true;
  //sj: logger.debug("isValidRangeResidualData=%d",
  //sj:             m_data->isValidRangeResidualData);
  //sj: logger.debug("isValidHStd=%d", m_data->isValidHStd);
  //sj: logger.debug("isValidZStd=%d", m_data->isValidZStd);
}


void NMEAParser::parseRMC (const char* sentence)
  // Purpose:
  //  This function parses an RMC sentence; all data will be stored in the
  //  NMEAData object within this class.  This function will correctly parse
  //  a partial RMC sentence.
  // Pre:
  //  The string to parse must be a valid RMC sentence.
{
  m_data->seenRMC=true;
  char field[255];
  char hemisphereField[32];
  char hemisphereChar;
  char directionField[32];
  char directionChar;
  uint_ currentPos = 0;
  bool isMore = true; // More strings to parse?

  // Count commas in this sentence to see if it's valid.
  if (countChars (sentence, ',', SENTENCE_RMC_COMMAS) < 0) return;

  // Skip past the '$xxRMC'
  if ((isMore = getNextField (field, sentence, currentPos)) == false) {
    return;
  }
	
  // UTC time
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateTime (field);
  if (isMore == false) return;

  // Skip past the navigation warning indicator for now.
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return;

  // Latitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLat (field, hemisphereChar);
  if (isMore == false) return; 

  // Longitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (hemisphereField, sentence, currentPos);
  if (strlen (hemisphereField) != 0) {
    hemisphereChar = hemisphereField[0];
  } else {
    hemisphereChar = ' ';
  }
  parseAndValidateLon (field, hemisphereChar);
  if (isMore == false) return; 

  // Current speed, in knots.
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateSpeed (field);
  if (isMore == false) return; 

  // Current track, in degrees.
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateTrack (field);
  if (isMore == false) return; 

  // Current date
  isMore = getNextField (field, sentence, currentPos);
  parseAndValidateDate (field);
  if (isMore == false) return;
	
  // Magnetic variation (degrees from true north)
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  isMore = getNextField (directionField, sentence, currentPos);
  if (strlen (directionField) != 0) {
    directionChar = directionField[0];
  } else {
    directionChar = ' ';
  }
  parseAndValidateMagVariation (field, directionChar);
  if (isMore == false) return; 
}

void NMEAParser::parseGST (const char* sentence)
{
  m_data->seenGST=true;
  char field[255];
  uint_ currentPos = 0;

  bool isMore = true; // More strings to parse?

  // Count commas in this sentence to see if it's valid.
  if (countChars (sentence, ',', SENTENCE_GST_COMMAS) < 0) return;

  // Skip past '$GPGST'
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the UTC time
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 
  parseAndValidateTime(field);

  // This field is the RMS value of the standard deviations to the range inputs
  // to the navigation process
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return;

  // Skip unimplemented fields
  currentPos += 3;

  // Standard deviation of latitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Parse the north standard deviation
  parseAndValidateNStd(field);

  // Standard deviation of longitude
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  parseAndValidateEStd(field);

  // Standard deviation of altitude
  isMore = getNextField (field, sentence, currentPos);

  parseAndValidateZStd(field);
}

void NMEAParser::parseVTG (const char* sentence)
{
  m_data->seenVTG=true;

  char field[255];
  char reference[256];
  uint_ currentPos = 0;

  bool isMore = true; // More strings to parse?

  // Count commas in this sentence to see if it's valid.
  if (countChars (sentence, ',', SENTENCE_VTG_COMMAS) < 0) return;

  // Skip past '$GPVTG'
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the COG wrt to true north
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the reference
  isMore = getNextField (reference, sentence, currentPos);
  if (isMore == false) return; 

  // Reference should be a 'T' to denote true north
  if (reference[0] != 'T')
    {
      // sj: logger.warn("parseVTG: the reference should be T but it's ",
      // sj:             reference);
      return;
    }
  
  // Get the track
  parseAndValidateTrack(field);


  // Get the COG wrt to magnetic north
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the reference
  isMore = getNextField (reference, sentence, currentPos);
  if (isMore == false) return; 

  // Reference should be a 'M' to denote wrt magnetic north
  if (reference[0] != 'M')
    {
      // sj: logger.warn("parseVTG: the reference should be M but it's ",
      // sj:             reference);
      return;
    }

  // Speed, should be in miles per hour
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the reference
  isMore = getNextField (reference, sentence, currentPos);
  if (isMore == false) return; 

  // Reference should be a 'N' to denote knots per hour
  if (reference[0] != 'N')
    {
      // sj: logger.warn("parseVTG: the reference should be N but it's ",
      // sj:             reference);
      return;
    }

  // Speed, should be in kilometres per hour
  isMore = getNextField (field, sentence, currentPos);
  if (isMore == false) return; 

  // Get the reference
  isMore = getNextField (reference, sentence, currentPos);

  // Reference should be a 'K' to denote kilometers per hour
  if (reference[0] != 'K')
    {
      // sj: logger.warn("parseVTG: the reference should be K but it's ",
      // sj:            reference);
      return;
    }
  parseAndValidateSpeed(field);
}

SENTENCE_STATUS NMEAParser::parseSentence (const char* sentence)
  // Purpose:
  //  This function parses a given NMEA sentence.  All valid information will be
  //  stored within the NMEAData object in this class; call the GetData()
  //  member function to retrieve the data.
  // Parameters:
  //  const char* sentence:
  //      The sentence to parse.
  // Returns:
  //  SENTENCE_VALID if the sentence passed is a valid NMEA sentence.
  //  SENTENCE_INVALID if the sentence passed is not a valid NMEA sentence, or
  //  the sentence type is unsupported (see NMEAParser.h for a list of
  //  supported sentences.)
  //  SENTENCE_BAD_CHECKSUM if the sentence has an invalid checksum.
{
#if 0   // sj
	if (logger.isDebugEnabled())
    {
      logger.debug("parsing sentence %s", sentence);
    }
#endif

  if (isCorrectChecksum (sentence) == false)
    {
      // sj: logger.debug("SENTENCE_BAD_CHECKSUM");
      return SENTENCE_BAD_CHECKSUM;
    }

  if (isKnownSentenceType (sentence) == false)
    {
      // sj: logger.debug("SENTENCE_UNKNOWN");
      return SENTENCE_UNKNOWN;
    }
  return parseValidSentence(sentence);
}

SENTENCE_STATUS NMEAParser::parseValidSentence (const char* sentence)
{
#if 0 // sj
	if (logger.isDebugEnabled())
    {
      logger.debug("NMEAParser: parsing %s", sentence);
    }
#endif 

  // Start the parsing 3 spaces past start to get past the initial
  // '$xx', where xx is the device type sending the sentences (GP =
  // GPS, etc.)
  uint_ currentPos = 3;
  char sentenceType[4];
  

  if (getNextField (sentenceType, sentence, currentPos) == false)
    {
      return SENTENCE_INVALID;
    }

  if (strcmp (sentenceType, startSentence) == 0)
    {
		// this message is the first in the sequence
		m_data->waitingForStart = false;
	}

  // If we are waiting for the start message to appear, don't bother
  // processing any other message type
  if (m_data->waitingForStart == true)
    {
      return SENTENCE_VALID;
    }
  
  // Parse the sentence.  Make sure the sentence has the correct
  // number of commas in it, otherwise the sentence is invalid.
  if (strcmp (sentenceType, "GGA") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_GGA_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseGGA (sentence);
    }
  else if (strcmp (sentenceType, "GLL") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_GLL_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseGLL (sentence);
    }
  else if (strcmp (sentenceType, "RMC") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_RMC_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseRMC (sentence);
    }
  else if (strcmp (sentenceType, "GSV") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_GSV_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseGSV (sentence);
    }
  else if (strcmp (sentenceType, "RRE") == 0)
    {
      //      if (countChars (sentence, ',', SENTENCE_RRE_COMMAS) < 0)
      //{
      //  return SENTENCE_INVALID;
      //}
      parseRRE (sentence);
    }
  else if (strcmp (sentenceType, "VTG") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_VTG_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseVTG (sentence);
    }
  else if (strcmp (sentenceType, "GST") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_GST_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseGST (sentence);
    }
  else if (strcmp (sentenceType, "ZDA") == 0)
    {
      if (countChars (sentence, ',', SENTENCE_ZDA_COMMAS) < 0)
        {
          return SENTENCE_INVALID;
        }
      parseZDA (sentence);
    }
  else
    {
      return SENTENCE_INVALID;
    }
  return SENTENCE_VALID;
}


/////////////////////////////////////////////////////////////////////////////
// Member functions for individual element parsing of sentences.

bool NMEAParser::parseDate (int& year, int& month, int& day,
                             const char* dateString) const
  // Purpose:
  //  This function parses a date string from an NMEA sentence in DDMMYY format
  //  and returns the year, month, and day values.
  // Parameters:
  //  int& year, int& month, int& year:
  //      Upon exit, these variables will contain the year, month, and day
  //      values specified in dateString, respectively.
  //  const char* dateString:
  //      The NMEA date string to parse.
  // Returns:
  //  true if the date string is in a valid format, false if not.
  // Notes:
  //  - NMEA sentences are *not* "Year 2000-compliant"{tm}; the software must
  //    correctly determine the year's century.
  //  - If this function returns false, then the variables year, month, and day
  //    are unaffected.
{
  // Date must be six characters.
  if (strlen (dateString) < unsigned (DATE_LEN))
    {
      return false;
    }
  
  long tempDate = atol (dateString);
  int tempYear, tempMonth, tempDay;
  tempYear  = tempDate - ((tempDate / 100) * 100);
  tempMonth = (tempDate - ((tempDate / 10000) * 10000)) / 100;
  tempDay   = tempDate / 10000;
  // Check to see if the date is valid.  (This function will accept
  // Feb 31 as a valid date; no check is made for how many days are in
  // each month of our whacked calendar.)
  if ((tempYear  >= 0 && tempYear  <= 99)
      && (tempMonth >= 1 && tempMonth <= 12)
      && (tempDay   >= 1 && tempDay   <= 31))
    {
      year = tempYear;
      month = tempMonth;
      day = tempDay;
      return true;
    }
  return false;
}


bool NMEAParser::parseDegrees (double& degrees, const char* degString) const
  // Purpose:
  //  This function converts a lat/lon string returned from an NMEA string into
  //  a numeric representation of that string, in degrees.  (A lat/lon string
  //  must be in the format DDMM.M(...) where D = degrees and M = minutes.)
  // Pre:
  //  The string degString must contain a number in the format DDMM.M(...).
  // Parameters:
  //  double& degrees:
  //      Upon exit, degrees will contain the numeric representation of the
  //      string passed to this function, in decimal degrees.
  //  const char* degString:
  //      Contains the string to convert.
  // Returns:
  //  - true if the conversion was successful.  If false is returned, either
  //    degString was not in one of those required formats, or the string data
  //    itself is invalid.  (For example, the string 23809.666 would not be
  //    valid, as the 238th degree does not exist.)
  //  - If this function returns false, then the parameter 'degrees' is
  //    unaffected.
{
  if (strlen (degString) == 0)
    {
      return false;
    }
  
  double tempPosition = atof (degString);
  double tempDegrees  = (double)((int)(tempPosition / 100.0));
  double tempMinutes  = (tempPosition - (tempDegrees * 100.0));
  tempPosition = tempDegrees + (tempMinutes / 60.0);
  
  if (tempPosition >= 0.0 && tempPosition <= 180.0)
    {
      degrees = tempPosition;
      return true;
    }
  return false;
}


bool NMEAParser::parseTime (int& hour, int& minute, int& second,
                             const char* timeString) const
  // Purpose:
  //  This function parses a time string from an NMEA sentence in HHMMSS.S(...)
  //  format and returns the hour, minute, and second values.
  // Parameters:
  //  int& hour, int& minute, int& second:
  //      Upon exit, these variables will contain the hour, minute, and second
  //      values specified in timeString, respectively.
  //  const char* timeString:
  //      The NMEA time string to parse.
  // Returns:
  //  true if the time string is in a valid format, false if not.
  // Notes:
  //  - Decimal second values are truncated.
  //  - If this function returns false, then the variables hour, minute, and
  //    second are unaffected.
{
  if (strlen (timeString) == 0)
    {
      return false;
    }
  
  long tempTime = atol (timeString);
  int tempHour   = tempTime / 10000;
  int tempMinute = (tempTime - ((tempTime / 10000) * 10000)) / 100;
  int tempSecond = tempTime - ((tempTime / 100) * 100);
  // Check to see if the time is valid.
  if ((tempHour   >= 0 && tempHour   <= 23)
      && (tempMinute >= 0 && tempMinute <= 59)
      && (tempSecond >= 0 && tempSecond <= 61))
    { // leap seconds
      hour   = tempHour;
      minute = tempMinute;
      second = tempSecond;
      return true;
    }
  return false;
}


/////////////////////////////////////////////////////////////////////////////
// parse And Validate member functions for NMEAParser.
//
// Each of these member functions parse a specified field from a sentence and
// updates the appropriate member variables in the NMEAData object.  If the
// data parsed is valid, the validation member variable associated the parsed
// value is set to true, otherwise it is set to false.

void NMEAParser::parseAndValidateAltitude (const char* field, const char unit)
  // Purpose:
  //  This function parses the altitude field of a sentence.
  // Parameters:
  //  const char* field:
  //      The altitude field of the sentence to parse.
  //  const char unit:
  //      The unit of altitude.  Valid values are 'f' (feet) and 'm' (meters).
  // Notes:
  //  The resulting altitude data is specified in feet.
{
  // Initially assume data is invalid.
  m_data->isValidAltitude = false;

  if (strlen (field) != 0) {
    if (unit == 'f' || unit == 'F') {
      // Altitude is in feet.
      m_data->altitude = atof (field) * FEET_TO_METERS;
      m_data->isValidAltitude = true;
    } else if (unit == 'm' || unit == 'M') {
      // Altitude is in meters.  Convert to feet.
      m_data->altitude = atof (field);
      m_data->isValidAltitude = true;
    }
  }
  // sj: logger.debug("isValidAltitude=%d; altitude=%f",
  // sj:             m_data->isValidAltitude, m_data->altitude);
}

void NMEAParser::parseAndValidateDate (const char* field)
  // Purpose:
  //  This function parses the date field of a sentence, in the format DDMMYY.
{
  // Initially assume data is invalid.
  m_data->isValidDate = false;

  if (strlen (field) != 0) {
    int year, month, day;
    if (parseDate (year, month, day, field) == true) {
      m_data->UTCYear  = year;
      m_data->UTCMonth = month;
      m_data->UTCDay   = day;
      m_data->isValidDate = true;
    }
  }
  // sj: logger.debug("isValidDate=%d; year=%d; month=%d; day=%d",
  // sj:             m_data->isValidDate, m_data->UTCYear, m_data->UTCMonth,
  // sj:             m_data->UTCDay);
}

void NMEAParser::parseAndValidateFixQuality (const char* field)
  // Purpose:
  //  This function parses the GPS fix quality field of a sentence.
{
  if (strlen (field) != 0) {
    m_data->isValidFixQuality = true;
    int fixQuality = atoi (field);
    if (fixQuality == 0) m_data->lastFixQuality = FIX_AUTONOMOUS;
    else if (fixQuality == 1) m_data->lastFixQuality = FIX_RTCM;
    else if (fixQuality == 2) m_data->lastFixQuality = FIX_CPD_FLOAT;
    else if (fixQuality == 3) m_data->lastFixQuality = FIX_CPD_FIXED;
    else if (fixQuality == 9) m_data->lastFixQuality = FIX_ALMANAC;
    else
      {
        // sj: logger.notice("unknown fix quality %d", fixQuality);
        m_data->lastFixQuality = FIX_AUTONOMOUS;
      }
  }
  // sj: logger.debug("lastFixQuality=%d", m_data->lastFixQuality);
}

void NMEAParser::parseAndValidateHdop (const char* field)
  // Purpose:
  //  This function parses the HDOP (horizontal dilution of precision) field of
  //  a sentence.
{
  if (strlen (field) != 0) {
    m_data->hdop = atof (field);
    m_data->isValidHdop = true;
  } else {
    m_data->isValidHdop = false;
  }
  // sj: logger.debug("isValidHdop=%d; hdop=%f", m_data->isValidHdop, m_data->hdop);
}

void NMEAParser::parseAndValidateNStd (const char* field)
  // Purpose:
  //  This function parses the NSTD (1 standard deviation of position estimate in northern
  // direction)
{
  if (strlen (field) != 0) {
    m_data->nStd = atof (field);
    m_data->isValidNStd = true;
  } else {
    m_data->isValidNStd = false;
  }
  // sj: logger.debug("isValidNStd=%d; nStd=%f", m_data->isValidNStd, m_data->nStd);
}


void NMEAParser::parseAndValidateEStd (const char* field)
  // Purpose:
  //  This function parses the ESTD (1 standard deviation of position estimate in eastern
  // direction)
{
  if (strlen (field) != 0) {
    m_data->eStd = atof (field);
    m_data->isValidEStd = true;
  } else {
    m_data->isValidEStd = false;
  }
}

void NMEAParser::parseAndValidateZStd (const char* field)
  // Purpose:
  //  This function parses the ZSTD (1 standard deviation of position estimate in altitude)
{
  if (strlen (field) != 0) {
    m_data->zStd = atof (field);
    m_data->isValidZStd = true;
  } else {
    m_data->isValidZStd = false;
  }
  // sj: logger.debug("isValidEStd=%d; eStd=%f", m_data->isValidEStd, m_data->eStd);
}

void NMEAParser::parseAndValidateLat (const char* field, const char hem)
  // Purpose:
  //  This function parses the latitude field of a sentence in the format
  //  DDMM.M(...).
  // Parameters:
  //  const char* field:
  //      The latitude field of the sentence to parse.
  //  const char hem:
  //      The hemisphere that contains the location.  Valid values are 'N' and
  //      'S'.
  // Notes:
  //  - If the latitude is in the southern hemisphere, the latitude member
  //    variable will be negative.  (e.g., 4000.000 S will be stored as -40.0.)
  //  - If the latitude field does not exist within a sentence, the fix
  //    quality variable, m_data->lastFixQuality, is set to FIX_AUTONOMOUS.
{
  // Initially assume data is invalid.
  m_data->isValidLat = false;

  if (strlen (field) != 0) {
    // GPS lat/lon data has been received.
    // Set the fix quality to "GPS navigation."  This is because some
    // GPS's may not send GGA sentences; therefore the last fix quality
    // would never get set.
    if (m_data->lastFixQuality == FIX_AUTONOMOUS) {
      m_data->lastFixQuality = FIX_RTCM;
    }
    m_data->hasCoordEverBeenValid = true;
    double degree;
    if (parseDegrees (degree, field) == true) {
      if (hem == 'N') {
				// Northern hemisphere.
        m_data->lat = degree;
        m_data->isValidLat = true;
      } else if (hem == 'S') {
				// Southern hemisphere, so make latitude negative.
        m_data->lat = -degree;
        m_data->isValidLat = true;
      }
    }
  } else {
    m_data->lastFixQuality = FIX_AUTONOMOUS;
  }
  // sj: logger.debug("isValidLat=%d; lat=%f", m_data->isValidLat, m_data->lat);
}


void NMEAParser::parseAndValidateLon (const char* field, const char hem)
  // Purpose:
  //  Same as parseAndValidateLat(), but the longitude is in the format
  //  DDDMM.M(...).
  // Notes:
  //  - The valid values for the hem parameter are 'E' and 'W'.
  //  - If the longitude is in the western hemisphere, the longitude member
  //    variable will be negative.  (e.g., 4000.000 W will be stored as -40.0.)
  //  - If the latitude field does not exist within a sentence, the last fix
  //    quality variable, m_data->lastFixQuality, is set to FIX_AUTONOMOUS.
{
  // Initially assume data is invalid.
  m_data->isValidLon = false;

  if (strlen (field) != 0) {
    // GPS lat/lon data has been received.
    // Set the fix quality to "GPS navigation."  This is because some
    // GPS's may not send GGA sentences; therefore the last fix quality
    // would never get set.
    if (m_data->lastFixQuality == FIX_AUTONOMOUS) {
      m_data->lastFixQuality = FIX_RTCM;
      m_data->hasCoordEverBeenValid = true;
    }
    double degree;
    if (parseDegrees (degree, field) == true) {
      if (hem == 'E') {
				// Eastern hemisphere.
        m_data->lon = degree;
        m_data->isValidLon = true;
      } else if (hem == 'W') {
				// Western hemisphere, so make longitude negative.
        m_data->lon = -degree;
        m_data->isValidLon = true;
      }
    }
  } else {
    m_data->lastFixQuality = FIX_AUTONOMOUS;
  }
  // sj: logger.debug("isValidLon=%d; lon=%f", m_data->isValidLon, m_data->lon);
}


void NMEAParser::parseAndValidateMagVariation (const char* field,
                                                const char direction)
  // Purpose:
  //  This function parses the magnetic variation field of a sentence, in
  //  relation to true north.
  // Parameters:
  //  const char* field:
  //      The magnetic variation field of the sentence to parse, in degrees.
  //  const char direction:
  //      The direction of the field in relation to true north.  Valid values
  //      are 'E' and 'W'.
  // Notes:
  //  If the magnetic variation points west of true north, the magnetic
  //  variation variable will be negative.  (e.g., 020.3 W will be stored as
  //  -20.3.)
{
  // Initially assume data is invalid.
  m_data->isValidMagVariation = false;

  if (strlen (field) != 0) {
    double degree = atof (field);
    if (degree >= 0.0 && degree <= 360.0) {
      if (direction == 'E') {
        m_data->magVariation = degree;
        m_data->isValidMagVariation = true;
      } else if (direction == 'W') {
        m_data->magVariation = -degree;
        m_data->isValidMagVariation = true;
      }
    }
  }
  // sj: logger.debug("isValidMagVariation=%d; magVariation=%f",
  // sj:              m_data->isValidMagVariation, m_data->magVariation);
}


void NMEAParser::parseAndValidateSpeed (const char* field)
  // Purpose:
  //  This function parses the speed field of a sentence.
{
  if (strlen (field) != 0) {
    m_data->speed = atof (field);
    m_data->isValidSpeed = true;
  } else {
    m_data->isValidSpeed = false;
  }
  // sj: logger.debug("isValidSpeed=%d; speed=%f",
  // sj:              m_data->isValidSpeed, m_data->speed);
}


void NMEAParser::parseAndValidateTime (const char* field)
  // Purpose:
  //  This function parses the date field of a sentence, in the format
  //  HHMMSS.S(...), except the decimal second values are truncated.
{
  // Initially assume data is invalid.
  m_data->isValidTime = false;

  if (strlen (field) != 0) {
    int hour, minute, second;
    if (parseTime (hour, minute, second, field) == true) {
      m_data->UTCHour   = hour;
      m_data->UTCMinute = minute;
      m_data->UTCSecond = second;
      m_data->isValidTime = true;
    }
  }
  // sj: logger.debug("isValidTime=%d; hour=%d; minute=%d; second=%d",
  // sj:              m_data->isValidTime, m_data->UTCHour, m_data->UTCMinute,
  // sj:             m_data->UTCSecond);
}

void NMEAParser::parseAndValidateTrack (const char* field)
  // Purpose:
  //  This function parses the track field of a sentence.
{
  // Initially assume data is invalid.
  m_data->isValidTrack = false;

  if (strlen (field) != 0) {
    double track = atof (field);
    if (track >= 0.0 && track <= 360.0) {
      m_data->track = track;
      m_data->isValidTrack = true;
    }
  }
  // sj: logger.debug("isValidTrack=%d; track=%f", m_data->isValidTrack, m_data->track);
}


/////////////////////////////////////////////////////////////////////////////
// Miscellaneous member functions

bool NMEAParser::getNextField (char* data, const char* sentence,
                                uint_& currentPos) const
  // Purpose:
  //  This function retrieves the next field in the NMEA sentence.  A field is
  //  defined as the text between two delimiters (in this case of NMEA
  //  sentences, a delimiter is a comma character.)
  // Pre:
  //  The specified sentence is valid.  (Before calling this function, call the
  //  member functions IsCorrectChecksum() and ValidSentenceType(), passing the
  //  sentence to those functions.)
  // Parameters:
  //  char* data:
  //      Upon exit, this string will contain the contents of the next field
  //      in the sentence.
  //  const char* sentence:
  //      The NMEA sentence to parse.
  //  uint_& currentPos:
  //      Determines the initial position within the NMEA sentence in which to
  //      parse.  This function will grab all of the characters from
  //      currentPos all the way to the character before the comma delimiter.
  //      Upon exit, currentPosition will point to the next field in the string.
  //      Note that the comma is not included in the field data.
  // Returns:
  //      true if there are more fields to parse, false if not.
  // Notes:
  //      To grab all of the fields, you can iteratively call GetNextData()
  //      using the same variable that is passed as currentPosition, until
  //      GetNextData() returns false.
{
  int srcPos = currentPos;
  int dstPos = 0;
  char currentChar = sentence[srcPos];

  // The delimiter character is the comma.
  while ((currentChar != '\0'  ) && (currentChar != ',')
         && (currentChar != '\x0d') && (currentChar != '*')) {

    data[dstPos++] = currentChar;
    currentChar = sentence[++srcPos];
  }
  data[dstPos] = '\0';

  if (currentChar == ',') {
    // Next data field to parse will be past the comma.
    currentPos = srcPos + 1;
    return true;
  } else {
    // No more characters in the string to parse; this function has
    // arrived at the end of the string.
    return false;
  }
}


bool NMEAParser::isCorrectChecksum (const char* sentence) const
  // Purpose:
  //  This function calculates the sentence's checksum and compares it with the
  //  checksum in the sentence.
  // Pre:
  //  The NMEA sentence is valid (ValidSentenceStructure() must be called with
  //  this sentence before calling this function; that function must return
  //  true.)
  // Returns:
  //  true if the checksum is valid or there is no checksum in the sentence.
  //  (It is not necessary to have a device append a checksum to a sentence.)
  //  Otherwise this function returns false.
  // Notes:
  //  The checksum in the sentence occurs after the * character.
{
  // Check all characters between the initial '$' and the last "*" in the
  // sentence and XOR them together.

  int charPos = 1; // start past the initial '$'.
  char currentChar = sentence[charPos];
  uint8_ checksum = 0;

  while (currentChar != '*' && currentChar != '\0') {
    checksum ^= (uint8_)currentChar;
    currentChar = sentence[++charPos];
  }

  // If no checksum exists (this function has reached the end of the string
  // without finding one), the sentence is good.
  if (sentence[charPos + 1] == '\0') return true;

  // Convert last two hex characters (the checksum) in the sentence with
  // the checksum this function has generated.
  char firstDigit = sentence[charPos + 1];
  char lastDigit  = sentence[charPos + 2];
  if (  (firstDigit <= '9' ? firstDigit - '0': (firstDigit - 'A') + 10) * 16
        + (lastDigit  <= '9' ? lastDigit  - '0': (lastDigit  - 'A') + 10)
        == checksum) {
    return true;
  } else {
    return false;
  }

  return true;
}

bool NMEAParser::isValidSentenceType (const char* sentence) const
{
  if (strlen (sentence) < 6) 
    {
      return false;
    }
  
  if (sentence[0] != '$')
    {
      return false;
    }

  return isKnownSentenceType(sentence);
}

bool NMEAParser::isKnownSentenceType (const char* sentence) const
  // Purpose:
  //  This function determines whether this is a valid NMEA sentence that this
  //  class can support.
  // Notes:
  //  See the header file for a list of sentences supported by this class.
{
  // Get the three letters after the '$xx'; this is the type of
  // sentence.  (Note the xx is the type of device which is sending
  // the string.  For example, GP = GPS, etc.)
  char sentenceType[4];
  memcpy (sentenceType, &(sentence[3]), 3);
  sentenceType[3] = '\0';
  
  return ((strcmp (sentenceType, "ZDA") == 0) 
          || (strcmp (sentenceType, "GGA") == 0) 
          || (strcmp (sentenceType, "GLL") == 0) 
          || (strcmp (sentenceType, "RMC") == 0) 
          || (strcmp (sentenceType, "GSV") == 0) 
          || (strcmp (sentenceType, "GST") == 0) 
          || (strcmp (sentenceType, "VTG") == 0)
          || (strcmp (sentenceType, "RRE") == 0));
}

int NMEAParser::countChars (const char* string, char charToCount, uint_ charCount) const
  // Purpose:
  //  This function counts the number of specified occurrences of the
  //  specified characters and compares to the number of characters that is
  //  expected.
  // Parameters:
  //  const char* string:
  //      The string to check.
  //  char charToCount:
  //      The character to count.
  //  uint_ charCount:
  //      The number of the characters specified by charToCount that is expected
  //      to be contained within that string.
  // Returns:
  //  0 if the number of specified characters in the sentence matches charCount.
  //  1 if the number of specified characters in the sentence is less than
  //  charCount.
  //  -1 if the number of specified characters in the sentence is greater than
  //  charCount.
{
  size_t stringSize = strlen (string);
  size_t currentCharCount = 0;
  const char* currentChar = string;
  for (size_t i = 0; i < stringSize; i++) {
    if (*currentChar++ == charToCount) ++currentCharCount;
  }
  if (currentCharCount > charCount) {
    return 1;
  } else if (currentCharCount < charCount) {
    return -1;
  } else {
    return 0;
  }
}
