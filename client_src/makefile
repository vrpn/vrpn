CFLAGS = -I./ -I../ -I/afs/unc/proj/hmd/include/ -g
CC = g++ -g
LIBVRPN = ../`hw_os`/libvrpn.a
HMDLIB = -L/afs/unc/proj/hmd/beta/lib/`hw_os` -L/afs/unc/proj/hmd/lib/`hw_os`

all:
	make sound_client printvals

clean:
	\rm -f printvals sound_client *.o

sound_client: sound_client.o $(LIBVRPN)
	$(CC) $(CFLAGS) -o sound_client sound_client.o $(HMDLIB) $(LIBVRPN) -lsdi -lm

printvals: printvals.o $(LIBVRPN)
	\rm -f printvals
	$(CC) -o printvals printvals.o $(HMDLIB) $(LIBVRPN) -lsdi -lm

$(LIBVRPN):
	(cd ..; gmake)

