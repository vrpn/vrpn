
CC = cc
CXX = CC

# SGI IRIX 6.x compile flags
CFLAGS =   -n32 -mips3 -O
CCFLAGS =  -n32 -mips3 -O


VRPNDIR = ../..
VRPNINC = -I$(VRPNDIR)
VRPNLIB = -L$(VRPNDIR)/sgi_irix.n32.mips3 -lvrpn

QUATDIR = ../../../quat
QUATINC = -I$(QUATDIR)
QUATLIB = -L$(QUATDIR)/sgi_irix.n32.mips3 -lquat

INCS = $(VRPNINC) $(QUATINC)
LIBS = $(VRPNLIB) $(QUATLIB) -lm

OBJS = testvrpn.o phantom-field.o phantom-plane.o phantom-wave.o \
	glwin.o glvrpn.o smdvrpn.o
BINS = testvrpn phantom-field phantom-plane phantom-wave glvrpn smdvrpn

all: $(BINS) $(OBJS)

.c.o :
	$(CC) $(CCFLAGS) $(INCS) -c $*.c

.C.o :
	$(CXX) $(CCFLAGS) $(INCS) -c $*.C

glwin.o : glwin.c glwin.h
	$(CC) $(CCFLAGS) -DUSEOPENGL $(INCS) -c $*.c

testvrpn : testvrpn.o
	$(CXX) $(CCFLAGS) $(INCS) testvrpn.o $(LIBS) -o $@

phantom-field : phantom-field.o
	$(CXX) $(CCFLAGS) $(INCS) phantom-field.o $(LIBS) -o $@

phantom-plane : phantom-plane.o
	$(CXX) $(CCFLAGS) $(INCS) phantom-plane.o $(LIBS) -o $@

phantom-wave : phantom-wave.o
	$(CXX) $(CCFLAGS) $(INCS) phantom-wave.o $(LIBS) -o $@

glvrpn : glvrpn.o glwin.o
	$(CXX) $(CCFLAGS) $(INCS) glvrpn.o glwin.o $(LIBS) -lGL -lGLU -lX11 -o $@

smdvrpn: smdvrpn.o glwin.o
	$(CXX) $(CCFLAGS) $(INCS) smdvrpn.o glwin.o $(LIBS) -lGL -lGLU -lX11 -o $@


clean :
	rm -f $(OBJS) $(BINS)
