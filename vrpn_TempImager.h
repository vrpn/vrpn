#ifndef	VRPN_TEMPIMAGER_H
#define	VRPN_TEMPIMAGER_H
#include  "vrpn_Connection.h"
#include  "vrpn_BaseClass.h"

const unsigned vrpn_IMAGER_MAX_CHANNELS = 10;
const unsigned vrpn_IMAGER_MAX_REGION = vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_int16) - 4;

/// Holds the description needed to convert from raw data to values for a channel
class vrpn_TempImager_Channel {
public:
  vrpn_TempImager_Channel(void) { name[0] = '\0'; units[0] = '\0'; minVal = maxVal = 0.0;
      scale = offset = 0; };

  inline  bool	buffer(char **insertPt, vrpn_int32 *buflen) {
    if (vrpn_buffer(insertPt, buflen, minVal) ||
        vrpn_buffer(insertPt, buflen, maxVal) ||
        vrpn_buffer(insertPt, buflen, offset) ||
        vrpn_buffer(insertPt, buflen, scale) ||
	vrpn_buffer(insertPt, buflen, name, sizeof(name)) ||
	vrpn_buffer(insertPt, buflen, units, sizeof(units)) ) {
      return false;
    } else {
      return true;
    }
  }

  inline  bool	unbuffer(const char **buffer) {
    if (vrpn_unbuffer(buffer, &minVal) ||
        vrpn_unbuffer(buffer, &maxVal) ||
        vrpn_unbuffer(buffer, &offset) ||
        vrpn_unbuffer(buffer, &scale) ||
	vrpn_unbuffer(buffer, name, sizeof(name)) ||
	vrpn_unbuffer(buffer, units, sizeof(units)) ) {
      return false;
    } else {
      return true;
    }
  }

  cName	name;			//< Name of the data set stored in this channel
  cName	units;			//< Units for the data set stored in this channel
  vrpn_float32	minVal, maxVal; //< Range of possible values for pixels in this channel
  vrpn_float32	offset, scale;	//< Values in units are (raw_values * scale) + offset
};

/// Holds the data for a sub-region of one channel of the image
class vrpn_TempImager_Region {
public:
  vrpn_TempImager_Region(void) { chanIndex = -1; rMin = rMax = cMin = cMax = 0; };
  
  inline  bool	buffer(char **insertPt, vrpn_int32 *buflen) {
    if (vrpn_buffer(insertPt, buflen, chanIndex) ||
	vrpn_buffer(insertPt, buflen, rMin) ||
        vrpn_buffer(insertPt, buflen, rMax) ||
        vrpn_buffer(insertPt, buflen, cMin) ||
        vrpn_buffer(insertPt, buflen, cMax) ) {
      return false;
    }
    int cols = cMax-cMin+1;
    for (unsigned r = rMin; r <= rMax; r++) {
      for (unsigned c = cMin; c <= cMax; c++) {
	if (vrpn_buffer(insertPt, buflen, vals[(c-cMin) + (r-rMin)*cols])) {
	  return false;
	}
      }
    }
    return true;
  }

  inline  bool	unbuffer(const char **buffer) {
    if (vrpn_unbuffer(buffer, &chanIndex) ||
	vrpn_unbuffer(buffer, &rMin) ||
        vrpn_unbuffer(buffer, &rMax) ||
        vrpn_unbuffer(buffer, &cMin) ||
        vrpn_unbuffer(buffer, &cMax) ) {
      return false;
    }
    int cols = cMax-cMin+1;
    for (unsigned r = rMin; r <= rMax; r++) {
      for (unsigned c = cMin; c <= cMax; c++) {
	if (vrpn_unbuffer(buffer, &vals[(c-cMin) + (r-rMin)*cols])) {
	  return false;
	}
      }
    }
    return true;
  }
  
  /// Returns the number of values in the <code>vals</code> array.
  inline vrpn_uint32 getNumVals( ) const
  {
	  return ( rMax - rMin + 1 ) * ( cMax - cMin + 1 );
  }

  /// Reads pixel from the region with no scale and offset applied to the value
  inline  bool	read_unscaled_pixel(vrpn_uint16 c, vrpn_uint16 r, vrpn_uint16 &val) const {
    if ( (c < cMin) || (c > cMax) || (r < rMin) || (r > rMax) ) {
      return false;
    } else {
      val = vals[(c-cMin) + (r-rMin)*(cMax-cMin+1)];
    }
    return true;
  }

  /// Writes pixel into the region; caller is responsible for doing scale and offset of the value
  inline  bool	write_unscaled_pixel(vrpn_uint16 c, vrpn_uint16 r, vrpn_uint16 val) {
    if ( (c < cMin) || (c > cMax) || (r < rMin) || (r > rMax) ) {
      return false;
    } else {
      vals[(c-cMin) + (r-rMin)*(cMax-cMin+1)] = val;
    }
    return true;
  }

  vrpn_int16  chanIndex;	//< Which channel this region holds data for 
  vrpn_uint16 rMin, rMax;	//< Range of indices for the rows
  vrpn_uint16 cMin, cMax;	//< Range of indices for the columns
  vrpn_uint16 vals[vrpn_IMAGER_MAX_REGION];	//< Values, stored with column index varying fastest
};

/// Base class for temporary implementation of Imager class
class vrpn_TempImager: public vrpn_BaseClass {
public:
  vrpn_TempImager(const char *name, vrpn_Connection *c = NULL);

protected:
  vrpn_int32	_nRows;		//< Number of rows in the image
  vrpn_int32	_nCols;		//< Number of columns in the image
  vrpn_float32	_minX,_maxX;	//< Real-space range of X pixel centers in meters
  vrpn_float32	_minY,_maxY;	//< Real-space range of Y pixel centers in meters
  vrpn_int32	_nChannels;	//< Number of image data channels
  vrpn_TempImager_Channel _channels[vrpn_IMAGER_MAX_CHANNELS];
  vrpn_TempImager_Region  _region;  //< Stores one region of data for sending or receiving

  virtual int register_types(void);
  vrpn_int32	_description_m_id;  //< ID of the message type describing the range and channels
  vrpn_int32	_region_m_id;	    //< ID of the message type describing a region
};

class vrpn_TempImager_Server: public vrpn_TempImager {
public:
  vrpn_TempImager_Server(const char *name, vrpn_Connection *c,
			 vrpn_int32 nCols, vrpn_int32 nRows,
			 vrpn_float32 minX = 0, vrpn_float32 maxX = 1,
			 vrpn_float32 minY = 0, vrpn_float32 MaxY = 1);

  /// Add a channel to the server, returns index of the channel or -1 on failure.
  int	add_channel(const char *name, const char *units = "unsigned8bit",
		    vrpn_float32 minVal = 0, vrpn_float32 maxVal = 255,
		    vrpn_float32 scale = 1, vrpn_float32 offset = 0);

  bool	fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_uint8 *data);
  bool	fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_uint16 *data);
  bool	fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_float32 *data);

  bool	send_region(const struct timeval *time = NULL);

  /// Handle baseclass ping/pong messages
  virtual void	mainloop(void);

protected:
  bool	_description_sent;    //< Has the description message been sent?
  /// Sends a description of the imager so the remote can process the region messages
  bool	send_description(void);

  // This method makes sure we send a description whenever we get a ping from
  // a client object.
  static  int handle_ping_message(void *userdata, vrpn_HANDLERPARAM p);
};

//------------------------------------------------------------------------------
// Users deal with things below this line.

typedef struct {
  struct timeval		msg_time; //< Timestamp of the region data's change
  const vrpn_TempImager_Region	*region;  //< New region of the image
} vrpn_IMAGERREGIONCB;

typedef void (*vrpn_IMAGERREGIONHANDLER) (void * userdata,
					  const vrpn_IMAGERREGIONCB info);
typedef void (*vrpn_IMAGERDESCRIPTIONHANDLER) (void * userdata,
					       const struct timeval msg_time);

/// This is the class users deal with: it tells the format and the region data when it arrives.
class vrpn_TempImager_Remote: public vrpn_TempImager {
public:
  vrpn_TempImager_Remote(const char *name, vrpn_Connection *c = NULL);

  /// Register a handler for when new data arrives (can look up info in object when this happens
  virtual int register_region_handler(void *userdata, vrpn_IMAGERREGIONHANDLER handler);
  virtual int unregister_region_handler(void *userdata, vrpn_IMAGERREGIONHANDLER handler);

  /// Register a handler for when the object's description changes (if desired)
  virtual int register_description_handler(void *userdata, vrpn_IMAGERDESCRIPTIONHANDLER handler);
  virtual int unregister_description_handler(void *userdata, vrpn_IMAGERDESCRIPTIONHANDLER handler);

  /// Call this each time through the program's main loop
  virtual void	mainloop(void);

  /// Accessors for the member variables: can be queried in the handler for object changes
  vrpn_int32	nRows(void) const { return _nRows; };
  vrpn_int32	nCols(void) const { return _nCols; };
  vrpn_float32	minX(void) const { return _minX; };
  vrpn_float32	maxX(void) const { return _maxX; };
  vrpn_float32	minY(void) const { return _minY; };
  vrpn_float32	maxY(void) const { return _maxY; };
  vrpn_int32	nChannels(void) const { return _nChannels; };
  const vrpn_TempImager_Channel *channel(unsigned chanNum) const;

protected:
  // Lists to keep track of registered user handlers.
  typedef struct vrpn_TIDCS {
      void			    *userdata;
      vrpn_IMAGERDESCRIPTIONHANDLER handler;
      struct vrpn_TIDCS		    *next;
  } vrpn_DESCRIPTIONLIST;
  vrpn_DESCRIPTIONLIST *_description_list;

  typedef struct vrpn_TIRCS {
      void			    *userdata;
      vrpn_IMAGERREGIONHANDLER	    handler;
      struct vrpn_TIRCS		    *next;
  } vrpn_REGIONLIST;
  vrpn_REGIONLIST *_region_list;

  /// Handler for description change message from the server.
  static int handle_region_message(void *userdata, vrpn_HANDLERPARAM p);

  /// Handler for region change message from the server.
  static int handle_description_message(void *userdata, vrpn_HANDLERPARAM p);
};

#endif
