# Couldn't understand or usefully modify the former makefile.
# Copied the one from ../server_src here and put the former one
# in Makefile.old

CC = CC
CFLAGS = -I../ -g
LFLAGS = -L../`hw_os` \
	 -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CFLAGS) -c $*.C

all:  sound_client 

sound_client: sound_client.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o sound_client sound_client.o -lvrpn -lsdi -lm

listen_client: listen_client.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o listen_client listen_client.o -lvrpn -lsdi -lm



clean:
	rm -f *.o sound_client 


../`hw_os`/libvrpn.a:


