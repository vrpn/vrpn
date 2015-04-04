// LatLonCoord.cpp: Implementation of the CLatLonCoord class.
// 
// Written by Jason Bevins in 1998.  File is in the public domain.
//

#include "gpsnmealib/typedCoord.h"      // for TypedCoord
#include "latLonCoord.h"

#ifdef sgi
#include <assert.h>
#include <math.h>
#include <stdio.h>
#else
#include <cassert>                      // for assert
#include <cmath>                        // for fabs
#include <cstdio>                       // for sprintf
#endif

// Format strings for the degree-to-string conversion functions.
// Note that east/west (EW) has a different set of format strings as north/
// south (NS), as integer degrees in east/west can have up to three digits
// instead of two.
static const char formatD_NS[]   = "%c %02d.%05d*";
static const char formatDM_NS[]  = "%c %02d*%02d.%03d'";
static const char formatDMS_NS[] = "%c %02d*%02d'%02d.%01d\x22";
static const char formatD_EW[]   = "%c %03d.%05d*";
static const char formatDM_EW[]  = "%c %03d*%02d.%03d'";
static const char formatDMS_EW[] = "%c %03d*%02d'%02d.%01d\x22";


/////////////////////////////////////////////////////////////////////////////
// LatLonCoord construction/destruction

LatLonCoord::LatLonCoord (): TypedCoord ()
{
  // Set the default output coordinate format to decimal degrees.
  m_latLonFormat = FORMAT_D;
}


LatLonCoord::LatLonCoord (double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;

  // Set the default output coordinate format to decimal degrees.
  m_latLonFormat = FORMAT_D;
}


LatLonCoord::LatLonCoord (const LatLonCoord& other)
{
  copyLatLonCoord (other);
}


LatLonCoord::LatLonCoord (const TypedCoord& other)
{
  m_latLonFormat = FORMAT_D;
  copyOtherCoord (other);
}


/////////////////////////////////////////////////////////////////////////////
// LatLonCoord operators

LatLonCoord& LatLonCoord::operator= (const LatLonCoord& other)
{
  copyLatLonCoord (other);
  return *this;
}


LatLonCoord& LatLonCoord::operator= (const TypedCoord& other)
{
  copyOtherCoord (other);
  return *this;
}


/////////////////////////////////////////////////////////////////////////////
// LatLonCoord members

void LatLonCoord::copyLatLonCoord (const LatLonCoord& other)
  // Purpose:
  //  This function copies the lat/lon coordinate of the specified object and
  //  assigns that coordinate to this object.
{
  m_lat = other.m_lat;
  m_lon = other.m_lon;
}


void LatLonCoord::copyOtherCoord (const TypedCoord& other)
  // Purpose:
  //  This function copies the coordinate of the specified object, converts
  //  the coordinate to the coordinate type of this object, then assigns the
  //  converted coordinate to this object.
{
  // Convert the right hand side coordinate to lat/lon; then store these
  // lat/lon values in this coordinate.
  other.getLatLonCoord (m_lat, m_lon);
}


const std::string& LatLonCoord::createCoordString(std::string& coordString) const
  // Purpose:
  //  This function creates a formatted coordinate string containing this
  //  object's coordinate value.
  // Parameters:
  //  std::string& coordString:
  //      Upon exit, this parameter will contain the generated coordinate
  //      string
  // Returns:
  //  A reference to the generated coordinate string.
  // Notes:
  //  The string can be in one of three different formats (degrees, degrees and
  //  decimal minutes, or degrees, minutes, and decimal seconds.)  Set the
  //  format by calling the SetCurrentLatLonFormat() member function.
{
  std::string latString;
  std::string lonString;
  // Create the formatted lat/lon strings; the format is specified by the
  // m_currentLatLonFormat member variable
  degreeToString (latString, m_lat, HEMISPHERE_NS);
  degreeToString (lonString, m_lon, HEMISPHERE_EW);
  coordString = latString + ", " + lonString;
  return coordString;
}


void LatLonCoord::createXYCoordStrings (std::string& xString, std::string& yString)
  const
  // Purpose:
  //  This function generates two strings: a string containing the longitude
  //  (stored in the parameter xString), and a string containing the latitude
  //  (stored in the parameter yString.)
  // Notes:
  //  The string can be in one of three different formats (degrees, degrees and
  //  decimal minutes, or degrees, minutes, and decimal seconds); Set the
  //  format by calling the SetCurrentLatLonFormat() member function.
{
  degreeToString (xString, m_lon, HEMISPHERE_EW);
  degreeToString (yString, m_lat, HEMISPHERE_NS);
}


const std::string& LatLonCoord::degreeToD (std::string& output, double degree,
                                       HEMISPHERE_DIRECTION direction) const
  // Purpose:
  //  This function converts a value specified in decimal degrees to a string
  //  formatted as H DDD.DDDDD*.
  // Pre:
  //  -180.0 <= degree <= 180.0
  // Parameters:
  //  std::string& output:
  //      Upon exit, this parameter contains the converted string.
  //  double degree:
  //      The degree value to convert.
  //  HEMISPHERE_DIRECTION direction:
  //      The direction of the hemisphere, either north/south or east/west.
  //      This value determines the hemisphere character in the converted
  //      string.
  // Returns:
  //  The converted string.
{
  char hemisphereChar = getHemisphereChar (degree, direction);
  double positiveDegrees = fabs (degree);

  // A "unit" is measured in 1/100000ths of a degree; this is the smallest
  // possible unit when coordinates are expressed as DDD.DDDDD.
  // (Calculating the degree and the 1/100000th degree parts as integers
  // avoid rounding errors in the final result.)
  int units = (int)((positiveDegrees * 100000.0) + 0.5);
  int intDegrees = units / 100000;
  int int100ThousandthDegrees = units % 100000;

  const char* pFormatString;
  char tempString[1000];
  if (direction == HEMISPHERE_NS) {
    pFormatString = formatD_NS;
  } else {
    pFormatString = formatD_EW;
  }
  sprintf(tempString, pFormatString, hemisphereChar, intDegrees,
          int100ThousandthDegrees);
  output = tempString;
  return output;
}


const std::string& LatLonCoord::degreeToDM (std::string& output, double degree,
                                            HEMISPHERE_DIRECTION direction) const
  // Purpose:
  //  Same as DegreeToD, except the string is formatted as H DDD*MM.MMM'.
  // Pre:
  //  -180.0 <= degree <= 180.0
{
  char hemisphereChar = getHemisphereChar (degree, direction);
  double positiveDegrees = fabs (degree);

  // A "unit" is measured in 1/1000ths of a minute (or 1/60000 of a degree);
  // this is the smallest possible unit when coordinates are expressed as
  // DDD MM.MMM.  (Calculating the degree, minute, and the 1/1000th minute
  // parts as integers avoid rounding errors in the final result.)
  int units = (int)((positiveDegrees * 60000.0) + 0.5);
  int intDegrees = units / 60000;
  int intDegreesInUnits = intDegrees * 60000;
  int intMinutes = (units - intDegreesInUnits) / 1000;
  int intThousandthMinutes = (units - intDegreesInUnits) % 1000;

  const char* pFormatString;
  char tempString[1000];
  if (direction == HEMISPHERE_NS) {
    pFormatString = formatDM_NS;
  } else {
    pFormatString = formatDM_EW;
  }
  sprintf(tempString, pFormatString, hemisphereChar,
          intDegrees, intMinutes, intThousandthMinutes);
  output = tempString;
  return output;
}


const std::string& LatLonCoord::degreeToDMS (std::string& output, double degree,
                                             HEMISPHERE_DIRECTION direction) const
  // Purpose:
  //  Same as DegreeToD, except the string is formatted as H DDD*MM'SS.S".
  // Pre:
  //  -180.0 <= degree <= 180.0
{
  char hemisphereChar = getHemisphereChar (degree, direction);
  double positiveDegrees = fabs (degree);

  // A "unit" is measured in 1/10ths of a second (or 1/36000 of a degree);
  // this is the smallest possible unit when coordinates are expressed as
  // DDD MM SS.S.  (Calculating the degree, minute, second, and the 1/10th
  // second parts as integers will avoid rounding errors in the final
  // result.)
  int units = (int)((positiveDegrees * 36000.0) + 0.5);
  int intDegrees = units / 36000;
  int intDegreesInUnits = intDegrees * 36000;
  int intMinutes = (units - intDegreesInUnits) / 600;
  int intMinutesInUnits = intMinutes * 600;
  int intSeconds = (units - intDegreesInUnits - intMinutesInUnits) / 10;
  int intTenthSeconds = (units - intDegreesInUnits - intMinutesInUnits) % 10;


  const char* pFormatString;
  char tempString[1000];
  if (direction == HEMISPHERE_NS) {
    pFormatString = formatDMS_NS;
  } else {
    pFormatString = formatDMS_EW;
  }
  sprintf(tempString, pFormatString, hemisphereChar,
          intDegrees, intMinutes, intSeconds, intTenthSeconds);
  output = tempString;
  return output;
}


const std::string& LatLonCoord::degreeToString (std::string& output, double degree,
                                                HEMISPHERE_DIRECTION direction) const
  // Purpose:
  //  This function converts a value specified in decimal degrees to a formatted
  //  string.
  // Pre:
  //  -180.0 <= degree <= 180.0
  // Parameters:
  //  std::string& output:
  //      Upon exit, this parameter contains the converted string.
  //  double degree:
  //      The degree value to convert.
  //  HEMISPHERE_DIRECTION direction:
  //      The direction of the hemisphere, either north/south or east/west.
  //      This value determines the hemisphere character in the converted
  //      string.
  // Returns:
  //  The converted string.
  // Notes:
  //  If formatType is FORMAT_D, the returned string format is H DDD.DDDDD*.
  //  If formatType is FORMAT_DM, the returned string format is H DDD*MM.MMM'.
  //  If formatType is FORMAT_DMS, the returned string format is H DDD*MM'SS.S".
{
  switch (m_latLonFormat) {
  case FORMAT_DMS:
    return degreeToDMS (output, degree, direction);
    break;
  case FORMAT_DM:
    return degreeToDM (output, degree, direction);
    break;
  case FORMAT_D:
    return degreeToD (output, degree, direction);
    break;
  default:
    assert(false);
    return output;  // needed to stop compiler warning.
  }
}


void LatLonCoord::getXYCoord (double& x, double& y) const
  // Purpose:
  //  This function returns the (x, y) coordinate stored within this object.
  //  In this class, the x coordinate is the longitude and the y coordinate is
  //  the latitude.
{
  x = m_lon;
  y = m_lat;
}


char LatLonCoord::getHemisphereChar (double degree,
                                     HEMISPHERE_DIRECTION direction) const
  // Purpose:
  //  This function determine the appropriate hemisphere character ('N', 'S',
  //  'E', 'W') given the degree value and the hemisphere direction (north/south
  //  or east/west).
  // Returns:
  //  'N' if direction is HEMISPHERE_NS and degree is positive.
  //  'S' if direction is HEMISPHERE_NS and degree is negative.
  //  'E' if direction is HEMISPHERE_EW and degree is positive.
  //  'W' if direction is HEMISPHERE_EW and degree is negative.
{
  if (direction == HEMISPHERE_NS) {
    if (degree >= 0.0) {
      return 'N';
    } else {
      return 'S';
    }
  } else {
    if (degree >= 0.0) {
      return 'E';
    } else {
      return 'W';
    }
  }
}



