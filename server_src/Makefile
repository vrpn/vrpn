CFLAGS = -I../ -g
LFLAGS = -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.C.o:
	g++ $(CFLAGS) -c $*.C

vrpn: vrpn.o vrpn_Tracker.o vrpn_Button.o vrpn_Connection.o
	g++ $(LFLAGS) -o vrpn \
		vrpn.o vrpn_Tracker.o vrpn_Button.o vrpn_Connection.o -lsdi -lm

vrpn_Connection.o: ../vrpn_Connection.C
	g++ $(CFLAGS) -c ../vrpn_Connection.C

vrpn_Button.o: ../vrpn_Button.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Button.C

vrpn_Tracker.o: ../vrpn_Tracker.C ../vrpn_Button.h ../vrpn_Connection.h
	g++ $(CFLAGS) -c ../vrpn_Tracker.C

clean:
	rm -f *.o vrpn

vrpn.o:		vrpn.C ../vrpn_Tracker.h ../vrpn_Connection.h ../vrpn_Button.h
