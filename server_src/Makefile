CC = CC
CFLAGS = -I../ -g
LFLAGS = -L../`hw_os` \
	 -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.C.o:
	$(CC) $(CFLAGS) -c $*.C

all:  vrpn_server sound_server 

# player: vrpn_player.o
# 	$(CC) $(LFLAGS) -o player vrpn_player.o -lvrpnserver -lsdi -lm

vrpn_server: vrpn.o  
	$(CC) $(LFLAGS) -o vrpn_server vrpn.o -lvrpnserver -lsdi -lm

#client: client.C
#	$(CC) $(CFLAGS) $(LFLAGS) -o client client.C ../`hw_os`/libvrpn.a -lsdi -lm

sound_server: sound_server.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o sound_server sound_server.o -lvrpnserver -lsdi -lm



clean:
	rm -f *.o vrpn_server sound_server 

../`hw_os`/libvrpn.a:

# client.o:	client.C
