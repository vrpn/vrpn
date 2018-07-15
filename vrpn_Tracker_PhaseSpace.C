/*
  PhaseSpace, Inc 2017
  vrpn_Tracker_PhaseSpace.C

  ChangeLog
  =================
  20170802
  * fixed a bug where timestamp's microsecond field was same as seconds in Windows.

  20170629
  * added support non-fatal errors (i.e. license warnings)
  * added server version query for newer cores that support it.
  * changed indentation of some debugging output.

  20170201
  * removed some extraneous comments

  20151008
  * slave mode now uses cfg specs to assign sensors to markers and rigids.
    Unassigned devices will still be assigned arbitrarily.

  20150818
  * Update to libowl2 API for X2e device support, removed support for Impulse
  * removed stylus and button support
  * removed maximum rigid and marker limitations
  * vrpn.cfg:  x,y, and z parameters for point sensors changed to single pos
    parameter
  * vrpn.cfg:  k1,k2,k3,k4 parameters for rigid sensors changed to single init
    parameter

  */
#include "vrpn_Tracker_PhaseSpace.h"

#include <errno.h>
#include <math.h>

#include <limits>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <vector>

#define MM_TO_METERS (0.001)
#define MSGBUFSIZE 1024

#ifdef VRPN_INCLUDE_PHASESPACE

//
struct SensorInfo {
  vrpn_int32 sensor_id; // vrpn sensor

  int type;             // owl tracker type
  uint32_t tracker;          // tracker_id
  uint32_t id;               // marker_id
  std::string opt;

  size_t search_hint;

  SensorInfo()
  {
    type = -1;
    tracker = 0;
    id = 0;
    opt = "";
    search_hint = std::numeric_limits<uint32_t>::max();
  }
};


//
typedef std::vector<SensorInfo> Sensors;


//
bool operator<(const SensorInfo& a, const SensorInfo& b)
{
  if(a.type < b.type) return true;
  if(a.type > b.type) return false;
  return a.id < b.id;
}

//
class vrpn_Tracker_PhaseSpace::SensorManager
{
protected:
  Sensors _v;

public:
  void add(const SensorInfo& s)
  {
    _v.push_back(s);
    std::sort(_v.begin(), _v.end());
  }

  const SensorInfo* get_by_sensor(int sensor_id)
  {
    for(size_t i = 0; i < _v.size(); i++)
      if(_v[i].sensor_id == sensor_id) return &_v[i];
    return NULL;
  }

  Sensors::iterator begin()
  {
    return _v.begin();
  }

  Sensors::iterator end()
  {
    return _v.end();
  }
};


// ctor
vrpn_Tracker_PhaseSpace::vrpn_Tracker_PhaseSpace(const char *name, vrpn_Connection *c)
  : vrpn_Tracker(name ,c),
    vrpn_Button_Filter(name, c),
    vrpn_Clipping_Analog_Server(name, c),
    debug(false),
    drop_frames(false),
    smgr(NULL)
{
  // TODO fix
  //num_buttons = vrpn_BUTTON_MAX_BUTTONS;
  num_buttons = 0;

  smgr = new SensorManager();

  if(d_connection) {
    // Register a handler for the update change callback
    if (register_autodeleted_handler(update_rate_id, handle_update_rate_request, this, d_sender_id))
      fprintf(stderr,"vrpn_Tracker: Can't register workspace handler\n");
  }
}


// dtor
vrpn_Tracker_PhaseSpace::~vrpn_Tracker_PhaseSpace()
{
  context.done();
  context.close();
}


