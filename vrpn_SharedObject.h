#ifndef VRPN_SHARED_OBJECT
#define VRPN_SHARED_OBJECT

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/time.h>  // for struct timeval
#endif

#include <vrpn_Shared.h>  // for types
#include <vrpn_Types.h>

#include <vrpn_Connection.h>  // for vrpn_HANDLERPARAM

// It's increasingly clear that we could handle all this with
// a template, except for the fact that vrpn_Shared_String is
// based on char *.  All we need is a String base class.
// We could try to adopt BCString from nano's libnmb...

// I'd like to implement shouldAcceptUpdate/shouldSendUpdate
// with the Strategy pattern (Gamma/Helm/Johnson/Vlissides 1995, pg 315).
// That would make it far, far easier to extend, but the implementation
// looks to unweildy.

class vrpn_Shared_int32;
class vrpn_Shared_float64;
class vrpn_Shared_String;

typedef int (* vrpnSharedIntCallback) (void * userdata, vrpn_int32 newValue);
typedef int (* vrpnSharedFloatCallback) (void * userdata,
                                         vrpn_float64 newValue);
typedef int (* vrpnSharedStringCallback)
                    (void * userdata, const char * newValue);

typedef int (* vrpnTimedSharedIntCallback)
                    (void * userdata, vrpn_int32 newValue,
                     timeval when);
typedef int (* vrpnTimedSharedFloatCallback)
                    (void * userdata, vrpn_float64 newValue,
                     timeval when);
typedef int (* vrpnTimedSharedStringCallback)
                    (void * userdata, const char * newValue,
                     timeval when);

// Update callbacks should return 0 on successful completion,
// nonzero on error (which will prevent further update callbacks
// from being invoked).

typedef int (* vrpnSharedIntSerializerPolicy)
                    (void * userdata, vrpn_int32 newValue,
                     timeval when, vrpn_Shared_int32 * object);
typedef int (* vrpnSharedFloatSerializerPolicy)
                    (void * userdata, vrpn_float64 newValue,
                     timeval when, vrpn_Shared_float64 * object);
typedef int (* vrpnSharedStringSerializerPolicy)
                    (void * userdata, const char * newValue,
                     timeval when, vrpn_Shared_String * object);

// Policy callbacks should return 0 if the update should be accepted,
// nonzero if it should be denied.

#define VRPN_SO_DEFAULT 0x00
#define VRPN_SO_IGNORE_IDEMPOTENT 0x01
#define VRPN_SO_DEFER_UPDATES 0x10
#define VRPN_SO_IGNORE_OLD 0x100

// Each of these flags can be passed to all vrpn_Shared_* constructors.
// If VRPN_SO_IGNORE_IDEMPOTENT is used, calls of operator = (v) or set(v)
// are *ignored* if v == d_value.  No callbacks are called, no network
// traffic takes place.
// If VRPN_SO_DEFER_UPDATES is used, calls of operator = (v) or set(v)
// on vrpn_Shared_*_Remote are sent to the server but not reflected
// locally until an update message is received from the server.
// If VRPN_SO_IGNORE_OLD is set, calls of set(v, t) are ignored if
// t < d_lastUpdate.  This includes messages propagated over the network.

// A vrpn_Shared_*_Server/Remote pair using VRPN_SO_IGNORE_OLD are
// guaranteed to reach the same final state - after quiescence (all messages
// sent on the network are delivered) they will yield the same value(),
// but they are *not* guaranteed to go through the same sequence of
// callbacks.

// Using VRPN_SO_DEFER_UPDATES serializes all changes to d_value and
// all callbacks, so it guarantees that all instances of the shared
// variable see the same sequence of callbacks.

// setSerializerPolicy() can be used to change the way VRPN_SO_DEFER_UPDATES
// operates.  The default value described above is equivalent to calling
// setSerializerPolicy(ACCEPT).  Also possible are DENY,
// which causes the serializer to ignore all updates from its peers,
// and CALLBACK, which passes the update to a callback which can
// return zero for ACCEPT or nonzero for DENY. 

enum vrpn_SerializerPolicy { ACCEPT, DENY, CALLBACK };

class vrpn_Shared_int32 {


  public:

    vrpn_Shared_int32 (const char * name,
                       vrpn_int32 defaultValue = 0,
                       vrpn_int32 mode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32 (void);

    // ACCESSORS

    const char * name (void) const { return d_name; }
    vrpn_int32 value (void) const;
    operator vrpn_int32 () const;

    // MANIPULATORS

    vrpn_Shared_int32 & operator = (vrpn_int32 newValue);
      // calls set(newValue, now);

    vrpn_Shared_int32 & set (vrpn_int32 newValue, timeval when);
      // calls protected set (newValue, when, vrpn_TRUE);

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedIntCallback, void *);
    void unregister_handler (vrpnSharedIntCallback, void *);
    void register_handler (vrpnTimedSharedIntCallback, void *);
    void unregister_handler (vrpnTimedSharedIntCallback, void *);
      // Callbacks are (currently) called *AFTER* the assignment
      // has been made, so any check of the value of their shared int
      // will return newValue

    void setSerializerPolicy (vrpn_SerializerPolicy policy = ACCEPT,
                              vrpnSharedIntSerializerPolicy f = NULL,
                              void * userdata = NULL);

  protected:

    vrpn_int32 d_value;
    char * d_name;
    vrpn_int32 d_mode;
    timeval d_lastUpdate;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;
    vrpn_int32 d_becomeSerializer_type;
    vrpn_int32 d_myUpdate_type;  // fragile

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

    // serializer policy code
    vrpn_SerializerPolicy d_policy;  // default to ACCEPT
    vrpnSharedIntSerializerPolicy d_policyCallback;
    void * d_policyUserdata;
    vrpn_bool d_isSerializer;
      // default to vrpn_TRUE for servers, FALSE for remotes

    vrpn_Shared_int32 & set (vrpn_int32, timeval,
                             vrpn_bool isLocalSet);

    virtual vrpn_bool shouldAcceptUpdate (vrpn_int32 newValue, timeval when,
                                    vrpn_bool isLocalSet);
    virtual vrpn_bool shouldSendUpdate (vrpn_bool isLocalSet,
                                        vrpn_bool acceptedUpdate);

    void sendUpdate (vrpn_int32 messagetype, vrpn_int32 newValue, timeval when);
    void encode (char ** buffer, vrpn_int32 * len,
                 vrpn_int32 newValue, timeval when) const;
      // We used to have sendUpdate() and encode() just read off of
      // d_value and d_lastUpdate, but that doesn't work when we're
      // serializing (VRPN_SO_DEFER_UPDATES), because we don't want
      // to change the local values but do want to send the new values
      // to the serializer.
    void decode (const char ** buffer, vrpn_int32 * len,
                 vrpn_int32 * newValue, timeval * when) const;

    int yankCallbacks (void);
      // must set d_lastUpdate BEFORE calling yankCallbacks()
        
    static int handle_becomeSerializer (void *, vrpn_HANDLERPARAM);
    static int handle_update (void *, vrpn_HANDLERPARAM);
};

// I don't think the derived classes should have to have operator = ()
// defined (they didn't in the last version??), but both SGI and HP
// compilers seem to insist on it.

class vrpn_Shared_int32_Server : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Server (const char * name,
                       vrpn_int32 defaultValue = 0,
                       vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32_Server (void);

    vrpn_Shared_int32_Server & operator = (vrpn_int32 newValue);

    virtual void bindConnection (vrpn_Connection *);

};

class vrpn_Shared_int32_Remote : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Remote (const char * name,
                              vrpn_int32 defaultValue = 0,
                              vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_int32_Remote (void);

    vrpn_Shared_int32_Remote & operator = (vrpn_int32 newValue);

    virtual void bindConnection (vrpn_Connection *);

};



class vrpn_Shared_float64 {


  public:

    vrpn_Shared_float64 (const char * name,
                         vrpn_float64 defaultValue = 0.0,
                         vrpn_int32 mode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_float64 (void);

    // ACCESSORS

    const char * name (void) const { return d_name; }
    vrpn_float64 value (void) const;
    operator vrpn_float64 () const;

    // MANIPULATORS

    vrpn_Shared_float64 & operator = (vrpn_float64 newValue);
      // calls set(newValue, now);

    virtual vrpn_Shared_float64 & set (vrpn_float64 newValue, timeval when);
      // calls protected set (newValue, when, vrpn_TRUE);

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedFloatCallback, void *);
    void unregister_handler (vrpnSharedFloatCallback, void *);
    void register_handler (vrpnTimedSharedFloatCallback, void *);
    void unregister_handler (vrpnTimedSharedFloatCallback, void *);
      // Callbacks are (currently) called *AFTER* the assignment
      // has been made, so any check of the value of their shared int
      // will return newValue

    void setSerializerPolicy (vrpn_SerializerPolicy policy = ACCEPT,
                              vrpnSharedFloatSerializerPolicy f = NULL,
                              void * userdata = NULL);

  protected:

    vrpn_float64 d_value;
    char * d_name;
    vrpn_int32 d_mode;
    timeval d_lastUpdate;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;
    vrpn_int32 d_becomeSerializer_type;
    vrpn_int32 d_myUpdate_type;  // fragile

    // callback code
    // Could generalize this by making a class that gets passed
    // a vrpn_HANDLERPARAM and passes whatever is needed to its callback,
    // but it's not worth doing that unless we need a third or fourth
    // kind of callback.
    struct callbackEntry {
      vrpnSharedFloatCallback handler;
      void * userdata;
      callbackEntry * next;
    };
    callbackEntry * d_callbacks;
    struct timedCallbackEntry {
      vrpnTimedSharedFloatCallback handler;
      void * userdata;
      timedCallbackEntry * next;
    };
    timedCallbackEntry * d_timedCallbacks;

    vrpn_SerializerPolicy d_policy;  // default to ACCEPT
    vrpnSharedFloatSerializerPolicy d_policyCallback;
    void * d_policyUserdata;
    vrpn_bool d_isSerializer;
      // default to vrpn_TRUE for servers, FALSE for remotes

    vrpn_Shared_float64 & set (vrpn_float64, timeval, vrpn_bool isLocalSet);

    virtual vrpn_bool shouldAcceptUpdate (vrpn_float64 newValue, timeval when,
                                          vrpn_bool isLocalSet);
    virtual vrpn_bool shouldSendUpdate (vrpn_bool isLocalSet,
                                        vrpn_bool acceptedUpdate);

    void sendUpdate (vrpn_int32 messagetype, vrpn_float64 newValue,
                     timeval when);
    void encode (char ** buffer, vrpn_int32 * len, vrpn_float64 newValue,
                     timeval when) const;
    void decode (const char ** buffer, vrpn_int32 * len,
                 vrpn_float64 * newValue, timeval * when) const;

    int yankCallbacks (void);
      // must set d_lastUpdate BEFORE calling yankCallbacks()
        
    static int handle_becomeSerializer (void *, vrpn_HANDLERPARAM);
    static int handle_update (void *, vrpn_HANDLERPARAM);
};

class vrpn_Shared_float64_Server : public vrpn_Shared_float64 {

  public:

    vrpn_Shared_float64_Server (const char * name,
                                vrpn_float64 defaultValue = 0,
                                vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_float64_Server (void);

    vrpn_Shared_float64_Server & operator = (vrpn_float64 newValue);

    virtual void bindConnection (vrpn_Connection *);

};

class vrpn_Shared_float64_Remote : public vrpn_Shared_float64 {

  public:

    vrpn_Shared_float64_Remote (const char * name,
                                vrpn_float64 defaultValue = 0,
                                vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_float64_Remote (void);

    vrpn_Shared_float64_Remote & operator = (vrpn_float64 newValue);

    virtual void bindConnection (vrpn_Connection *);

};




class vrpn_Shared_String {


  public:

    vrpn_Shared_String (const char * name,
                         const char * defaultValue = NULL,
                         vrpn_int32 mode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_String (void);

    // ACCESSORS

    const char * name (void) const { return d_name; }
    const char * value (void) const;
    operator const char * () const;

    // MANIPULATORS

    vrpn_Shared_String & operator = (const char * newValue);
      // calls set(newValue, now);

    virtual vrpn_Shared_String & set (const char * newValue, timeval when);
     // calls protected set (newValue, when, vrpn_TRUE);

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedStringCallback, void *);
    void unregister_handler (vrpnSharedStringCallback, void *);
    void register_handler (vrpnTimedSharedStringCallback, void *);
    void unregister_handler (vrpnTimedSharedStringCallback, void *);
      // Callbacks are (currently) called *AFTER* the assignment
      // has been made, so any check of the value of their shared int
      // will return newValue

    void setSerializerPolicy (vrpn_SerializerPolicy policy = ACCEPT,
                              vrpnSharedStringSerializerPolicy f = NULL,
                              void * userdata = NULL);


  protected:

    char * d_value;
    char * d_name;
    vrpn_int32 d_mode;
    timeval d_lastUpdate;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;
    vrpn_int32 d_becomeSerializer_type;
    vrpn_int32 d_myUpdate_type;  // fragile

    // callback code
    // Could generalize this by making a class that gets passed
    // a vrpn_HANDLERPARAM and passes whatever is needed to its callback,
    // but it's not worth doing that unless we need a third or fourth
    // kind of callback.
    struct callbackEntry {
      vrpnSharedStringCallback handler;
      void * userdata;
      callbackEntry * next;
    };
    callbackEntry * d_callbacks;
    struct timedCallbackEntry {
      vrpnTimedSharedStringCallback handler;
      void * userdata;
      timedCallbackEntry * next;
    };
    timedCallbackEntry * d_timedCallbacks;

    vrpn_SerializerPolicy d_policy;  // default to ACCEPT
    vrpnSharedStringSerializerPolicy d_policyCallback;
    void * d_policyUserdata;
    vrpn_bool d_isSerializer;
      // default to vrpn_TRUE for servers, FALSE for remotes

    vrpn_Shared_String & set (const char *, timeval,
                             vrpn_bool isLocalSet);

    virtual vrpn_bool shouldAcceptUpdate (const char * newValue, timeval when,
                                    vrpn_bool isLocalSet);
    virtual vrpn_bool shouldSendUpdate (vrpn_bool isLocalSet,
                                        vrpn_bool acceptedUpdate);



    void sendUpdate (vrpn_int32 messagetype, const char * newValue,
                     timeval when);
    void encode (char ** buffer, vrpn_int32 * len, const char * newValue,
                     timeval when) const;
    void decode (const char ** buffer, vrpn_int32 * len,
                 char * newValue, timeval * when) const;

    int yankCallbacks (void);
      // must set d_lastUpdate BEFORE calling yankCallbacks()
        
    static int handle_becomeSerializer (void *, vrpn_HANDLERPARAM);
    static int handle_update (void *, vrpn_HANDLERPARAM);

};

class vrpn_Shared_String_Server : public vrpn_Shared_String {

  public:

    vrpn_Shared_String_Server (const char * name,
                                const char * defaultValue = NULL,
                                vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_String_Server (void);

    vrpn_Shared_String_Server & operator = (const char *);

    virtual void bindConnection (vrpn_Connection *);

};

class vrpn_Shared_String_Remote : public vrpn_Shared_String {

  public:

    vrpn_Shared_String_Remote (const char * name,
                                const char * defaultValue = NULL,
                                vrpn_int32 defaultMode = VRPN_SO_DEFAULT);
    virtual ~vrpn_Shared_String_Remote (void);

    vrpn_Shared_String_Remote & operator = (const char *);

    virtual void bindConnection (vrpn_Connection *);

};



#endif  // VRPN_SHARED_OBJECT

