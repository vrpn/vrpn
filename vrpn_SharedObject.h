#ifndef VRPN_SHARED_OBJECT
#define VRPN_SHARED_OBJECT

#include <sys/time.h>  // for struct timeval

#include <vrpn_Shared.h>  // for types

#include <vrpn_Connection.h>  // for vrpn_HANDLERPARAM

//#define VRPN_SO_DEFER_UPDATES
// Implementation - if DEFER_UPDATES is set, a Remote will NOT
//   update d_value on assignment, but only in handle_updateFromServer.
// This avoids loops.
// If DEFER_UPDATES is NOT defined, Remotes will immediately update
//   their values, reducing latency.

typedef int (* vrpnSharedIntCallback) (void * userdata, vrpn_int32 newValue);
typedef int (* vrpnSharedFloatCallback) (void * userdata,
                                         vrpn_float64 newValue);
typedef int (* vrpnSharedStringCallback) (void * userdata, char * newValue);
typedef int (* vrpnTimedSharedIntCallback)
                    (void * userdata, vrpn_int32 newValue,
                     timeval when);
typedef int (* vrpnTimedSharedFloatCallback)
                    (void * userdata, vrpn_float64 newValue,
                     timeval when);
typedef int (* vrpnTimedSharedStringCallback)
                    (void * userdata, char * newValue,
                     timeval when);

#define VRPN_SO_DEFAULT 0x00
#define VRPN_SO_IGNORE_IDEMPOTENT 0x01
#define VRPN_SO_DEFER_UPDATES 0x10


class vrpn_Shared_int32 {


  public:

    vrpn_Shared_int32 (const char * name,
                       vrpn_int32 defaultValue = 0,
                       vrpn_int32 mode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32 (void);

    // ACCESSORS

    const char * name (void) const { return d_name; }
    vrpn_int32 value (void) const;
    operator int () const;

    // MANIPULATORS

    vrpn_Shared_int32 & operator = (vrpn_int32);
    virtual vrpn_Shared_int32 & set (vrpn_int32, timeval) = 0;

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedIntCallback, void *);
    void unregister_handler (vrpnSharedIntCallback, void *);
    void register_handler (vrpnTimedSharedIntCallback, void *);
    void unregister_handler (vrpnTimedSharedIntCallback, void *);
      // Callbacks are (currently) called *AFTER* the assignment
      // has been made, so any check of the value of their shared int
      // will return newValue

  protected:

    vrpn_int32 d_value;
    char * d_name;
    vrpn_int32 d_mode;
    timeval d_lastUpdate;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;

    // callback code
    // Could generalize this by making a class that gets passed
    // a vrpn_HANDLERPARAM and passes whatever is needed to its callback,
    // but it's not worth doing that unless we need a third or fourth
    // kind of callback.
    struct callbackEntry {
      vrpnSharedIntCallback handler;
      void * userdata;
      callbackEntry * next;
    };
    callbackEntry * d_callbacks;
    struct timedCallbackEntry {
      vrpnTimedSharedIntCallback handler;
      void * userdata;
      timedCallbackEntry * next;
    };
    timedCallbackEntry * d_timedCallbacks;

    void sendUpdate (vrpn_int32 messagetype);
    void encode (char ** buffer, vrpn_int32 * len) const;
    void decode (const char ** buffer, vrpn_int32 * len);
      // decode() is non-const;  it writes *this as a side-effect.

    int yankCallbacks (void);
      // must set d_lastUpdate BEFORE calling yankCallbacks()
        
};

class vrpn_Shared_int32_Server : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Server (const char * name,
                       vrpn_int32 defaultValue = 0,
                       vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32_Server (void);

    virtual vrpn_Shared_int32 & set (vrpn_int32, timeval);

    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromRemote (void *, vrpn_HANDLERPARAM);
};

class vrpn_Shared_int32_Remote : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Remote (const char * name,
                              vrpn_int32 defaultValue = 0,
                              vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32_Remote (void);

    virtual vrpn_Shared_int32 & set (vrpn_int32, timeval);

    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromServer (void *, vrpn_HANDLERPARAM);
};

#endif  // VRPN_SHARED_OBJECT