//
bool vrpn_Tracker_PhaseSpace::InitOWL()
{
  printf("connecting to OWL server at %s...\n", device.c_str());

  std::string init_options = "event.markers=1 event.rigids=1";
  if(options.length()) init_options += " " + options;

  if(drop_frames) printf("drop_frames enabled\n");

  // connect to Impulse server
  if(context.open(device) <= 0)
    {
      fprintf(stderr, "owl connect error: %s\n", context.lastError().c_str());
      return false;
    }

  if(debug)
    {
      std::string coreversion = context.property<std::string>("coreversion");
      printf("[debug] server version: %s\n", coreversion.length() ? coreversion.c_str() : "unknown");

      std::string apiversion = context.property<std::string>("apiversion");

      printf("[debug] API version: %s\n", apiversion.length() ? apiversion.c_str() : "unknown");
    }

  if(debug) printf("[debug] initialization parameters: %s\n", init_options.c_str());
  if(context.initialize(init_options) <= 0)
    {
      fprintf(stderr, "owl init error: %s\n", context.lastError().c_str());
      return false;
    }

  if(options.find("timebase=") == std::string::npos) context.timeBase(1, 1000000);
  if(options.find("scale=") == std::string::npos) context.scale(MM_TO_METERS);

  if(context.lastError().length())
    {
      fprintf(stderr, "owl error: %s\n", context.lastError().c_str());
      return false;
    }

  int slave = context.property<int>("slave");

  if(slave) printf("slave mode enabled.\n");
  else if(!create_trackers()) return false;

  printf("owl initialized\n");
  return true;
}


//
bool vrpn_Tracker_PhaseSpace::create_trackers()
{
  if(debug) printf("[debug] creating trackers...\n");

  // create trackers
  int nr = 0;
  int nm = 0;
  std::vector<uint32_t> ti;

  // create rigid trackers
  for(Sensors::iterator s = smgr->begin(); s != smgr->end(); s++)
    {
      if(s->type != OWL::Type::RIGID) continue;
      nr++;
      if(std::find(ti.begin(), ti.end(), s->tracker) != ti.end())
        {
          fprintf(stderr, "rigid tracker %d already defined\n", s->tracker);
          continue;
        }

      printf("creating rigid tracker %d\n", s->tracker);
      std::string type = "rigid";
      std::stringstream name; name << type << nr;
      if(debug) printf("[debug] type=%s id=%d name=%s options=\'%s\'\n", type.c_str(), s->tracker, name.str().c_str(), s->opt.c_str());

      if(!context.createTracker(s->tracker, type, name.str(), s->opt))
        {
          fprintf(stderr, "tracker creation error: %s\n", context.lastError().c_str());
          return false;
        }
      ti.push_back(s->tracker);
    }

  // create point trackers
  for(Sensors::iterator s = smgr->begin(); s != smgr->end(); s++)
    {
      if(s->type != OWL::Type::MARKER) continue;
      nm++;
      if(std::find(ti.begin(), ti.end(), s->tracker) == ti.end())
        {
          printf("creating point tracker %d\n", s->tracker);
          std::string type = "point";
          std::stringstream name; name << type << nm;
          ti.push_back(s->tracker);
          context.createTracker(s->tracker, type, name.str());
        }
      if(!context.assignMarker(s->tracker, s->id, "", s->opt))
        {
          fprintf(stderr, "marker assignment error: %s\n", context.lastError().c_str());
          return false;
        }
      if(debug) printf("[debug] id=%d tracker=%d options=\'%s\'\n", s->id, s->tracker, s->opt.c_str());
    }

  if(context.lastError().length())
    {
      fprintf(stderr, "tracker creation error: %s\n", context.lastError().c_str());
      return false;
    }

  return true;
}


// This function should be called each time through the main loop
// of the server code. It polls for data from the OWL server and
// sends them if available.
void vrpn_Tracker_PhaseSpace::mainloop()
{
  get_report();
  server_mainloop();
  return;
}


//
std::string trim(char* line, int len)
{
  // cut off leading whitespace
  int s, e;
  for(s = 0; isspace(line[s]) && s < len; s++);
  // cut off comments and trailing whitespace
  e = s;
  for(int i = s; line[i] && i < len; i++)
    {
      if(line[i] == '#') break;
      if(!isspace(line[i])) e = i+1;
    }
  return std::string(line+s, e-s);
}


