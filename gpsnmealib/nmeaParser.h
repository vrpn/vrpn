/*
 * This class was adapted from the GPSThing CNmeaParser class.
 * Originally written by Jason Bevins
 */

#ifndef __NMEAPARSER_HPP
#define __NMEAPARSER_HPP

#if 1

typedef unsigned char  uint8_;
typedef unsigned short uint16_;
typedef unsigned int   uint32_;
typedef unsigned int   uint_;

#else

// Get standard types (uint8_, etc.)
#include <cc++/config.h>

#endif

#include <string.h>

// sj: #include <log4cpp/Category.hh>

struct SatData
{
  SatData ()
  {
    prn       = 0;
    elevation = 0;
    azimuth   = 0;
    strength  = 0;
  }

  SatData (uint8_ prn_, uint16_ elevation_, uint16_ azimuth_,
            uint16_ strength_)
  {
    prn       = prn_;
    elevation = elevation_;
    azimuth   = azimuth_;
    strength  = strength_;
  }

  uint16_ prn;       // Satellite's ID.
  uint16_ elevation; // Elevation of satellite, in degrees.
  uint16_ azimuth;   // Azimuth of satellite, in degrees.
  uint16_ strength;  // Signal strength of satellite.
};

struct RangeResidualData
{
  RangeResidualData()
  {
    prn = 0;
    residual = 0;
  }
  
  RangeResidualData(uint8_ prn_, double residual_)
  {
    prn = prn_;
    residual = residual_;
  }
  
  uint8_ prn;
  double residual;
};

const double METERS_TO_FEET = 3.280839895013;
const double FEET_TO_METERS = 1 / METERS_TO_FEET;
const double KM_TO_NM = 1.853;
const double NM_TO_KM = 1 / KM_TO_NM;
const double KM_TO_MI = FEET_TO_METERS * 5.28;
const double MI_TO_KM = 1 / KM_TO_MI;

// GPS coordinate fix quality.
enum GPS_FIX_QUALITY
{
  FIX_AUTONOMOUS = 0,
  FIX_RTCM = 1,
  FIX_CPD_FLOAT = 2,
  FIX_CPD_FIXED = 3,
  FIX_ALMANAC = 9
};

// Sentence parsing status.
enum SENTENCE_STATUS
{
  SENTENCE_VALID=0,       // Sentence parsed is valid.
  SENTENCE_INVALID,      // Sentence parsed has invalid data.
  SENTENCE_BAD_CHECKSUM,  // Sentence parsed has a bad checksum.
  SENTENCE_UNKNOWN       // Sentence is of unknown type
};

// Number of commas in each sentence.  In order for a sentence to be valid,
// it must have a specified number of commas.
enum SENTENCE_COMMA_SIZES
{
  SENTENCE_ZDA_COMMAS = 6,
  SENTENCE_GGA_COMMAS = 14,
  SENTENCE_GLL_COMMAS = 6,
  SENTENCE_RMC_COMMAS = 11,
  SENTENCE_GSV_COMMAS = 19,
  SENTENCE_VTG_COMMAS = 7,
  SENTENCE_GST_COMMAS = 8,
  SENTENCE_RRE_COMMAS = 4
};

// Maximum size of an NMEA sentence (plus the NULL character.)
const int MAX_SENTENCE_SIZE = 1024;

// No GPS I'm aware of can track more than 12 satellites.
const uint8_ MAX_SATS = 12;

// Data class stored with the parser.  To extract the data parsed from the
// parser, pass an object of this class to the parser.
// NOTE! NMEA sentences are not "Year 2000-compliant"{tm}
struct NMEAData
{
  NMEAData ();

  virtual ~NMEAData()
  {
  }

  virtual void reset ();

  // Data retrieved from the NMEA sentences.
  double lat;          // Latitude, in degrees (positive=N, negative=S)
  double lon;          // Longitude, in degrees (positive=E, negative=W)
  double altitude;     // Altitude, in feet
  double speed;        // Speed, in knots
  double track;        // Current track, in degrees.
  double magVariation; // Magnetic variation, in degrees.
  double hdop;         // Horizontal dilution of precision.
  double nStd;         // North standard deviation.
  double eStd;         // East standard deviation.
  double zStd;         // Altitude standard deviation.
  double hStd;         // Horizonal standard deviation.
  uint_ numSats;         // Number of satellites in the sky.
  int UTCYear;         // GPS Date (UTC), year part
  int UTCMonth;        // GPS Date (UTC), month part  
  int UTCDay;          // GPS Date (UTC), day part
  int UTCHour;         // GPS Time (UTC), hour part.
  int UTCMinute;       // GPS Time (UTC), minute part  
  int UTCSecond;       // GPS Time (UTC), second part
  SatData satData[MAX_SATS];

  RangeResidualData rrData[MAX_SATS];

  // Quality of last fix:
  // 0 = invalid, 1 = GPS fix, 2 = DGPS fix.
  GPS_FIX_QUALITY lastFixQuality;

  // Validity of data parsed.
  bool isValidLat;          // Latitude
  bool isValidLon;          // Longitude
  bool isValidAltitude;     // Altitude
  bool isValidSpeed;        // Speed
  bool isValidDate;         // Date
  bool isValidTime;         // Time
  bool isValidTrack;        // Track
  bool isValidMagVariation; // Magnetic variation
  bool isValidHdop;         // Horizontal dilution of precision
  bool isValidNStd;
  bool isValidEStd;
  bool isValidZStd;
  bool isValidSatData;      // Satellite data
  bool isValidRangeResidualData;      // Satellite data
  bool isValidHStd;
  bool isValidFixQuality;

  // Has a valid coordinate ever been sent over the serial port?
  bool hasCoordEverBeenValid;

  // Flag indicates if we are waiting for the frame to start
  bool waitingForStart;

  // Whether we have seen a particular message since the data was
  // reset
  bool seenZDA;
  bool seenGGA;
  bool seenGLL;
  bool seenRMC;
  bool seenGSV;
  bool seenGST;
  bool seenVTG;
  bool seenRRE;
};

class NMEAParser
{
 public:
  NMEAParser (/* sj: log4cpp::Category& l*/);
  virtual ~NMEAParser ();
  SENTENCE_STATUS parseSentence (const char* sentence);

  void setStartSentence(const char *sentence)
  {
	  strcpy(startSentence, sentence);
  }

  void getData (NMEAData& data) const
  {
    data = *m_data;
  }

  NMEAData& getData()
  {
  return *m_data;
  }

  void reset ()
  {
    m_data->reset ();
  }
 
  bool isValidSentenceType (const char* sentence) const;

  bool isCorrectChecksum (const char* sentence) const;

protected:
  NMEAParser (/* sj: log4cpp::Category& l,*/ NMEAData* data);
  
  bool parseDegrees (double& degrees, const char* degString) const;
  bool parseDate (int& year, int& month, int& day,
                  const char* dateString) const;
  bool parseTime (int& hour, int& minute, int& second,
                  const char* timeString) const;
  void parseAndValidateAltitude    (const char* field, const char unit);
  void parseAndValidateDate        (const char* field);
  void parseAndValidateFixQuality  (const char* field);
  void parseAndValidateLat         (const char* field, const char hem);
  void parseAndValidateLon         (const char* field, const char hem);
  void parseAndValidateHdop        (const char* field);
  void parseAndValidateSpeed       (const char* field);
  void parseAndValidateNStd        (const char* field);
  void parseAndValidateEStd        (const char* field);
  void parseAndValidateZStd        (const char* field);
  void parseAndValidateMagVariation(const char* field,
                                    const char direction);
  void parseAndValidateTime  (const char* field);
  void parseAndValidateTrack (const char* field);

  bool getNextField(char* data, const char* sentence,
                    uint_& currentPosition) const;

  int countChars(const char* sentence, char charToCount,
                 uint_ charCount) const;

  virtual bool isKnownSentenceType (const char* sentence) const;
  virtual SENTENCE_STATUS parseValidSentence (const char* sentence);

  NMEAData* m_data;

  // the sentence that marks the beginning of the set of packets
  char startSentence[16];

  // Needed for parsing the GSV sentence.
  int m_lastSentenceNumber;// Which sentence number was the last one?
  int m_numSentences;      // Number of sentences to process.
  int m_numSatsExpected;   // Number of satellites expected to parse.
  int m_numSatsLeft;       // Number of satellites left to parse.
  int m_satArrayPos;       // Array position of the next sat entry.		
  SatData m_tempSatData[MAX_SATS];


  // The logging category
  // sj: log4cpp::Category& logger;

private:
  void parseZDA(const char* sentence);
  void parseGGA (const char* sentence);
  void parseGLL (const char* sentence);
  void parseRMC (const char* sentence);
  void parseGSV (const char* sentence);
  void parseGST (const char* sentence);
  void parseVTG (const char* sentence);
  void parseRRE (const char* sentence);
};

#endif


