#ifndef VRPN_SHARED_OBJECT
#define VRPN_SHARED_OBJECT

#include <vrpn_Shared.h>  // for types

#include <vrpn_Connection.h>  // for vrpn_HANDLERPARAM

typedef int (* vrpnSharedIntCallback) (void * userdata, vrpn_int32 newValue);
typedef int (* vrpnSharedFloatCallback) (void * userdata,
                                         vrpn_float64 newValue);
typedef int (* vrpnSharedStringCallback) (void * userdata, char * newValue);

class vrpn_Shared_int32 {


  public:

    vrpn_Shared_int32 (const char * name,
                       int defaultValue = 0);
    virtual ~vrpn_Shared_int32 (void);

    // ACCESSORS

    int value (void) const;
    operator int () const;

    // MANIPULATORS

    virtual vrpn_Shared_int32 & operator = (int &) = 0;

    virtual void bindConnection (vrpn_Connection *);

    void register_handler (vrpnSharedIntCallback, void *);

  protected:

    vrpn_int32 d_value;
    char * d_name;

    vrpn_Connection * d_connection;
    vrpn_int32 d_myId;
    vrpn_int32 d_updateFromServer_type;
    vrpn_int32 d_updateFromRemote_type;

    void encode (char ** buffer, int * len);
    void decode (const char ** buffer, int * len);

    // callback code
    struct callbackEntry {
      vrpnSharedIntCallback handler;
      void * userdata;
      callbackEntry * next;
    };
    callbackEntry * d_callbacks;
        
};

class vrpn_Shared_int32_Server : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Server (const char * name,
                       int defaultValue = 0);
    virtual ~vrpn_Shared_int32_Server (void);

    virtual vrpn_Shared_int32 & operator = (int &);
    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromRemote (void *, vrpn_HANDLERPARAM);
};

class vrpn_Shared_int32_Remote : public vrpn_Shared_int32 {

  public:

    vrpn_Shared_int32_Remote (const char * name,
                       int defaultValue = 0);
    virtual ~vrpn_Shared_int32_Remote (void);

    virtual vrpn_Shared_int32 & operator = (int &);
    virtual void bindConnection (vrpn_Connection *);

  protected:

    static int handle_updateFromServer (void *, vrpn_HANDLERPARAM);
};

#endif  // VRPN_SHARED_OBJECT