//
bool read_int(const char* str, int &i)
{
  if(!str) return false;
  errno = 0;
  char* endptr = NULL;
  i = strtol(str, &endptr, 10);
  if(*endptr || errno) {
    fprintf(stderr, "Error, expected an integer but got token: \"%s\"\n", str);
    return false;
  }
  return true;
}


//
bool read_uint(const char* str, uint32_t &i)
{
  if(!str) return false;
  errno = 0;
  char* endptr = NULL;
  i = strtoul(str, &endptr, 10);
  if(*endptr || errno) {
    fprintf(stderr, "Error, expected an unsigned integer but got token: \"%s\"\n", str);
    return false;
  }
  return true;
}


//
bool read_bool(const char* str, bool &b)
{
  if(!str) return false;
  std::string s(str);
  if(s == "true") {
    b = true;
    return true;
  } else if(s == "false") {
    b = false;
    return true;
  }
  int i = 0;
  bool ret = read_int(str, i);
  b = i ? true : false;
  return ret;
}


//
bool read_float(const char* str, float &f)
{
  if(!str) return false;
  errno = 0;
  char* endptr = NULL;
  f = (float) strtod(str, &endptr);
  if(*endptr || errno) {
    fprintf(stderr, "Error, expected a float but got token: \"%s\"\n", str);
    return false;
  }
  return true;
}


// TODO: Replace all of this junk with <regex> once it is standard in gcc
class ConfigParser
{
protected:
  struct Spec {
    std::string key;
    std::string type;
    void* dest;
    bool required;
  };

  std::map <std::string, std::string> keyvals;
  std::stringstream _error;

public:

  std::string error()
  {
    return _error.str();
  }

  bool contains(const std::string &key)
  {
    return keyvals.find(key) != keyvals.end();
  }

  std::string join()
  {
    std::stringstream s;
    for(std::map<std::string, std::string>::iterator i = keyvals.begin(); i != keyvals.end(); i++)
      s << (i==keyvals.begin()?"":" ") << i->first << "=" << i->second;
    return s.str();
  }

  //
  bool parse(SensorInfo &si)
  {
    _error.str("");

    std::string spec_type;
    float x=std::numeric_limits<float>::quiet_NaN();
    float y=std::numeric_limits<float>::quiet_NaN();
    float z=std::numeric_limits<float>::quiet_NaN();

    Spec spec_marker[] = {
      { "led", "uint32_t", &si.id, true },
      { "tracker", "uint32_t", &si.tracker, false },
      { "x", "float", &x, false },
      { "y", "float", &y, false },
      { "z", "float", &z, false },
      { "", "", NULL, false } // sentinel
    };

    Spec spec_rigid[] = {
      { "tracker", "uint32_t", &si.tracker, true },
      { "", "", NULL, false } // sentinel
    };

    if(pop_int("sensor", si.sensor_id))
      {
        _error << "required key 'sensor' not found";
        return false;
      }

    if(pop("type", spec_type))
      {
        _error << "required key 'type' not found";
        return false;
      }

    Spec* spec = NULL;
    if(spec_type == "rigid" || spec_type == "rigid_body")
      {
        si.type = OWL::Type::RIGID;
        spec = spec_rigid;
      }
    else if(spec_type == "point")
      {
        si.type = OWL::Type::MARKER;
        spec = spec_marker;
      }
    else {
      _error << "unknown sensor type: " << spec_type;
      return false;
    }

    for(int i = 0; spec[i].dest; i++)
      {
        int ret = 0;
        if(spec[i].type == "string")
          ret = pop(spec[i].key, *((std::string*) spec[i].dest));
        else if(spec[i].type == "uint32_t")
          ret = pop_uint(spec[i].key, *((uint32_t*) spec[i].dest));
        else if(spec[i].type == "float")
          ret = pop_float(spec[i].key, *((float*) spec[i].dest));
        else
          {
            _error << "unknown spec type: " << spec[i].type;
            return false;
          }
        if(ret == 1)
          {
            if(spec[i].required)
              {
                _error << "required key not found: " << spec[i].key;
                return false;
              }
          }
        else if(ret)
          {
            _error << "error reading value for key \'" << spec[i].key << "'";
            return false;
          }
      }

    if(si.type == OWL::Type::RIGID)
      si.id = si.tracker;

    //special case (legacy)
    if(!isnan(x) || !isnan(y) || !isnan(z))
      {
        if(isnan(x) || isnan(y) || isnan(z))
          {
            _error << "x,y,z keys must all be specified if any are specified.";
            return false;
          }
        if(contains("pos"))
          {
            _error << "pos and x,y,z keys are mutually exclusive.";
            return false;
          }
        std::stringstream pos; pos << x << ',' << y << ',' << z;
        keyvals["pos"] = pos.str();
      }

    si.opt = join();
    return true;
  }

  // returns: 0 on success, 1 on key error
  int pop(std::string key, std::string &val)
  {
    if(keyvals.find(key) == keyvals.end())
      return 1;
    val = keyvals[key];
    keyvals.erase(key);
    return 0;
  }

  // returns: 0 on success, 1 on key error, 2 on parse error
  int pop_int(std::string key, int &n)
  {
    std::string v;
    if(pop(key, v)) return 1;
    return read_int(v.c_str(), n) ? 0 : 2;
  }

  // returns: 0 on success, 1 on key error, 2 on parse error
  int pop_uint(std::string key, uint32_t &n)
  {
    std::string v;
    if(pop(key, v)) return 1;
    return read_uint(v.c_str(), n) ? 0 : 2;
  }

  // returns: 0 on success, 1 on key error, 2 on parse error
  int pop_float(std::string key, float &n)
  {
    std::string v;
    if(pop(key, v)) return 1;
    return read_float(v.c_str(), n) ? 0 : 2;
  }

  // returns: 0 on success, 1 on key error, 2 on parse error
  int pop_bool(std::string key, bool &n)
  {
    std::string v;
    if(pop(key, v)) return 1;
    return read_bool(v.c_str(), n) ? 0 : 2;
  }

  // return index of error, or -1 on success
  int parse_kv(std::string str)
  {
    keyvals.clear();
    _error.str("");

    std::vector<char> current_key;
    std::vector<char> current_val;

    int i = 0;
    while(str[i])
      {
        current_key.clear();
        current_val.clear();
        while(isspace(str[i])) i++;
        while(str[i]) {
          if(str[i] == '"') {
            _error << "key names are not allowed to contain quotes or be contained in quotes.";
            return i;
          } else if(isspace(str[i])) {
            _error << "unexpected whitespace.";
            return i;
          } else if(str[i] == '=') {
            i++;
            break;
          } else current_key.push_back(str[i++]);
        }
        if(!current_key.size()) {
          _error << "empty key name.";
          return i;
        }
        bool quoted = false;
        if(str[i] == '"') {
          quoted = true;
          i++;
        }
        while(str[i]) {
          if(str[i] == '"') {
            if(quoted) {
              quoted = false;
              i++;
              break;
            } else {
              _error << "misplaced quotes.";
              return i;
            }
          } else if(str[i] == '=') {
            _error << "unexpected '=' char in value token.";
            return i;
          } else if(!quoted && isspace(str[i])) break;
          else current_val.push_back(str[i++]);
        }
        if(quoted) {
          _error << "unterminated quotes.";
          return i;
        }
        if(str[i] && !isspace(str[i])) {
          _error << "expected whitespace after value token.";
          return i;
        }
        if(!current_val.size()) {
          _error << "empty value string.";
          return i;
        }
        std::string key = std::string(current_key.data(), current_key.size());
        std::string val = std::string(current_val.data(), current_val.size());

        if(keyvals.find(key) != keyvals.end())
          {
            _error << "duplicate key encountered: '" << key << "'";
            return i;
          }
        keyvals[key] = val;
      }

    return -1;
  }
};


