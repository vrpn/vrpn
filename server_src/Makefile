CFLAGS = -I../ -g
LFLAGS = -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.C.o:
	g++ $(CFLAGS) -c $*.C

sound_server: sound_server.o vrpn_Tracker.o vrpn_Button.o vrpn_Connection.o vrpn_Sound.o
	g++ $(LFLAGS) -o sound_server \
		sound_server.o vrpn_Tracker.o vrpn_Button.o vrpn_Connection.o vrpn_Sound.o -lsdi -lm

player: vrpn_player.o
	g++ $(LFLAGS) -o player vrpn_player.o ../`hw_os`/libvrpn.a -lsdi -lm

vrpn: vrpn.o 
	g++ $(LFLAGS) -o vrpn vrpn.o ../`hw_os`/libvrpn.a -lsdi -lm

client: client.C
	g++ $(CFLAGS) $(LFLAGS) -o client client.C ../`hw_os`/libvrpn.a -lsdi -lm

vrpn_Connection.o: ../vrpn_Connection.C
	g++ $(CFLAGS) -c ../vrpn_Connection.C

vrpn_Button.o: ../vrpn_Button.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Button.C

vrpn_Tracker.o: ../vrpn_Tracker.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Tracker.C

vrpn_3Space.o: ../vrpn_3Space.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_3Space.C

vrpn_Shared.o: ../vrpn_Shared.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Shared.C

vrpn_Sound.o: ../vrpn_Sound.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Sound.C

clean:
	rm -f *.o vrpn

vrpn.o:		vrpn.C ../vrpn_Tracker.h ../vrpn_Connection.h ../vrpn_Button.h ../vrpn_Sound.h
sound_server.o:		sound_server.C ../vrpn_Tracker.h ../vrpn_Connection.h ../vrpn_Button.h ../vrpn_Sound.h
client.o:	client.C
