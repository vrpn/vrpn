# Couldn't understand or usefully modify the former makefile.
# Copied the one from ../server_src here and put the former one
# in Makefile.old

##########################
# common definitions. For non-UNC sites, uncomment one of the lines
# that defines hw_os for the machine you are on in the section just
# below. Then, the code should compile in your environment.
#
HW_OS := $(shell hw_os)
#HW_OS := sgi_irix
#HW_OS := pc_linux
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
##########################


# check if its for pxfl
ifdef PBASE_ROOT
  HW_OS := hp_flow
  ifeq ($(PXFL_COMPILER), aCC)
    HW_OS = hp_flow_aCC
  endif
endif

CC := g++
ARCH_LIBS := -lquat -lm

ifeq ($(HW_OS),pc_linux)
  CC := g++
endif
ifeq ($(HW_OS),sgi_irix)
  CC := CC -32
endif
ifeq ($(HW_OS),hp700_hpux10)
  CC := CC +a1
endif
ifeq ($(HW_OS), hp_flow_aCC)
  CC := /opt/aCC/bin/aCC
endif

ifeq ($(HW_OS),sparc_solaris)
  ARCH_LIBS := -lsocket -lnsl $(ARCH_LIBS)
endif

OBJ_DIR := $(HW_OS)

CFLAGS = -I../ -g
LFLAGS = -L../$(HW_OS) -L../../quat/$(HW_OS) \
	 -L/afs/unc/proj/hmd/beta/lib/$(HW_OS) \
         -L/afs/unc/proj/hmd/lib/$(HW_OS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CFLAGS) -c $*.C

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.C
	$(CC) $(CFLAGS) -o $@ -c $<

all:	$(OBJ_DIR)/printvals $(OBJ_DIR)/printcereal \
	$(OBJ_DIR)/checklogfile $(OBJ_DIR)/logfilesenders \
	$(OBJ_DIR)/logfiletypes $(OBJ_DIR)/text \
	forwarderClient $(OBJ_DIR)/bdbox_client \
	$(OBJ_DIR)/ff_client \
	$(OBJ_DIR)/ohm_client $(OBJ_DIR)/phan_client \
	$(OBJ_DIR)/sphere_client $(OBJ_DIR)/bdbox_client \
	add_vrpn_cookie

# testSharedObject

.PHONY:	forwarderClient
forwarderClient:	$(OBJ_DIR)/forwarderClient

.PHONY:	add_vrpn_cookie
add_vrpn_cookie:	$(OBJ_DIR)/add_vrpn_cookie

.PHONY: testSharedObject
testSharedObject:	$(OBJ_DIR)/testSharedObject

.PHONY:	text
text:	$(OBJ_DIR)/text

$(OBJ_DIR)/sound_client: $(OBJ_DIR)/sound_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sound_client \
		$(OBJ_DIR)/sound_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printvals: $(OBJ_DIR)/printvals.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printvals \
		$(OBJ_DIR)/printvals.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printcereal: $(OBJ_DIR)/printcereal.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printcereal \
		$(OBJ_DIR)/printcereal.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/bdbox_client: $(OBJ_DIR)/bdbox_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/bdbox_client \
		$(OBJ_DIR)/bdbox_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/ff_client: $(OBJ_DIR)/ff_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/ff_client \
		$(OBJ_DIR)/ff_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/ohm_client: $(OBJ_DIR)/ohm_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/ohm_client \
		$(OBJ_DIR)/ohm_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/phan_client: $(OBJ_DIR)/phan_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/phan_client \
		$(OBJ_DIR)/phan_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/sphere_client: $(OBJ_DIR)/sphere_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sphere_client \
		$(OBJ_DIR)/sphere_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/listen_client: $(OBJ_DIR)/listen_client.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/listen_client \
		$(OBJ_DIR)/listen_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/text: $(OBJ_DIR)/text.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/text \
		$(OBJ_DIR)/text.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/forwarderClient: $(OBJ_DIR)/forwarderClient.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forwarderClient \
		$(OBJ_DIR)/forwarderClient.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/checklogfile: $(OBJ_DIR)/checklogfile.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/checklogfile \
		$(OBJ_DIR)/checklogfile.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/add_vrpn_cookie: $(OBJ_DIR)/add_vrpn_cookie.o ../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/add_vrpn_cookie \
		$(OBJ_DIR)/add_vrpn_cookie.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/testSharedObject: $(OBJ_DIR)/testSharedObject.o \
				../$(HW_OS)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testSharedObject \
		$(OBJ_DIR)/testSharedObject.o -lvrpn $(ARCH_LIBS)


# These utilities actually DON'T use libvrpn

$(OBJ_DIR)/logfilesenders: $(OBJ_DIR)/logfilesenders.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfilesenders \
		$(OBJ_DIR)/logfilesenders.o

$(OBJ_DIR)/logfiletypes: $(OBJ_DIR)/logfiletypes.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfiletypes $(OBJ_DIR)/logfiletypes.o


clean:
	rm -f $(OBJ_DIR)/*

../$(HW_OS)/libvrpn.a:


