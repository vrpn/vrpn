CC = g++
CFLAGS = -I../ -g
LFLAGS = -L../`hw_os` \
	 -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.C.o:
	$(CC) $(CFLAGS) -c $*.C

all:  sound_server vrpn_server

# player: vrpn_player.o
# 	g++ $(LFLAGS) -o player vrpn_player.o -lvrpn -lsdi -lm

vrpn_server: vrpn.o  ../`hw_os`/libvrpn.a
	g++ $(LFLAGS) -o vrpn_server vrpn.o -lvrpnserver -lsdi -lm

#client: client.C
#	$(CC) $(CFLAGS) $(LFLAGS) -o client client.C ../`hw_os`/libvrpn.a -lsdi -lm

sound_server: sound_server.o ../`hw_os`/libvrpn.a
	g++ $(LFLAGS) -o sound_server sound_server.o -lvrpnserver -lsdi -lm



clean:
	rm -f *.o vrpn_server sound_server 

../`hw_os`/libvrpn.a:

# client.o:	client.C
