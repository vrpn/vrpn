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

CFLAGS = -I../ -g -I/afs/unc/proj/hmd/beta/include
LFLAGS = -L../$(HW_OS) \
	 -L/afs/unc/proj/hmd/beta/lib/$(HW_OS) \
         -L/afs/unc/proj/hmd/lib/$(HW_OS)

.C.o:
	$(CC) $(CFLAGS) -c $*.C

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all:  vrpn_server 

# player: vrpn_player.o
# 	$(CC) $(LFLAGS) -o player vrpn_player.o -lvrpnserver -lsdi -lm

vrpn_server: vrpn.o  midi.o ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o vrpn_server vrpn.o midi.o -lvrpnserver -lsdi -lm -lquat

clean:
	rm -f *.o vrpn_server