//
bool vrpn_Tracker_PhaseSpace::load(FILE* file)
{
  const int BUFSIZE = 1024;
  char line[BUFSIZE];
  bool inTag = false;

  std::string ln;
  bool eof = false;

  while(!(eof = (fgets(line, BUFSIZE, file) == NULL)))
    {
      ConfigParser parser;
      ln = trim(line, BUFSIZE);

      // skip contentless lines
      if(!ln.length()) continue;

      // skip lines unless we're nested in <owl> tags
      if(ln == "<owl>") {
        if (inTag) {
          fprintf (stderr, "Error, nested <owl> tag encountered.  Aborting...\n");
          return false;
        } else {
          inTag = true;
          continue;
        }
      } else if (ln == "</owl>") {
        if (inTag) {
          if(debug)
            {
              printf("[debug] parsed config file:\n");
              printf("[debug]     %s\n", options.c_str());
              for(Sensors::const_iterator s = smgr->begin(); s != smgr->end(); s++)
                fprintf(stdout, "[debug]     sensor=%d type=%d tracker=%d options=\'%s\'\n", s->sensor_id, s->type, s->tracker, s->opt.c_str());
            }
          // closing tag encountered,  exit.
          return true;
        } else {
          fprintf (stderr, "Error, </owl> tag without <owl> tag.  Aborting...\n");
          return false;
        }
      }

      // parse line for key-value pairs
      if(inTag) {
        int e = parser.parse_kv(ln);
        if(e >= 0) {
          fprintf(stderr, "Error at character %d.\n", e);
          break;
        }
      }

      if(parser.contains("sensor")) {
        // line is a sensor specification
        SensorInfo info;
        if(parser.parse(info))
          {
            // check duplicates
            if(smgr->get_by_sensor(info.sensor_id))
              {
                fprintf(stderr, "duplicate sensor defined: %d\n", info.sensor_id);
                break;
              }
            smgr->add(info);
          }
        else
          {
            fprintf(stderr, "%s\n", parser.error().c_str());
            break;
          }
      } else {
        // line is an option specification
        if(parser.pop("device", device) == 2)
          {
            fprintf(stderr, "error reading value for key 'device'\n");
            break;
          }
        if(parser.pop_bool("drop_frames", drop_frames) == 2)
          {
            fprintf(stderr, "error reading value for key 'drop_frames'\n");
            break;
          }
        if(parser.pop_bool("debug", debug) == 2)
          {
            fprintf(stderr, "error reading value for key 'debug'\n");
            break;
          }


        std::string new_options = parser.join();
        if(new_options.length()) options += (options.length()?" ":"") + parser.join();
      }
    }

  if(eof) fprintf(stderr, "Unexpected end of file.\n");
  else fprintf(stderr, "Unable to parse line: \"%s\"\n", ln.c_str());
  return false;
}


//
bool vrpn_Tracker_PhaseSpace::enableStreaming(bool enable)
{
  if(!context.isOpen()) return false;

  context.streaming(enable ? 1 : 0);
  printf("streaming: %d\n", enable);

  if(context.lastError().length())
    {
      fprintf(stderr, "owl error: %s\n", context.lastError().c_str());
      return false;
    }

  return true;
}


//
void vrpn_Tracker_PhaseSpace::set_pose(const OWL::Rigid &r)
{
  //set the position
  pos[0] = r.pose[0];
  pos[1] = r.pose[1];
  pos[2] = r.pose[2];

  //set the orientation quaternion
  //OWL has the scale factor first, whereas VRPN has it last.
  d_quat[0] = r.pose[4];
  d_quat[1] = r.pose[5];
  d_quat[2] = r.pose[6];
  d_quat[3] = r.pose[3];
}

//
void vrpn_Tracker_PhaseSpace::report_marker(vrpn_int32 sensor, const OWL::Marker &m)
{
  d_sensor = sensor;

  if(debug)
    {
      int tr = context.markerInfo(m.id).tracker_id;
      printf("[debug] sensor=%d type=point tracker=%d led=%d x=%f y=%f z=%f cond=%f\n", d_sensor, tr, m.id, m.x, m.y, m.z, m.cond);
    }

  if(m.cond <= 0) return;

  pos[0] = m.x;
  pos[1] = m.y;
  pos[2] = m.z;

  //raw positions have no rotation
  d_quat[0] = 0;
  d_quat[1] = 0;
  d_quat[2] = 0;
  d_quat[3] = 1;

  send_report();
}


//
void vrpn_Tracker_PhaseSpace::report_rigid(vrpn_int32 sensor, const OWL::Rigid& r, bool is_stylus)
{
  d_sensor = sensor;

  if(debug)
    {
  printf("[debug] sensor=%d type=rigid tracker=%d x=%f y=%f z=%f w=%f a=%f b=%f c=%f cond=%f\n", d_sensor, r.id, r.pose[0], r.pose[1], r.pose[2], r.pose[3], r.pose[4], r.pose[5], r.pose[6], r.cond);
    }

  if(r.cond <= 0) return;

  // set the position/orientation
  set_pose(r);
  send_report();
}


//
void vrpn_Tracker_PhaseSpace::send_report(void)
{
  if(d_connection)
    {
      char	msgbuf[MSGBUFSIZE];
      int	len = vrpn_Tracker::encode_to(msgbuf);
      if(d_connection->pack_message(len, vrpn_Tracker::timestamp, position_m_id, d_sender_id, msgbuf,
                                    vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(stderr,"PhaseSpace: cannot write message: tossing\n");
      }
    }
}


//
void vrpn_Tracker_PhaseSpace::report_button(vrpn_int32 sensor, int value)
{
  if(d_sensor < 0 || d_sensor >= vrpn_BUTTON_MAX_BUTTONS)
    {
      fprintf(stderr, "error: sensor %d exceeds max button count\n", sensor);
      return;
    }

  d_sensor = sensor;
  buttons[d_sensor] = value;

  if(debug)
    {
      printf("[debug] button %d 0x%x\n", d_sensor, value);
    }
}


//
void vrpn_Tracker_PhaseSpace::report_button_analog(vrpn_int32 sensor, int value)
{
  d_sensor = sensor;
  setChannelValue(d_sensor, value);

  if(debug)
    {
      printf("[debug] analog button %d 0x%x\n", d_sensor, value);
    }
}


//
template<class A>
const A* find(int id, size_t& hint, std::vector<A> &data)
{
  if(hint >= data.size() || id != data[hint].id)
    {
      for(size_t i = 0; i < data.size(); i++)
        {
          const A &d = data[i];
          if(d.id == id)
            {
              hint = i;
              return &d;
            }
        }
      return NULL;
    }
  return &data[hint];
}

//
int vrpn_Tracker_PhaseSpace::get_report(void)
{
  if(!context.isOpen()) return 0;

  int maxiter = 1;

  // set limit on looping
  if(drop_frames) maxiter = 10000;

  // read new data
  const OWL::Event *event = NULL;

  {
    const OWL::Event *e = NULL;
    int count = 0;
    while(context.isOpen() && context.property<int>("initialized") && (e = context.nextEvent()))
      {
        if(e->type_id() == OWL::Type::FRAME) event = e;
        else if(e->type_id() == OWL::Type::ERROR)
          {
            std::string err;
            e->get(err);
            fprintf(stderr, "owl error event: %s\n", err.c_str());
            if(e->name() == "fatal")
              {
                fprintf(stderr, "stopping...\n");
                context.done();
                context.close();
                return -1;
              }
          }
        else if(e->type_id() == OWL::Type::BYTE)
          {
            std::string s; e->get(s);
            printf("%s: %s\n", e->name(), s.c_str());

            if(strcmp(e->name(), "done") == 0)
              return 0;
          }
        if(maxiter && ++count >= maxiter) break;
      }
  }

  if(!event) return 0;

  // set timestamp
#if WIN32 // msvc 2008
  vrpn_Tracker::timestamp.tv_sec =  (long) event->time() / 1000000;
  vrpn_Tracker::timestamp.tv_usec = (long) event->time() % 1000000;
#else
  lldiv_t divresult = lldiv(event->time(), 1000000);
  vrpn_Tracker::timestamp.tv_sec = divresult.quot;
  vrpn_Tracker::timestamp.tv_usec = divresult.rem;
#endif

  if(debug)
    {
      printf("[debug] time=%ld.%06ld\n", vrpn_Tracker::timestamp.tv_sec, vrpn_Tracker::timestamp.tv_usec);
    }

  OWL::Markers markers;
  OWL::Rigids rigids;

  for(const OWL::Event *e = event->begin(); e != event->end(); e++)
    {
      if(e->type_id() == OWL::Type::MARKER)
        e->get(markers);
      else if(e->type_id() == OWL::Type::RIGID)
        e->get(rigids);
    }

  int slave = context.property<int>("slave");
  std::vector<const OWL::Marker*> reported_markers;
  std::vector<const OWL::Rigid*> reported_rigids;
  int sensor = 0;

  for(Sensors::iterator s = smgr->begin(); s != smgr->end(); s++)
    {
      switch(s->type)
        {
        case OWL::Type::MARKER:
          {
            const OWL::Marker *m = find<OWL::Marker>(s->id, s->search_hint, markers);
            if(m) {
              report_marker(s->sensor_id, *m);
              if(slave) reported_markers.push_back(m);
            }
          }
          break;
        case OWL::Type::RIGID:
          {
            const OWL::Rigid *r = find<OWL::Rigid>(s->id, s->search_hint, rigids);
            if(r) {
              report_rigid(s->sensor_id, *r);
              if(slave) reported_rigids.push_back(r);
            }
          }
          break;
        default:
          break;
        }

      // TODO implement styluses

      if(s->sensor_id > sensor) sensor = s->sensor_id;
    }

  if(slave)
    {
      // in slave mode, client doesn't necessarily know what the incoming data will be.
      // Report the ones not specified in the cfg file
      if(sensor < 1000) sensor = 1000; // start arbitrarily at 1000
      for(OWL::Markers::iterator m = markers.begin(); m != markers.end(); m++)
        if(std::find(reported_markers.begin(), reported_markers.end(), &*m) == reported_markers.end())
          report_marker(sensor++, *m);
      for(OWL::Rigids::iterator r = rigids.begin(); r != rigids.end(); r++)
        if(std::find(reported_rigids.begin(), reported_rigids.end(), &*r) == reported_rigids.end())
          report_rigid(sensor++, *r);
    }

  // finalize button and analog reports
  vrpn_Button_Filter::report_changes();
  vrpn_Clipping_Analog_Server::report_changes();

  if(context.lastError().length())
    {
      printf("owl error: %s\n", context.lastError().c_str());
      return -1;
    }

  if(!context.property<int>("initialized"))
    return -1;

  return (markers.size() || rigids.size()) ? 1 : 0;
}


//
int vrpn_Tracker_PhaseSpace::handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p)
{
  vrpn_Tracker_PhaseSpace* thistracker = (vrpn_Tracker_PhaseSpace*)userdata;
  if(thistracker->debug) {
    printf("[debug] vrpn_Tracker_PhaseSpace::handle_update_rate_request\n");
  }
  vrpn_float64 update_rate = 0;
  vrpn_unbuffer(&p.buffer,&update_rate);
  thistracker->context.frequency((float) update_rate);
  return 0;
}

#endif // VRPN_INCLUDE_PHASESPACE
