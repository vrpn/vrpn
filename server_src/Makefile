# this is a gmake file

HW_OS := $(shell /usr/local/bin/hw_os)
CC := g++
ifeq ($(HW_OS),sgi_irix)
	CC := CC
endif
ifeq ($(HW_OS),hp700_hpux10)
	CC := CC +a1
endif
ifeq ($(HW_OS),sparc_sunos)
	CC := /usr/local/lib/CenterLine/bin/CC
endif

CFLAGS = -I../ -g
LFLAGS = -L../$(HW_OS) \
	 -L/afs/unc/proj/hmd/beta/lib/$(HW_OS) \
         -L/afs/unc/proj/hmd/lib/$(HW_OS)

.C.o:
	$(CC) $(CFLAGS) -c $*.C

all:  vrpn_server sound_server 

# player: vrpn_player.o
# 	$(CC) $(LFLAGS) -o player vrpn_player.o -lvrpnserver -lsdi -lm

vrpn_server: vrpn.o  ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o vrpn_server vrpn.o -lvrpnserver -lsdi -lm

#client: client.C
#	$(CC) $(CFLAGS) $(LFLAGS) -o client client.C ../$(HW_OS)/libvrpn.a -lsdi -lm

sound_server: sound_server.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o sound_server sound_server.o -lvrpnserver -lsdi -lm



clean:
	rm -f *.o vrpn_server sound_server 


# client.o:	client.C
vprn.o: vrpn.C 
vrpn.C: ../ghost.h ../plane.h ../vrpn_3Space.h ../vrpn_Button.h ../vrpn_Clock.h ../vrpn_Connection.h ../vrpn_Dyna.h \
../vrpn_Flock.h ../vrpn_Flock_Parallel.h ../vrpn_ForceDevice.h ../vrpn_Shared.h \
../vrpn_Sound.h ../vrpn_Tracker.h ../vrpn_Tracker_Ceiling.h ../vrpn_Tracker_Fastrak.h
