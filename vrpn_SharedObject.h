#ifndef VRPN_SHARED_OBJECT
#define VRPN_SHARED_OBJECT

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

class vrpn_Shared_int32 {


  public:

    vrpn_Shared_int32 (const char * name,
                       vrpn_int32 defaultValue = 0);
    virtual ~vrpn_Shared_int32 (void);

    // ACCESSORS

    const char * name (void) const { return d_name; }
    vrpn_int32 value (void) const;
    operator int () const;

    // MANIPULATORS

    virtual vrpn_Shared_int32 & operator = (vrpn_int32) = 0;

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedIntCallback, void *);
    void unregister_handler (vrpnSharedIntCallback, void *);
      // Callbacks are (currently) called *AFTER* the assignment
      // has been made, so any check of the value of their shared int
      // will return newValue

  protected:

    vrpn_int32 d_value;
    char * d_name;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;

    // callback code
    struct callbackEntry {
      vrpnSharedIntCallback handler;
      void * userdata;
      callbackEntry * next;
    };
    callbackEntry * d_callbacks;

    void encode (char ** buffer, vrpn_int32 * len) const;
    void decode (const char ** buffer, vrpn_int32 * len);
      // decode() is non-const;  it writes *this as a side-effect.

    int yankCallbacks (void);
        
};

class vrpn_Shared_int32_Server : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Server (const char * name,
                       vrpn_int32 defaultValue = 0);
    virtual ~vrpn_Shared_int32_Server (void);

    virtual vrpn_Shared_int32 & operator = (vrpn_int32);
    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromRemote (void *, vrpn_HANDLERPARAM);
};

class vrpn_Shared_int32_Remote : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Remote (const char * name,
                              vrpn_int32 defaultValue = 0);
    virtual ~vrpn_Shared_int32_Remote (void);

    virtual vrpn_Shared_int32 & operator = (vrpn_int32);
    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromServer (void *, vrpn_HANDLERPARAM);
};

#endif  // VRPN_SHARED_OBJECT

