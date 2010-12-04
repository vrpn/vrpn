*everything* must be built singlethreaded!
this includes vrpn, ausim_sound_server, and client?

make sure that you ignore the correct C runtime libs:
libcmt.lib,msvcrt.lib,libcd.lib,libcmtd.lib,msvcrtd.lib

