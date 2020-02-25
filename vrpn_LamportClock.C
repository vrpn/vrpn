#include "vrpn_LamportClock.h"
#include <string.h>
#include <stdio.h>

vrpn_LamportTimestamp::vrpn_LamportTimestamp (int vectorLength,
                                              vrpn_uint32 * vector) :
    d_timestampSize (vectorLength),
    d_timestamp(NULL)
{
  d_timestamp = new vrpn_uint32[vectorLength];
  copy(vector);
}

vrpn_LamportTimestamp::vrpn_LamportTimestamp
                            (const vrpn_LamportTimestamp & r) :
    d_timestampSize (r.d_timestampSize),
    d_timestamp(NULL)
{
  d_timestamp = new vrpn_uint32[r.d_timestampSize];
  copy(r.d_timestamp);
}



vrpn_LamportTimestamp::~vrpn_LamportTimestamp (void) {
  if (d_timestamp) {
    try {
      delete[] d_timestamp;
    } catch (...) {
      fprintf(stderr, "vrpn_LamportTimestamp::~vrpn_LamportTimestamp(): delete failed\n");
      return;
    }
  }
}

vrpn_LamportTimestamp & vrpn_LamportTimestamp::operator =
        (const vrpn_LamportTimestamp & r) {

  if (d_timestamp) {
    try {
      delete[] d_timestamp;
    } catch (...) {
      fprintf(stderr, "vrpn_LamportTimestamp::operator =(): delete failed\n");
      return *this;
    }
    d_timestamp = NULL;
  }

  d_timestampSize = r.d_timestampSize;
  try { d_timestamp = new vrpn_uint32[r.d_timestampSize]; }
  catch (...) {
    d_timestamp = NULL;
    return *this;
  }

  copy(r.d_timestamp);

  return *this;
}



vrpn_bool vrpn_LamportTimestamp::operator < (const vrpn_LamportTimestamp & r)
                              const {
  int i;

  // TODO
  // What's the right thing to do here?  Throw an exception?
  if (d_timestampSize != r.d_timestampSize) {
    return d_timestampSize < r.d_timestampSize;
  }

  // TODO
  // How do we compare these correctly?

  for (i = 0; i < d_timestampSize; i++) {
    if (d_timestamp[i] > r.d_timestamp[i]) {
      return vrpn_false;
    }
  }

  for (i = 0; i < d_timestampSize; i++) {
    if (d_timestamp[i] < r.d_timestamp[i]) {
      return vrpn_true;
    }
  }

  return vrpn_false;  // equal
}

vrpn_uint32 vrpn_LamportTimestamp::operator [] (int i) const {
  if ((i < 0) || (i >= d_timestampSize)) {
    return 0;
  }
  return d_timestamp[i];
}

int vrpn_LamportTimestamp::size (void) const {
  return d_timestampSize;
}


void vrpn_LamportTimestamp::copy (const vrpn_uint32 * vector) {
  int i;

  if (d_timestamp && vector) {
    for (i = 0; i < d_timestampSize; i++) {
      d_timestamp[i] = vector[i];
    }
  }
}



vrpn_LamportClock::vrpn_LamportClock (int numHosts, int ourIndex) :
    d_numHosts (numHosts),
    d_ourIndex (ourIndex),
    d_currentTimestamp(NULL)
{
  d_currentTimestamp = new vrpn_uint32[numHosts];

  int i;
  if (d_currentTimestamp) {
    for (i = 0; i < numHosts; i++) {
      d_currentTimestamp[i] = 0;
    }
  }
}


vrpn_LamportClock::~vrpn_LamportClock (void) {
  if (d_currentTimestamp) {
    try {
      delete[] d_currentTimestamp;
    } catch (...) {
      fprintf(stderr, "vrpn_LamportClock::~vrpn_LamportClock(): delete failed\n");
      return;
    }
  }
}

void vrpn_LamportClock::receive (const vrpn_LamportTimestamp & r) {
  int i;

  if (r.size() != d_numHosts) {
    // Throw exception!
    return;
  }

  for (i = 0; i < d_numHosts; i++) {
    if (r[i] > d_currentTimestamp[i]) {
      d_currentTimestamp[i] = r[i];
    }
  }

}

vrpn_LamportTimestamp * vrpn_LamportClock::getTimestampAndAdvance (void)
{
  d_currentTimestamp[d_ourIndex]++;

  vrpn_LamportTimestamp *ret = NULL;
  try { ret = new vrpn_LamportTimestamp(d_numHosts, d_currentTimestamp); }
  catch (...) { return NULL; }
  return ret;
}
  








