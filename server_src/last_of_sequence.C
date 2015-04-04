// Demo program last_of_sequence
// Shows how to ensure that only the last of a sequence of messages of the
// same type that are waiting to be delivered are actually delivered.
// Minimizes latency.  This is actually easier than the most recent period
// of that type, since that requires lots of lookahead if we want to maintain
// ordering.

// We register two types of messages, foo and bar.
// We expect the client to send streams of many messages of type foo
// punctuated by messages of type bar.
// Every time mainloop is called, we execute all received bar messages,
// the last foo message before each bar message, and the last foo message.

//  f1 f2 f3 m f4 f5 f6 m b1 f7 f8 m f9 f10 b2 m f11 b3 b4 f12 m
//  f13 f14 f15 f16 m b5 f17 f18 b6 m f19 b7 f20 f21 b8 m f22 f23 f24 b9 m
//    produces
//  f3;  f6;  b1 f8;  f10 b2;  f11 b3 b4 f12;  f16;  b5 f18 b6;
//  f19 b7 f21 b8;  f24 b9

// There is one implementation dependency in this example:
//   We require that handle_anything_but_foo is called before handle_bar
// to guarantee that ordering is preserved.
//   Currently, all callbacks on vrpn_ANY_TYPE are triggered before
// callbacks on individual types.  We are in the process of adding to the
// spec the guarantee that if this changes it will be to triggering callbacks
// in the exact order registered.  So long as one of those two callback
// ordering policies are followed, this code should work.

#include <vrpn_Connection.h>
#include <string.h>

static vrpn_HANDLERPARAM g_fooStore;
static void * g_fooData;

static vrpn_int32 g_foo_type;
static vrpn_int32 g_bar_type;

int VRPN_CALLBACK handle_real_foo (void * userdata, vrpn_HANDLERPARAM p);

int VRPN_CALLBACK handle_anything_but_foo (void *, vrpn_HANDLERPARAM p) {
  int retval = 0;

  if (p.type == g_foo_type) {
    return retval;
  }

  if (g_fooStore.type == g_foo_type) {
    retval = handle_real_foo(g_fooData, g_fooStore);
    g_fooStore.type = -1;
  }

  return retval;
}

int VRPN_CALLBACK handle_bar (void *, vrpn_HANDLERPARAM p) {

  printf("BAR!  At time %ld.%ld.\n", static_cast<long>(p.msg_time.tv_sec),
         static_cast<long>(p.msg_time.tv_usec));

  return 0;
}

int VRPN_CALLBACK handle_potential_foo (void * userdata, vrpn_HANDLERPARAM p) {

  // HACK - we don't need to delete and new this every time
  // if we keep track of its real length.
  if (g_fooStore.buffer) {
    delete [] g_fooStore.buffer;
  }

  g_fooStore = p;

  // HACK - this probably needs to be aligned
  g_fooStore.buffer = new char [p.payload_len];
  if (!g_fooStore.buffer) {
    fprintf(stderr, "handle_potential_foo:  out of memory!\n");
    return -1;
  }
  memcpy(const_cast<char *>(g_fooStore.buffer), p.buffer, p.payload_len);

  g_fooData = userdata;

  return 0;
}

int VRPN_CALLBACK handle_real_foo (void *, vrpn_HANDLERPARAM p) {

  printf("FOO!  At time %ld.%ld.\n", static_cast<long>(p.msg_time.tv_sec),
         static_cast<long>(p.msg_time.tv_usec));

  return 0;
}

int main (int, char **) {

  vrpn_Connection * c;
  vrpn_int32 myId;

  c = vrpn_create_server_connection();

  myId = c->register_sender ("Me!");
  g_foo_type = c->register_message_type ("Foo?");
  g_bar_type = c->register_message_type ("Bar bar bar bar bar");

  c->register_handler(vrpn_ANY_TYPE, handle_anything_but_foo, NULL);
  c->register_handler(g_foo_type, handle_potential_foo, NULL);
  c->register_handler(g_bar_type, handle_bar, NULL);

  while (1) {
    c->mainloop();

    if (g_fooStore.type == g_foo_type) {
      handle_real_foo(g_fooData, g_fooStore);
      g_fooStore.type = -1;
    }
  }
}
