// TypedCoord.h: Definition of the CTypedCoord abstract base class.
//
// The CTypedCoord class is an abstract base class whose derived classes
// represents a WGS84 coordinate with a specific type.  For example, a UTM
// coordinate class can be derived from the CTypedCoord class.  These classes
// store a coordinate in the class' coordinate type.
//
// Since a GPS uses lat/lon format for its coordinates, and distance/bearing
// formulas require lat/lon coordinates, CTypedCoord contains a latitude and
// a longitude member variable; member functions exist for converting between
// the lat/lon coordinate and the class' coordinate type.
//
// Conversion of coordinates from lat/lon to another format can be very
// computationally intensive, especially on machines without FPUs.  Because
// of this, it is encouraged that derived classes use lazy evaluation when
// returning coordinates, so the conversions take place only when necessary.
// Because of the lazy evaluation, derived classes should not access the
// coordinate member variables directly; use the GetLatLonCoord() and the
// SetLatLonCoord() member functions to access and/or modify these variables.
//
// Since this is an abstract base class, you *must* override the following
// member functions in your derived classes:
// - GetCoordType(): Override this member function to return the type of this
//   object.  Each derived class from CTypedCoord has a unique type ID that
//   specified the type of this object.  Add a value to the enumerated type
//   COORD_TYPE when deriving a new class, and return this value from this
//   function.  The COORD_TYPE enumerated type is found elsewhere in this
//   project.
// - CreateCoordString(): Override this member function to create a formatted
//   coordinate std::string using the coordinate within the class.
// - CreateXYCoordStrings(): Override this member function to create two
//   formatted std::strings containing the x and y coordinate values;
// - GetXYCoord(): Override this member function to return the (x, y)
//   coordinate values.  This function is useful when the other data (e.g.,
//   zones, etc.) are not needed.
// - CreateDisplayStrings: Override this function to create four std::strings that
//   can be used for display purposes.  The four std::strings contain coordinate
//   data; these std::strings are displayed at the top left, top right, bottom
//   left, and bottom right of a display.  For example:
//   A lat/lon coordinate class may create std::strings as follows:
//     - top left:     North/south hemisphere character
//     - top right:    Latitude
//     - bottom left:  East/west hemisphere character
//     - bottom right: Longitude
//   This would create a display as follows:
//     N   48*25'33.3"
//     W  123*20'09.8"
//   A UTM coordinate class may create std::strings as follows:
//     - top left:     UTM zone (e.g., "10U")
//     - top right:    UTM easting
//     - bottom left   A std::string describing the coordinate type ("UTM")
//     - bottom right: UTM northing
//   This would create a display as follows:
//     10U   475139
//     UTM  5363695
//   It is up to the calling function to write the std::strings to a display.
//
// Member functions that can (and should) be overridden include:
// - SetLatLonCoord(): Override this member function to set the current lat/
//   lon coordinate within this class and convert the lat/lon coordinate to
//   the appropriate coordinate type so it can be stored within this class.
// - GetLatLonCoord(): Override this member function so that the lat/lon
//   coordinate can be returned by converting the class' coordinate from the
//   class' coordinate type.
// - operator=(): Override this operator to implement assignment operations.
//   Two operator=() functions should be created as follows:
//   - An operator=() function that takes a parameter of type CTypedCoord&.
//     Use the GetLatLonCoord() function to convert the right hand side
//     coordinate from its current type to a pair of lat/lon values, then
//     call this class' SetLatLonCoord() function to convert the lat/lon pair
//     to a coordinate of this class' type.
//   - An operator=() function that takes a parameter of a reference to an
//     object as the same type as the object.  Simply copy all member
//     variables from the right hand side object to this object.
//
// Using coordinate objects derived from this base class, it is simple to
// perform such operations as:
// - Finding the distance between two coordinate objects with different
//   coordinate types.
// - Assigning the coordinates of an object of one coordinate type to another
//   object with a different coordinate type.
//
// [TO DO: Allow for other map datums.  For now, these coordinates are using
// the WGS84 datum.]
//
// Written by Jason Bevins in 1998.  File is in the public domain.
//

#ifndef __TYPED_COORD_HPP
#define __TYPED_COORD_HPP

#include <string>

// Because of the way I hacked up the GPSThing code
// Needed for GPS_FIX_QUALITY
#include "nmeaParser.h"

enum COORD_TYPE
{
  COORD_LATLON = 0,
  COORD_UTM    = 1
};

class TypedCoord
{
public:
  TypedCoord () {m_lat = 0; m_lon = 0;}
  virtual ~TypedCoord () {}
  TypedCoord& operator= (const TypedCoord& other);
  void calculateDistAndBearing (const TypedCoord& coord, double& dist,
                                double& dirStartToEnd, double& bearingEndToStart) const;
  virtual const std::string& createCoordString (std::string& coordString) const
    = 0;
  virtual COORD_TYPE getCoordType () const = 0;
  GPS_FIX_QUALITY getFixQuality () const
  {return m_fixQuality;}
  virtual void getLatLonCoord (double& lat, double& lon) const;
  virtual void getXYCoord (double& x, double& y) const = 0;
  void setFixQuality (GPS_FIX_QUALITY fixQuality);
  virtual void setLatLonCoord (double lat, double lon)
  {m_lat = lat; m_lon = lon;}

protected:
  void distAndBearing (double a, double f, double startLat,
                       double startLon, double endLat, double endLon,
		       double& dist,
                       double& bearingStartToEnd,
		       double& bearingEndToStart) const;
  void distAndBearingWGS84 (double startLat, double startLon,
                            double endLat, double endLon, double& dist,
                            double& bearingStartToEnd,
			    double& bearingEndToStart) const;

  // Current lat/lon coordinate.
  double m_lat;
  double m_lon;

  // Fix quality for the coordinate that originated from a GPS.
  GPS_FIX_QUALITY m_fixQuality;
};

#endif // __TYPED_COORD_HPP
