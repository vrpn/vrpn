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
#HW_OS := sparc_solaris
#HW_OS := hp700_hpux10
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
   ifndef SGI_ABI
      SGI_ABI := n32
   endif
   OBJECT_DIR_SUFFIX := .$(SGI_ABI)
        CC := CC -$(SGI_ABI)
  endif

#ifeq ($(HW_OS),sgi_irix)
#  CC := CC -32
#endif

ifeq ($(HW_OS),hp700_hpux10)
  CC := CC +a1
endif
ifeq ($(HW_OS), hp_flow_aCC)
  CC := /opt/aCC/bin/aCC
endif

ifeq ($(HW_OS),sparc_solaris)
  CC := /opt/SUNWspro/bin/CC
  ARCH_LIBS := -lsocket -lnsl $(ARCH_LIBS)
endif

CFLAGS = -I../ -g
.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CFLAGS) -c $*.C

OBJ_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
LIB_DIR := ../$(OBJ_DIR)

LFLAGS = -g -L$(LIB_DIR) -L../../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) \
	 -L/afs/unc/proj/hmd/beta/lib/$(HW_OS)$(OBJECT_DIR_SUFFIX) \
         -L/afs/unc/proj/hmd/lib/$(HW_OS)$(OBJECT_DIR_SUFFIX)

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.C
	$(CC) $(CFLAGS) -o $@ -c $<

all:	$(OBJ_DIR)/printvals $(OBJ_DIR)/printcereal \
	$(OBJ_DIR)/checklogfile $(OBJ_DIR)/logfilesenders \
	$(OBJ_DIR)/logfiletypes $(OBJ_DIR)/text \
	forwarderClient $(OBJ_DIR)/bdbox_client \
	$(OBJ_DIR)/ff_client \
	$(OBJ_DIR)/phan_client \
	$(OBJ_DIR)/sphere_client $(OBJ_DIR)/bdbox_client \
	add_vrpn_cookie testLamport
#	$(OBJ_DIR)/vrpn_print_messages

# $(OBJ_DIR)/ohm_client

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

# testSharedObject

.PHONY:	forwarderClient
forwarderClient:	$(OBJ_DIR)/forwarderClient

.PHONY:	add_vrpn_cookie
add_vrpn_cookie:	$(OBJ_DIR)/add_vrpn_cookie

.PHONY: testSharedObject
testSharedObject:	$(OBJ_DIR)/testSharedObject

.PHONY: testLamport
testLamport:	$(OBJ_DIR)/testLamport

.PHONY:	text
text:	$(OBJ_DIR)/text

$(OBJ_DIR)/sound_client: $(OBJ_DIR)/sound_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sound_client \
		$(OBJ_DIR)/sound_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/vrpn_print_messages: $(OBJ_DIR)/vrpn_print_messages.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_print_messages \
		$(OBJ_DIR)/vrpn_print_messages.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printvals: $(OBJ_DIR)/printvals.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printvals \
		$(OBJ_DIR)/printvals.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printcereal: $(OBJ_DIR)/printcereal.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printcereal \
		$(OBJ_DIR)/printcereal.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/bdbox_client: $(OBJ_DIR)/bdbox_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/bdbox_client \
		$(OBJ_DIR)/bdbox_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/ff_client: $(OBJ_DIR)/ff_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/ff_client \
		$(OBJ_DIR)/ff_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/ohm_client: $(OBJ_DIR)/ohm_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/ohm_client \
		$(OBJ_DIR)/ohm_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/phan_client: $(OBJ_DIR)/phan_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/phan_client \
		$(OBJ_DIR)/phan_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/sphere_client: $(OBJ_DIR)/sphere_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sphere_client \
		$(OBJ_DIR)/sphere_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/listen_client: $(OBJ_DIR)/listen_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/listen_client \
		$(OBJ_DIR)/listen_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/text: $(OBJ_DIR)/text.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/text \
		$(OBJ_DIR)/text.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/forwarderClient: $(OBJ_DIR)/forwarderClient.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forwarderClient \
		$(OBJ_DIR)/forwarderClient.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/checklogfile: $(OBJ_DIR)/checklogfile.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/checklogfile \
		$(OBJ_DIR)/checklogfile.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/add_vrpn_cookie: $(OBJ_DIR)/add_vrpn_cookie.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/add_vrpn_cookie \
		$(OBJ_DIR)/add_vrpn_cookie.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/testSharedObject: $(OBJ_DIR)/testSharedObject.o \
				$(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testSharedObject \
		$(OBJ_DIR)/testSharedObject.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/testLamport: $(OBJ_DIR)/vrpn_LamportClock.t.o \
				$(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testLamport \
		$(OBJ_DIR)/vrpn_LamportClock.t.o -lvrpn $(ARCH_LIBS)


# These utilities actually DON'T use libvrpn

$(OBJ_DIR)/logfilesenders: $(OBJ_DIR)/logfilesenders.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfilesenders \
		$(OBJ_DIR)/logfilesenders.o

$(OBJ_DIR)/logfiletypes: $(OBJ_DIR)/logfiletypes.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfiletypes $(OBJ_DIR)/logfiletypes.o


clean:
	rm -f $(OBJ_DIR)/*

$(LIB_DIR)/libvrpn.a:


