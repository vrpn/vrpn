#include  <stdio.h>
#include  "vrpn_TempImager.h"

vrpn_TempImager::vrpn_TempImager(const char *name, vrpn_Connection *c) :
  vrpn_BaseClass(name, c),
  _nRows(0), _nCols(0),
  _minX(0), _maxX(0),
  _minY(0), _maxY(0),
  _nChannels(0)
{
    vrpn_BaseClass::init();
}

int vrpn_TempImager::register_types(void)
{
  _description_m_id = d_connection->register_message_type("vrpn_TempImager Description");
  _region_m_id = d_connection->register_message_type("vrpn_TempImager Region");
  if ((_description_m_id == -1) || (_region_m_id == -1) ) {
    return -1;
  } else {
    return 0;
  }
}

vrpn_TempImager_Server::vrpn_TempImager_Server(const char *name, vrpn_Connection *c,
			 vrpn_int32 nCols, vrpn_int32 nRows,
			 vrpn_float32 minX, vrpn_float32 maxX,
			 vrpn_float32 minY, vrpn_float32 maxY) :
    vrpn_TempImager(name, c),
    _description_sent(false)
{
    _nRows = nRows;
    _nCols = nCols;
    _minX = minX; _maxX = maxX;
    _minY = minY; _maxY = maxY;

    // Set up callback handler for ping message from client so that it
    // sends the description.  This will make sure that the other side has
    // heard the descrption before it hears a region message.  Also set this up
    // to fire on the "new connection" system message.

    register_autodeleted_handler(d_ping_message_id, handle_ping_message, this, d_sender_id);
    register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), handle_ping_message, this);
}

int vrpn_TempImager_Server::add_channel(const char *name, const char *units,
		    vrpn_float32 minVal, vrpn_float32 maxVal,
		    vrpn_float32 scale, vrpn_float32 offset)
{
  if (_nChannels >= vrpn_IMAGER_MAX_CHANNELS) {
    return -1;
  }
  strncpy(_channels[_nChannels].name, name, sizeof(cName));
  strncpy(_channels[_nChannels].units, units, sizeof(cName));
  _channels[_nChannels].minVal = minVal;
  _channels[_nChannels].maxVal = maxVal;
  if (scale == 0) {
    fprintf(stderr,"vrpn_TempImager_Server::add_channel(): Scale was zero, set to 1\n");
    scale = 1;
  }
  _channels[_nChannels].scale = scale;
  _channels[_nChannels].offset = offset;
  _nChannels++;

  // We haven't set a proper description now
  _description_sent = false;
  return _nChannels-1;
}

bool  vrpn_TempImager_Server::fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_uint16 *data)
{
  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) > vrpn_IMAGER_MAX_REGION) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Region to large\n");
    return false;
  }

  // Set the channel index
  _region.chanIndex = chanIndex;

  // Set the region size.
  _region.rMin = rMin;
  _region.rMax = rMax;
  _region.cMin = cMin;
  _region.cMax = cMax;

  // Fill the region with the values from the data, unscaling and unoffsetting beforehand so
  // that it fits within the range.
  unsigned  r,c;
  vrpn_float32	offset = _channels[_region.chanIndex].offset;
  vrpn_float32	scale = _channels[_region.chanIndex].scale;
  for (r = rMin; r <= rMax; r++) {
    for (c = cMin; c <= cMax; c++) {
      _region.write_unscaled_pixel(c, r, (vrpn_uint16)( (data[c+r*_nCols]-offset)/scale) );
    }
  }

  return true;
}

bool  vrpn_TempImager_Server::fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_float32 *data)
{
  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) > vrpn_IMAGER_MAX_REGION) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Region to large\n");
    return false;
  }

  // Set the channel index
  _region.chanIndex = chanIndex;

  // Set the region size.
  _region.rMin = rMin;
  _region.rMax = rMax;
  _region.cMin = cMin;
  _region.cMax = cMax;

  // Fill the region with the values from the data, unscaling and unoffsetting beforehand so
  // that it fits within the range.
  unsigned  r,c;
  vrpn_float32	offset = _channels[_region.chanIndex].offset;
  vrpn_float32	scale = _channels[_region.chanIndex].scale;
  for (r = rMin; r <= rMax; r++) {
    for (c = cMin; c <= cMax; c++) {
      _region.write_unscaled_pixel(c, r, (vrpn_uint16)( (data[c+r*_nCols]-offset)/scale) );
    }
  }

  return true;
}

bool  vrpn_TempImager_Server::fill_region(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, vrpn_uint8 *data)
{
  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) > vrpn_IMAGER_MAX_REGION) {
    fprintf(stderr,"vrpn_TempImager_Server::fill_region(): Region to large\n");
    return false;
  }

  // Set the channel index
  _region.chanIndex = chanIndex;

  // Set the region size.
  _region.rMin = rMin;
  _region.rMax = rMax;
  _region.cMin = cMin;
  _region.cMax = cMax;

  // Fill the region with the values from the data, unscaling and unoffsetting beforehand so
  // that it fits within the range.
  unsigned  r,c;
  vrpn_float32	offset = _channels[_region.chanIndex].offset;
  vrpn_float32	scale = _channels[_region.chanIndex].scale;
  for (r = rMin; r <= rMax; r++) {
    for (c = cMin; c <= cMax; c++) {
      _region.write_unscaled_pixel(c, r, (vrpn_uint16)( (data[c+r*_nCols]-offset)/scale) );
    }
  }

  return true;
}

bool  vrpn_TempImager_Server::send_region(const struct timeval *time)
{
  // If we haven't sent the description yet, send it now.
  if (!_description_sent) {
    if (!send_description()) {
      return false;
    }
  }

  // msgbuf must be float64-aligned!
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;

  if (time != NULL) {
    timestamp = *time;
  } else {
    gettimeofday(&timestamp, NULL);
  }
  if (!_region.buffer(&msgbuf, &buflen)) {
    return false;
  }
  vrpn_int32  len = sizeof(fbuf) - buflen;
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _region_m_id, d_sender_id, (char*)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region(): cannot write message: tossing\n");
    return false;
  }

  return true;
}

bool  vrpn_TempImager_Server::send_description(void)
{
  // msgbuf must be float64-aligned!
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;
  int	  i;

  // Pack the description of all of the fields in the imager into the buffer,
  // including the channel descriptions.
  if (vrpn_buffer(&msgbuf, &buflen, _minX) ||
      vrpn_buffer(&msgbuf, &buflen, _maxX) ||
      vrpn_buffer(&msgbuf, &buflen, _minY) ||
      vrpn_buffer(&msgbuf, &buflen, _maxY) ||
      vrpn_buffer(&msgbuf, &buflen, _nRows) ||
      vrpn_buffer(&msgbuf, &buflen, _nCols) ||
      vrpn_buffer(&msgbuf, &buflen, _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_description(): Can't pack message header, tossing\n");
    return false;
  }
  for (i = 0; i < _nChannels; i++) {
    if (!_channels[i].buffer(&msgbuf, &buflen)) {
      fprintf(stderr,"vrpn_TempImager_Server::send_description(): Can't pack message channel, tossing\n");
      return false;
    }
  }

  // Pack the buffer into the connection's outgoing reliable queue, if we have
  // a valid connection.
  vrpn_int32  len = sizeof(fbuf) - buflen;
  gettimeofday(&timestamp, NULL);
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _description_m_id, d_sender_id, (char *)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_description(): cannot write message: tossing\n");
    return false;
  }

  _description_sent = true;
  return true;
}

void  vrpn_TempImager_Server::mainloop(void)
{
  server_mainloop();
}

int  vrpn_TempImager_Server::handle_ping_message(void *userdata, vrpn_HANDLERPARAM p)
{
  vrpn_TempImager_Server  *me = (vrpn_TempImager_Server*)userdata;
  me->send_description();
  return 0;
}

vrpn_TempImager_Remote::vrpn_TempImager_Remote(const char *name, vrpn_Connection *c) :
  vrpn_TempImager(name, c),
  _region_list(NULL),
  _description_list(NULL)
{
  // Register the handlers for the description message and the region change message
  register_autodeleted_handler(_region_m_id, handle_region_message, this, d_sender_id);
  register_autodeleted_handler(_description_m_id, handle_description_message, this, d_sender_id);
}

void  vrpn_TempImager_Remote::mainloop(void)
{
  client_mainloop();
  if (d_connection) { d_connection->mainloop(); };
}

const vrpn_TempImager_Channel *vrpn_TempImager_Remote::channel(unsigned chanNum) const
{
  if (chanNum >= (unsigned)_nChannels) {
    return NULL;
  }
  return &_channels[chanNum];
}

int vrpn_TempImager_Remote::register_description_handler(void *userdata,
			vrpn_IMAGERDESCRIPTIONHANDLER handler)
{
  vrpn_DESCRIPTIONLIST	*new_entry;

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_description_handler: NULL handler\n");
    return -1;
  }

  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_DESCRIPTIONLIST) == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_description_handler: Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = _description_list;
  _description_list = new_entry;

  return 0;
}

int vrpn_TempImager_Remote::unregister_description_handler(void *userdata,
			vrpn_IMAGERDESCRIPTIONHANDLER handler)
{
  // The pointer at *snitch points to victim
  vrpn_DESCRIPTIONLIST	*victim, **snitch;

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  snitch = &_description_list;
  victim = *snitch;
  while ( (victim != NULL) &&
	  ( (victim->handler != handler) ||
	    (victim->userdata != userdata) )) {
    snitch = &( (*snitch)->next );
    victim = victim->next;
  }

  // Make sure we found one
  if (victim == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::unregister_description_handler: No such handler\n");
    return -1;
  }

  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;

  return 0;
}

int vrpn_TempImager_Remote::register_region_handler(void *userdata,
			vrpn_IMAGERREGIONHANDLER handler)
{
  vrpn_REGIONLIST	*new_entry;

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_region_handler: NULL handler\n");
    return -1;
  }

  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_REGIONLIST) == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_region_handler: Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = _region_list;
  _region_list = new_entry;

  return 0;
}

int vrpn_TempImager_Remote::unregister_region_handler(void *userdata,
			vrpn_IMAGERREGIONHANDLER handler)
{
  // The pointer at *snitch points to victim
  vrpn_REGIONLIST	*victim, **snitch;

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  snitch = &_region_list;
  victim = *snitch;
  while ( (victim != NULL) &&
	  ( (victim->handler != handler) ||
	    (victim->userdata != userdata) )) {
    snitch = &( (*snitch)->next );
    victim = victim->next;
  }

  // Make sure we found one
  if (victim == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::unregister_region_handler: No such handler\n");
    return -1;
  }

  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;

  return 0;
}

int vrpn_TempImager_Remote::handle_description_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
  const char *bufptr = p.buffer;
  vrpn_TempImager_Remote *me = (vrpn_TempImager_Remote *)userdata;
  vrpn_DESCRIPTIONLIST *handler = me->_description_list;
  int i;

  // Get my new information from the buffer
  if (vrpn_unbuffer(&bufptr, &me->_minX) ||
      vrpn_unbuffer(&bufptr, &me->_maxX) ||
      vrpn_unbuffer(&bufptr, &me->_minY) ||
      vrpn_unbuffer(&bufptr, &me->_maxY) ||
      vrpn_unbuffer(&bufptr, &me->_nRows) ||
      vrpn_unbuffer(&bufptr, &me->_nCols) ||
      vrpn_unbuffer(&bufptr, &me->_nChannels) ) {
    return -1;
  }
  for (i = 0; i < me->_nChannels; i++) {
    if (!me->_channels[i].unbuffer(&bufptr)) {
      return -1;
    }
  }

  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  while (handler != NULL) {
	  handler->handler(handler->userdata, p.msg_time);
	  handler = handler->next;
  }

  return 0;
}

int vrpn_TempImager_Remote::handle_region_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
  const char *bufptr = p.buffer;
  vrpn_TempImager_Remote *me = (vrpn_TempImager_Remote *)userdata;
  vrpn_REGIONLIST *handler = me->_region_list;
  vrpn_IMAGERREGIONCB rp;

  // Get the region information from the buffer
  if (!me->_region.unbuffer(&bufptr)) {
    return -1;
  }

  // Fill in a user callback structure with the data
  rp.msg_time = p.msg_time;
  rp.region = &me->_region;

  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  while (handler != NULL) {
	  handler->handler(handler->userdata, rp);
	  handler = handler->next;
  }

  return 0;
}

