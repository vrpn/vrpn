// UtmCoord.C: Implementaion of the CUtmCoord class.
//
// Written by Jason Bevins in 1998.  File is in the public domain.
//

#include "gpsnmealib/typedCoord.h"      // for TypedCoord
#include "utmCoord.h"
#ifdef sgi
#include <assert.h>
#include <math.h>
#else
#include <cassert>                      // for assert
#include <cmath>                        // for sin, cos, sqrt, M_PI
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Radians to degrees conversion constants.
static const double RAD_TO_DEG = 180.0 / M_PI;
static const double DEG_TO_RAD = M_PI / 180.0;

// Some constants used by these functions.
static const double fe = 500000.0;
static const double ok = 0.9996;

// An array containing each vertical UTM zone.
static char cArray[] = "CDEFGHJKLMNPQRSTUVWX";


/////////////////////////////////////////////////////////////////////////////
// Miscellaneous functions for these UTM conversion formulas.

static double calculateESquared (double a, double b)
{
  return ((a * a) - (b * b)) / (a * a);
}


static double calculateE2Squared (double a, double b)
{
  return ((a * a) - (b * b)) / (b * b);
}


static double denom (double es, double sphi)
{
  double sinSphi = sin (sphi);
  return sqrt (1.0 - es * (sinSphi * sinSphi));
}

static double sphsr (double a, double es, double sphi)
{
  double dn = denom (es, sphi);
  return a * (1.0 - es) / (dn * dn * dn);
}


static double sphsn (double a, double es, double sphi)
{
  double sinSphi = sin (sphi);
  return a / sqrt (1.0 - es * (sinSphi * sinSphi));
}


static double sphtmd (double ap, double bp, double cp, double dp, double ep,
               double sphi)
{
  return (ap * sphi) - (bp * sin (2.0 * sphi)) + (cp * sin (4.0 * sphi))
    - (dp * sin (6.0 * sphi)) + (ep * sin (8.0 * sphi));
}

/////////////////////////////////////////////////////////////////////////////
// UTMCoord construction/destruction

UTMCoord::UTMCoord ()
{
  m_requireLatLonConvert = false;
  m_requireUTMConvert = false;
  setLatLonCoord (0.0, 0.0);

  m_utmXZone = 0;
  m_utmYZone = 0;
  m_easting = 0;
  m_northing = 0;
}


UTMCoord::UTMCoord (int utmXZone, char utmYZone, double easting,
                    double northing)
{
  m_requireLatLonConvert = false;
  m_requireUTMConvert = false;
  setUTMCoord (utmXZone, utmYZone, easting, northing);
}


UTMCoord::UTMCoord (const UTMCoord& other)
{
  copyUTMCoord (other);
}


UTMCoord::UTMCoord (const TypedCoord& other)
{
  copyOtherCoord (other);
}


/////////////////////////////////////////////////////////////////////////////
// UTMCoord operators

UTMCoord& UTMCoord::operator= (const UTMCoord& other)
{
  copyUTMCoord (other);
  return *this;
}


UTMCoord& UTMCoord::operator= (const TypedCoord& other)
{
  copyOtherCoord (other);
  return *this;
}


/////////////////////////////////////////////////////////////////////////////
// UTMCoord members

void UTMCoord::copyUTMCoord (const UTMCoord& other)
  // Purpose:
  //  This function copies the UTM coordinate of the specified object and
  //  assigns that coordinate to this object.
  // Notes:
  //  This function copies every member function from that object, including
  //  its lat/lon value and its lazy evaluation flags.
{
  m_lat = other.m_lat;
  m_lon = other.m_lon;
  m_utmXZone = other.m_utmXZone;
  m_utmYZone = other.m_utmYZone;
  m_easting = other.m_easting;
  m_northing = other.m_northing;
  m_requireLatLonConvert = other.m_requireLatLonConvert;
  m_requireUTMConvert = other.m_requireUTMConvert;
}


void UTMCoord::copyOtherCoord (const TypedCoord& other)
  // Purpose:
  //  This function copies the coordinate of the specified object, converts
  //  the coordinate to the coordinate type of this object, then assigns the
  //  converted coordinate to this object.
{
  // Convert the right hand side coordinate to lat/lon; then convert this
  // lat/lon coordinate to UTM.
  double lat, lon;
  other.getLatLonCoord (lat, lon);
  setLatLonCoord (lat, lon);
}


const std::string& UTMCoord::createCoordString (std::string& coordString) const
  // Purpose:
  //  This function creates a formatted coordinate string containing this
  //  object's coordinate.
  // Parameters:
  //  CString& coordString:
  //      Upon exit, this parameter will contain the coordinate string.
  // Returns:
  //  The coordinate string.
  // Notes:
  //  If the coordinate is outside of the UTM grid boundary (>= 84 N or <= 80 S)
  //  the string will contain the resource string specified by the constant
  //  IDS_OUTSIDE_UTM_BOUNDARY.
{
  double easting;
  double northing;
  int utmXZone;
  char utmYZone;
  getUTMCoord (utmXZone, utmYZone, easting, northing);
#if 0
  if (utmYZone != '*') {
    coordString.Format ("%02d%c %06d %07d", utmXZone, utmYZone,
			(int)easting, (int)northing);
  } else {
    // UTM vertical zone is out of range.
    coordString.LoadString (IDS_OUTSIDE_UTM_BOUNDARY);
  }
#endif
  return coordString;
}


void UTMCoord::getLatLonCoord (double& lat, double& lon) const
  // Purpose:
  //  This function converts the object's UTM coordinate to a lat/lon coordinate
  //  and returns the latitude and longitude.
{
  if (m_requireLatLonConvert == true) {
    // The conversion between UTM and lat/lon has not occurred yet, so do
    // the conversion now.  A non-const pointer to this object must be
    // created so that this function can convert the UTM coordinate stored
    // within this object to a lat/lon coordinate if the conversion is
    // necessary.  This lat/lon coordinate is then copied to the m_lat
    // and m_lon members of this object, requiring the object to be
    // non-const.
    UTMCoord* myThis = const_cast<UTMCoord*>(this);
    // Convert to lat/lon.
    utmToLatLonWGS84 (myThis->m_utmXZone, myThis->m_utmYZone,
                      myThis->m_easting, myThis->m_northing, myThis->m_lat,
                      myThis->m_lon);
    // Do not make this conversion again (unless the coordinates have been
    // changed and another conversion is required.)
    myThis->m_requireLatLonConvert = false;
  }
  lat = m_lat;
  lon = m_lon;
}


void UTMCoord::getUTMCoord (int& utmXZone, char& utmYZone, double& easting,
                            double& northing) const
  // Purpose:
  //  This function returns the current UTM coordinate.
{
  if (m_requireUTMConvert == true) {
    // The conversion between lat/lon and UTM has not occurred yet, so do
    // the conversion now.  A non-const pointer to this object must be
    // created so that this function can convert the lat/lon coordinate
    // stored within this object to a UTM coordinate if the conversion
    // is necessary.  This UTM coordinate is then copied to the m_utmXZone
    // m_utmYZone, m_easting, and m_northing members of this object,
    // requiring the object to be non-const.
    UTMCoord* myThis = const_cast<UTMCoord*>(this);
    // Convert to UTM.
    latLonToUTMWGS84 (myThis->m_utmXZone, myThis->m_utmYZone,
                      myThis->m_easting, myThis->m_northing, myThis->m_lat,
                      myThis->m_lon);
    // Do not make this conversion again (unless the coordinates have been
    // changed and another conversion is required.)
    myThis->m_requireUTMConvert = false;
  }
  easting = m_easting;
  northing = m_northing;
  utmXZone = m_utmXZone;
  utmYZone = m_utmYZone;
}


void UTMCoord::getUTMZone (int& utmXZone, char& utmYZone) const
  // Purpose:
  //  This function returns the current UTM zone.
  // Parameters:
  //  int& utmXZone:
  //      Upon exit, this parameter contains the UTM horizontal zone.  This
  //      zone will be between 1 and 60, inclusive.
  //  char& utmYZone:
  //      Upon exit, this parameter contains the UTM vertical zone.  This
  //      zone will be one of: CDEFGHJKLMNPQRSTUVWX.
{
  double easting;
  double northing;
  getUTMCoord (utmXZone, utmYZone, easting, northing);
}


void UTMCoord::getXYCoord (double& x, double& y) const
  // Purpose:
  //  This function returns the coordinate's (x, y) coordinate.  For this class,
  //  the x coordinate is the easting, and the y coordinate is the northing.
{
  int utmXZone;
  char utmYZone;
  getUTMCoord (utmXZone, utmYZone, x, y);
}


bool UTMCoord::isOutsideUTMGrid () const
  // Purpose:
  //  This function determines whether the current coordinate is outside of the
  //  UTM grid boundary (i.e., >= 84 N or <= 80 s)
{
  // If a UTM coordinate has been converted to a lat/lon coordinate by lazy
  // evaluation, simply determine whether the coordinate is north of 84, or
  // south of 80.
  if (m_requireLatLonConvert == false) {
    if (m_lat >= 84.0 || m_lat <= -80.0) {
      return true;
    } else {
      return false;
    }
  } else if (m_requireUTMConvert == false) {
    if (m_utmYZone == '*') {
      return true;
    } else {
      return false;
    }
  } else {
    // This should not happen
    assert(false);
    return false;
  }
}


void UTMCoord::latLonToUTM (double a, double f, int& utmXZone, char& utmYZone,
                             double& easting, double& northing, double lat, double lon) const
  // Purpose:
  //  This function converts the specified lat/lon coordinate to a UTM
  //  coordinate.
  // Parameters:
  //  double a:
  //      Ellipsoid semi-major axis, in meters. (For WGS84 datum, use 6378137.0)
  //  double f:
  //      Ellipsoid flattening. (For WGS84 datum, use 1 / 298.257223563)
  //  int& utmXZone:
  //      Upon exit, this parameter will contain the hotizontal zone number of
  //      the UTM coordinate.  The returned value for this parameter is a number
  //      within the range 1 to 60, inclusive.
  //  char& utmYZone:
  //      Upon exit, this parameter will contain the zone letter of the UTM
  //      coordinate.  The returned value for this parameter will be one of:
  //      CDEFGHJKLMNPQRSTUVWX.
  //  double& easting:
  //      Upon exit, this parameter will contain the UTM easting, in meters.
  //  double& northing:
  //      Upon exit, this parameter will contain the UTM northing, in meters.
  //  double lat, double lon:
  //      The lat/lon coordinate to convert.
  // Notes:
  //  - The code in this function is a C conversion of some of the source code
  //    from the Mapping Datum Transformation Software (MADTRAN) program,
  //    written in PowerBasic.  To get the source code for MADTRAN, go to:
  //
  //      http://164.214.2.59/publications/guides/MADTRAN/index.html
  //    
  //    and download MADTRAN.ZIP
  //  - If the UTM zone is out of range, the y-zone character is set to the
  //    asterisk character ('*').
{
  double recf;
  double b;
  double eSquared;
  double e2Squared;
  double tn;
  double ap;
  double bp;
  double cp;
  double dp;
  double ep;
  double olam;
  double dlam;
  double s;
  double c;
  double t;
  double eta;
  double sn;
  double tmd;
  double t1, t2, t3, t6, t7;
  double nfn;

  if (lon <= 0.0) {
    utmXZone = 30 + (int)(lon / 6.0);
  } else {
    utmXZone = 31 + (int)(lon / 6.0);
  }
  if (lat < 84.0 && lat >= 72.0) {
    // Special case: zone X is 12 degrees from north to south, not 8.
    utmYZone = cArray[19];
  } else {
    utmYZone = cArray[(int)((lat + 80.0) / 8.0)];
  }
  if (lat >= 84.0 || lat < -80.0) {
    // Invalid coordinate; the vertical zone is set to the invalid
    // character.
    utmYZone = '*';
  }

  double latRad = lat * DEG_TO_RAD;
  double lonRad = lon * DEG_TO_RAD;
  recf = 1.0 / f;
  b = a * (recf - 1.0) / recf;
  eSquared = calculateESquared (a, b);
  e2Squared = calculateE2Squared (a, b);
  tn = (a - b) / (a + b);
  ap = a * (1.0 - tn + 5.0 * ((tn * tn) - (tn * tn * tn)) / 4.0 + 81.0 *
            ((tn * tn * tn * tn) - (tn * tn * tn * tn * tn)) / 64.0);
  bp = 3.0 * a * (tn - (tn * tn) + 7.0 * ((tn * tn * tn)
                                          - (tn * tn * tn * tn)) / 8.0 + 55.0 * (tn * tn * tn * tn * tn) / 64.0)
    / 2.0;
  cp = 15.0 * a * ((tn * tn) - (tn * tn * tn) + 3.0 * ((tn * tn * tn * tn)
                                                       - (tn * tn * tn * tn * tn)) / 4.0) / 16.0;
  dp = 35.0 * a * ((tn * tn * tn) - (tn * tn * tn * tn) + 11.0
                   * (tn * tn * tn * tn * tn) / 16.0) / 48.0;
  ep = 315.0 * a * ((tn * tn * tn * tn) - (tn * tn * tn * tn * tn)) / 512.0;
  olam = (utmXZone * 6 - 183) * DEG_TO_RAD;
  dlam = lonRad - olam;
  s = sin (latRad);
  c = cos (latRad);
  t = s / c;
  eta = e2Squared * (c * c);
  sn = sphsn (a, eSquared, latRad);
  tmd = sphtmd (ap, bp, cp, dp, ep, latRad);
  t1 = tmd * ok;
  t2 = sn * s * c * ok / 2.0;
  t3 = sn * s * (c * c * c) * ok * (5.0 - (t * t) + 9.0 * eta + 4.0
                                    * (eta * eta)) / 24.0;
  if (latRad < 0.0) nfn = 10000000.0; else nfn = 0;
  northing = nfn + t1 + (dlam * dlam) * t2 + (dlam * dlam * dlam
                                              * dlam) * t3 + (dlam * dlam * dlam * dlam * dlam * dlam) + 0.5;
  t6 = sn * c * ok;
  t7 = sn * (c * c * c) * (1.0 - (t * t) + eta) / 6.0;
  easting = fe + dlam * t6 + (dlam * dlam * dlam) * t7 + 0.5;
  if (northing >= 9999999.0) northing = 9999999.0;
}


void UTMCoord::latLonToUTMWGS84 (int& utmXZone, char& utmYZone,
                                  double& easting, double& northing, double lat, double lon) const
  // Purpose:
  //  This function converts the specified lat/lon coordinate to a UTM
  //  coordinate in the WGS84 datum.  (See the comment block for the
  //  LatLonToUTM() member function.)
{
  latLonToUTM (6378137.0, 1 / 298.257223563, utmXZone, utmYZone,
               easting, northing, lat, lon);
}


void UTMCoord::setLatLonCoord (double lat, double lon)
  // Purpose:
  //  This function sets the UTM coordinate given a latitude and a longitude.
{
  m_lat = lat;
  m_lon = lon;

  // No conversion between UTM and lat/lon is necessary, since this function
  // has set a new lat/lon coordinate.
  m_requireLatLonConvert = false;
  // Do not perform the conversion between lat/lon and UTM now; wait until
  // the user requests the UTM coordinate.
  m_requireUTMConvert = true;
}


void UTMCoord::setUTMCoord (int utmXZone, char utmYZone, double easting,
                             double northing)
{
  m_easting = easting;
  m_northing = northing;
  m_utmXZone = utmXZone;
  m_utmYZone = utmYZone;

  // Do not perform the conversion between UTM and lat/lon now; wait until
  // the user requests the lat/lon coordinate.
  m_requireLatLonConvert = true;
  // No conversion between lat/lon and UTM is necessary, since this function
  // has set a new UTM coordinate.
  m_requireUTMConvert = false;
}


void UTMCoord::utmToLatLon (double a, double f, int utmXZone, char utmYZone,
                            double easting, double northing, double& lat, double& lon) const
  // Purpose:
  //  This function converts the specified UTM coordinate to a lat/lon
  //  coordinate.
  // Pre:
  //  - utmXZone must be between 1 and 60, inclusive.
  //  - utmYZone must be one of: CDEFGHJKLMNPQRSTUVWX
  // Parameters:
  //  double a:
  //      Ellipsoid semi-major axis, in meters. (For WGS84 datum, use 6378137.0)
  //  double f:
  //      Ellipsoid flattening. (For WGS84 datum, use 1 / 298.257223563)
  //  int utmXZone:
  //      The horizontal zone number of the UTM coordinate.
  //  char utmYZone:
  //      The vertical zone letter of the UTM coordinate.
  //  double easting, double northing:
  //      The UTM coordinate to convert.
  //  double& lat:
  //      Upon exit, lat contains the latitude.
  //  double& lon:
  //      Upon exit, lon contains the longitude.
  // Notes:
  //  The code in this function is a C conversion of some of the source code
  //  from the Mapping Datum Transformation Software (MADTRAN) program, written
  //  in PowerBasic.  To get the source code for MADTRAN, go to:
  //
  //    http://164.214.2.59/publications/guides/MADTRAN/index.html
  //
  //  and download MADTRAN.ZIP
{
  double recf;
  double b;
  double eSquared;
  double e2Squared;
  double tn;
  double ap;
  double bp;
  double cp;
  double dp;
  double ep;
  double nfn;
  double tmd;
  double sr;
  double sn;
  double ftphi;
  double s;
  double c;
  double t;
  double eta;
  double de;
  double dlam;
  double olam;

  recf = 1.0 / f;
  b = a * (recf - 1) / recf;
  eSquared = calculateESquared (a, b);
  e2Squared = calculateE2Squared (a, b);
  tn = (a - b) / (a + b);
  ap = a * (1.0 - tn + 5.0 * ((tn * tn) - (tn * tn * tn)) / 4.0 + 81.0 *
            ((tn * tn * tn * tn) - (tn * tn * tn * tn * tn)) / 64.0);
  bp = 3.0 * a * (tn - (tn * tn) + 7.0 * ((tn * tn * tn)
                                          - (tn * tn * tn * tn)) / 8.0 + 55.0 * (tn * tn * tn * tn * tn) / 64.0)
    / 2.0;
  cp = 15.0 * a * ((tn * tn) - (tn * tn * tn) + 3.0 * ((tn * tn * tn * tn)
                                                       - (tn * tn * tn * tn * tn)) / 4.0) / 16.0;
  dp = 35.0 * a * ((tn * tn * tn) - (tn * tn * tn * tn) + 11.0
                   * (tn * tn * tn * tn * tn) / 16.0) / 48.0;
  ep = 315.0 * a * ((tn * tn * tn * tn) - (tn * tn * tn * tn * tn)) / 512.0;
  if ((utmYZone <= 'M' && utmYZone >= 'C')
      || (utmYZone <= 'm' && utmYZone >= 'c')) { 
    nfn = 10000000.0;
  } else {
    nfn = 0;
  }
  tmd = (northing - nfn) / ok;
  sr = sphsr (a, eSquared, 0.0);
  ftphi = tmd / sr;
  double t10, t11, t14, t15;
  for (int i = 0; i < 5; i++) {
    t10 = sphtmd (ap, bp, cp, dp, ep, ftphi);
    sr = sphsr (a, eSquared, ftphi);
    ftphi = ftphi + (tmd - t10) / sr;
  }
  sr = sphsr (a, eSquared, ftphi);
  sn = sphsn (a, eSquared, ftphi);
  s = sin (ftphi);
  c = cos (ftphi);
  t = s / c;
  eta = e2Squared * (c * c);
  de = easting - fe;
  t10 = t / (2.0 * sr * sn * (ok * ok));
  t11 = t * (5.0 + 3.0 * (t * t) + eta - 4.0 * (eta * eta) - 9.0 * (t * t)
             * eta) / (24.0 * sr * (sn * sn * sn) * (ok * ok * ok * ok));
  lat = ftphi - (de * de) * t10 + (de * de * de * de) * t11;
  t14 = 1.0 / (sn * c * ok);
  t15 = (1.0 + 2.0 * (t * t) + eta) / (6 * (sn * sn * sn) * c 
                                       * (ok * ok * ok));
  dlam = de * t14 - (de * de * de) * t15;
  olam = (utmXZone * 6 - 183.0) * DEG_TO_RAD;
  lon = olam + dlam;
  lon *= RAD_TO_DEG;
  lat *= RAD_TO_DEG;
}


void UTMCoord::utmToLatLonWGS84 (int utmXZone, char utmYZone, double easting,
                                 double northing, double& lat, double& lon) const
  // Purpose:
  //  This function converts the specified UTM coordinate to a lat/lon
  //  coordinate in the WGS84 datum.  (See the comment block for the
  //  UTMToLatLon() member function.
{
  utmToLatLon (6378137.0, 1 / 298.257223563, utmXZone, utmYZone,
               easting, northing, lat, lon);
}
