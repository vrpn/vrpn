# Couldn't understand or usefully modify the former makefile.
# Copied the one from ../server_src here and put the former one
# in Makefile.old

##########################
# common definitions. For non-UNC sites, uncomment one of the lines
# that defines hw_os for the machine you are on in the section just
# below. Then, the code should compile in your environment.
#
#HW_OS := sgi_irix
#HW_OS := sparc_solaris
#HW_OS := hp700_hpux10
#HW_OS := pc_linux
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
##########################

ifndef	HW_OS
# hw_os does not exist on FreeBSD at UNC
UNAME := $(shell uname -s)
ifeq ($(UNAME), FreeBSD)
  HW_OS := pc_FreeBSD
else
  # pc_cygwin doesn't have HW_OS
  ifeq ($(UNAME), CYGWIN_NT-4.0)
    HW_OS := pc_cygwin
  else
    ifeq ($(UNAME), CYGWIN_98-4.10)
      HW_OS := pc_cygwin
    else
      HW_OS := $(shell hw_os)
    endif
  endif
endif
endif


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
  ifndef SGI_ARCH
      SGI_ARCH := mips3
  endif
  OBJECT_DIR_SUFFIX := .$(SGI_ABI).$(SGI_ARCH)
  CC := CC -$(SGI_ABI) -$(SGI_ARCH)
  GLUTLIBS := -L/afs/cs.unc.edu/project/walk/lib/sgi_irix.$(SGI_ABI) -lglut -lGLU -lGL -lXmu -lXext -lX11
endif

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

ifeq ($(HW_OS),pc_cygwin)
  ARCH_LIBS := -lcygwin -luser32 -lgdi32 -lcomdlg32 -lwsock32
endif

GLUTINCLUDE = /afs/cs.unc.edu/project/walk/include

CFLAGS = -I../ -I$(GLUTINCLUDE) -g
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
	add_vrpn_cookie testLamport test_mutex $(OBJ_DIR)/setrouter
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

.PHONY: test_mutex
test_mutex:	$(OBJ_DIR)/test_mutex

.PHONY:	text
text:	$(OBJ_DIR)/text

.PHONY:	vrpn_Router_Client_GUI
vrpn_Router_Client_GUI:	$(OBJ_DIR)/vrpn_Router_Client_GUI

$(OBJ_DIR)/sound_client: $(OBJ_DIR)/sound_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sound_client \
		$(OBJ_DIR)/sound_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/vrpn_print_messages: $(OBJ_DIR)/vrpn_print_messages.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_print_messages \
		$(OBJ_DIR)/vrpn_print_messages.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printvals: $(OBJ_DIR)/printvals.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printvals \
		$(OBJ_DIR)/printvals.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/vrpn_Router_Client_GUI: $(OBJ_DIR)/vrpn_Router_Client_GUI.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_Router_Client_GUI \
		$(OBJ_DIR)/vrpn_Router_Client_GUI.o $(GLUTLIBS) -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/setrouter: $(OBJ_DIR)/setrouter.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/setrouter \
		$(OBJ_DIR)/setrouter.o -lvrpn $(ARCH_LIBS)

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

$(OBJ_DIR)/test_mutex: $(OBJ_DIR)/test_mutex.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_mutex \
		$(OBJ_DIR)/test_mutex.o -lvrpn $(ARCH_LIBS)


# These utilities actually DON'T use libvrpn

$(OBJ_DIR)/logfilesenders: $(OBJ_DIR)/logfilesenders.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfilesenders \
		$(OBJ_DIR)/logfilesenders.o

$(OBJ_DIR)/logfiletypes: $(OBJ_DIR)/logfiletypes.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfiletypes $(OBJ_DIR)/logfiletypes.o


clean:
	rm -f $(OBJ_DIR)/*

$(LIB_DIR)/libvrpn.a:


