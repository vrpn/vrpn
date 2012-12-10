// TypedCoord.C: Implementation of the CTypedCoord abstract base class.
//
// Written by Jason Bevins in 1998.  File is in the public domain.
//

#include "gpsnmealib/nmeaParser.h"      // for GPS_FIX_QUALITY
#include "typedCoord.h"
#ifdef sgi
#include <math.h>
#else
#include <cmath>                        // for sqrt, atan2, cos, sin, fabs
#endif

// Mmmmm, pi...
static const double PI = 3.141592653589793;

// Radians to degrees conversion constants.
static const double RAD_TO_DEG = 180.0 / PI;
static const double DEG_TO_RAD = PI / 180.0;

static double mod (double num, double divisor)
// Purpose:
//  This function determines the remainder of num / divisor.
{
  if (num >= 0) {
    return num - divisor * (int)(num / divisor);
  } else {
    return num + divisor * (int)((-num / divisor) + 1);
  }
}

/////////////////////////////////////////////////////////////////////////////
// TypedCoord operators

TypedCoord& TypedCoord::operator= (const TypedCoord& other)
{
  double lat, lon;
  other.getLatLonCoord (lat, lon);
  setLatLonCoord (lat, lon);
  return *this;
}


/////////////////////////////////////////////////////////////////////////////
// TypedCoord members

void TypedCoord::calculateDistAndBearing (const TypedCoord& coord,
                                          double& dist, double& bearingStartToEnd, double& bearingEndToStart) const
  // Purpose:
  //  This function calculates the distance between this coordinate and the
  //  specified coordinate on the Earth and determines the bearing to the
  //  coordinates in the forward and the reverse direction.
  // Parameters:
  //  TypedCoord& coord:
  //      The specified coordinate.
  //  double& dist:
  //      Upon exit, this parameter contains the distance between the two
  //      coordinates, in meters.
  //  double& bearingStartToEnd:
  //      Upon exit, this parameter contains the bearing from this coordinate
  //      to the specified coordinate, in degrees.
  //  double& bearingEndToStart:
  //      Upon exit, this parameter contains the bearing from the specified
  //      coordinate to this coordinate, in degrees.
{
  double startLat, startLon;
  double endLat, endLon;
  getLatLonCoord (startLat, startLon);
  coord.getLatLonCoord (endLat, endLon);
  distAndBearingWGS84 (startLat, startLon, endLat, endLon, dist,
                       bearingStartToEnd, bearingEndToStart);
}


void TypedCoord::distAndBearing (double a, double f, double startLat,
                                 double startLon, double endLat, double endLon, double& dist,
                                 double& bearingStartToEnd, double& bearingEndToStart) const
  // Purpose:
  //  This function calculates the distance between two coordinates on the Earth
  //  and determines the bearing to the coordinates in the forward and the
  //  reverse direction.
  // Pre:
  //  startLat and endLat does not equal 90 or -90 degrees.
  // Parameters:
  //  double a:
  //      Ellipsoid semi-major axis, in meters. (For WGS84 datum, use 6378137.0)
  //  double f:
  //      Ellipsoid flattening. (For WGS84 datum, use 1 / 298.257223563)
  //  double startLat:
  //      The starting latitude.
  //  double startLon:
  //      The starting longitude.
  //  double endLat:
  //      The ending latitude.
  //  double startLon:
  //      The ending longitude.
  //  double& dist:
  //      Upon exit, this parameter contains the distance between the two
  //      coordinates, in meters.
  //  double& bearingStartToEnd:
  //      Upon exit, this parameter contains the bearing from the starting
  //      coordinate to the ending coordinate, in degrees.
  //  double& bearingEndToStart:
  //      Upon exit, this parameter contains the bearing from the ending
  //      coordinate to the starting coordinate, in degrees.
{
  double c;
  double c1;
  double c2;
  double c2a;
  double sinX, cosX;
  double cy, cz;
  double d, e;
  double r, s;
  double s1, sa, sy;
  double tan1, tan2;
  double x, y;
  double t, t1, t2, t3, t4, t5;
  double coordStartLatRad = startLat * DEG_TO_RAD;
  double coordStartLonRad = startLon * DEG_TO_RAD;
  double coordEndLatRad = endLat * DEG_TO_RAD;
  double coordEndLonRad = endLon * DEG_TO_RAD;
  r = 1.0 - f;
  tan1 = (r * sin (coordStartLatRad)) / cos (coordStartLatRad);
  tan2 = (r * sin (coordEndLatRad)) / cos (coordEndLatRad);
  c1 = 1.0 / sqrt ((tan1 * tan1) + 1.0);
  s1 = c1 * tan1;
  c2 = 1.0 / sqrt ((tan2 * tan2) + 1.0);
  s = c1 * c2;
  bearingEndToStart = s * tan2;
  bearingStartToEnd = bearingEndToStart * tan1;
  x = coordEndLonRad - coordStartLonRad;
  int exitLoop = 0;
  int loopCount = 0;
  while (exitLoop == 0 && loopCount < 6)
    {
      sinX = sin (x);
      cosX = cos (x);
      tan1 = c2 * sinX;
      tan2 = bearingEndToStart - (s1 * c2 * cosX);
      sy = sqrt ((tan1 * tan1) + (tan2 * tan2));
      cy = (s * cosX) + bearingStartToEnd;
      y = atan2 (sy, cy); 
      sa = (s * sinX) / sy;
      c2a = ((-sa) * sa) + 1.0;
      cz = bearingStartToEnd * 2.0;
      if (c2a > 0.0) {
        cz = ((-cz) / c2a) + cy;
      }
      e = (cz * cz * 2.0) - 1.0;
      c = (((((-3.0 * c2a) + 4.0 ) * f) + 4.0) * c2a * f) / 16.0;
      d = x;
      t = ((((e * cy * c) + cz) * sy * c) + y) * sa;
      x = ((1.0 - c) * t * f) + coordEndLonRad - coordStartLonRad;
      // Make sure this function does not go into an infinite loop...
      if (fabs (d - x) > 0.00000000005) {
        exitLoop = 0;
      } else {
        exitLoop = 1;
      }
      ++loopCount;
    }
  bearingStartToEnd = atan2 (tan1, tan2);
  bearingEndToStart = atan2 (c1 * sinX, ((bearingEndToStart * cosX)
                                         - (s1 * c2))) + PI;
  t = sqrt ((((1.0 / r / r) - 1) * c2a) + 1.0) + 1.0;
  x = (t - 2.0) / t;
  t = 1.0 - x;
  c = (((x * x) / 4.0) + 1.0) / t;
  d = ((0.375 * (x * x)) - 1.0) * x;
  x *= cy;
  s = (1.0 - e) - e;
  t1 = (sy * sy * 4.0) - 3.0;
  t2 = ((s * cz * d) / 6.0) - x;
  t3 = t1 * t2;
  t4 = ((t3 * d) / 4.0) + cz;
  t5 = (t4 * sy * d ) + y;
  dist = t5 * c * a * r;
  bearingStartToEnd *= RAD_TO_DEG;
  bearingEndToStart *= RAD_TO_DEG;
  // Make sure the bearings are between 0 and 360, inclusive.
  bearingStartToEnd = mod (bearingStartToEnd, 360.0);
  bearingEndToStart = mod (bearingEndToStart, 360.0);
}


void TypedCoord::distAndBearingWGS84 (double startLat, double startLon,
                                      double endLat, double endLon, double& dist, double& bearingStartToEnd,
                                      double& bearingEndToStart) const
  // Purpose:
  //  This function calculates the distance between two coordinates on the Earth
  //  and determines the bearing to the coordinates in the forward and the
  //  reverse direction.
  // Pre:
  //  startLat and endLat does not equal 90 or -90 degrees.
  // Parameters:
  //  See TypedCoord::DistAndBearing().
{
  distAndBearing (6378137.0, 1 / 298.257223563, startLat, startLon, endLat,
                  endLon, dist, bearingStartToEnd, bearingEndToStart);
}


void TypedCoord::getLatLonCoord (double& lat, double& lon) const
  // Purpose:
  //  This function returns the latitude and longitude contained within this
  //  object.
{
  lat = m_lat;
  lon = m_lon;
}


void TypedCoord::setFixQuality (GPS_FIX_QUALITY fixQuality)
  // Purpose:
  //  This function sets the fix quality of the coordinate.  Use this function
  //  to store the quality of the fix from a GPS with the coordinate.
{
  m_fixQuality = fixQuality;
}
