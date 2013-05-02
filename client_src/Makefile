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
#HW_OS := sparc_solaris_64
#HW_OS := hp700_hpux10
#HW_OS := pc_linux
#HW_OS := pc_linux64
#HW_OS := pc_linux_ia64
# Try using this to cross-compile for ARM on a PC running Linux
#HW_OS := pc_linux_arm
# Try using this to cross-compile for ARM on a PC running Cygwin
#HW_OS := pc_cygwin_arm
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
#HW_OS := powerpc_aix
#HW_OS := powerpc_macosx
#HW_OS := universal_macosx
#HW_OS := macosx_32_64
#HW_OS := macosx_64
##########################

INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
INCLUDE_DIR := $(INSTALL_DIR)/include
LIB_DIR := $(INSTALL_DIR)/lib

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
ARCH_LIBS := -lpthread -lquat -lm

ifeq ($(HW_OS),pc_linux)
  CC := g++
endif

ifeq ($(HW_OS),pc_linux_ia64)
  CC := g++
endif

ifeq ($(HW_OS), pc_linux_arm)
	CC := arm-linux-g++
	RANLIB := arm-linux-ranlib
	AR := arm-linux-ar ruv
endif

ifeq ($(HW_OS), pc_cygwin_arm)
	CC := arm-unknown-linux-gnu-g++
	RANLIB := arm-unknown-linux-gnu-ranlib
	AR := arm-unknown-linux-gnu-ar ruv
endif

ifeq ($(HW_OS),pc_linux64)
  CC := g++ -m64
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
  GLUTLIBS := -lglut -lGLU -lGL -lXmu -lXext -lX11
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

ifeq ($(HW_OS),sparc_solaris_64)
  CC := /opt/SUNWspro/bin/CC -xarch=v9a
  AR := /opt/SUNWspro/bin/CC -xarch=v9a -xar -o
  ARCH_LIBS := -lsocket -lnsl $(ARCH_LIBS)
endif

ifeq ($(HW_OS),powerpc_aix)
#  CC := /usr/ibmcxx/bin/xlC_r -g -qarch=pwr3 -w -+
  CC := /usr/ibmcxx/bin/xlC_r $(DEBUG_FLAGS) -qarch=pwr3 -w -+
endif

ifeq ($(HW_OS),pc_cygwin)
  ARCH_LIBS := -lquat -lcygwin -luser32 -lgdi32 -lcomdlg32 -lwsock32
endif

#CFLAGS = -I../ -I../quat -g
override CFLAGS := -I../ -I../quat $(DEBUG_FLAGS) $(CFLAGS)
override CXXFLAGS := -I../ -I../quat $(DEBUG_FLAGS) $(CXXFLAGS)

.SUFFIXES:	.cpp

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CXXFLAGS) -c $*.C

.cpp.o:
	$(CC) $(CXXFLAGS) -c $*.cpp

OBJ_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
LIB_DIR := ../$(OBJ_DIR)

#LFLAGS = -g -L$(LIB_DIR) -L../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX)
override LFLAGS := -L$(LIB_DIR) -L../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) $(DEBUG_FLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.C
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.cpp
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

INSTALL_APPS := vrpn_print_devices forcedevice_test_client vrpn_ping \
	add_vrpn_cookie vrpn_print_performance vrpn_print_messages

APPS := $(INSTALL_APPS) printvals printcereal checklogfile logfilesenders \
	logfiletypes text forwarderClient bdbox_client ff_client phan_client \
	sphere_client bdbox_client test_mutex test_imager c_interface_example

all:	$(APPS)

.PHONY:	vrpn_print_devices
vrpn_print_devices:	$(OBJ_DIR)/vrpn_print_devices

.PHONY:	vrpn_print_messages
vrpn_print_messages:	$(OBJ_DIR)/vrpn_print_messages

.PHONY:	vrpn_print_performance
vrpn_print_performance:	$(OBJ_DIR)/vrpn_print_performance

.PHONY:	forcedevice_test_client
forcedevice_test_client:	$(OBJ_DIR)/forcedevice_test_client

.PHONY:	printvals
printvals:	$(OBJ_DIR)/printvals

.PHONY:	printcereal
printcereal:	$(OBJ_DIR)/printcereal

.PHONY:	checklogfile
checklogfile:	$(OBJ_DIR)/checklogfile

.PHONY:	bdbox_client
bdbox_client:	$(OBJ_DIR)/bdbox_client

.PHONY:	ff_client
ff_client:	$(OBJ_DIR)/ff_client

.PHONY:	phan_client
phan_client:	$(OBJ_DIR)/phan_client

.PHONY:	sphere_client
sphere_client:	$(OBJ_DIR)/sphere_client

.PHONY:	forwarderClient
forwarderClient:	$(OBJ_DIR)/forwarderClient

.PHONY:	add_vrpn_cookie
add_vrpn_cookie:	$(OBJ_DIR)/add_vrpn_cookie

.PHONY: testSharedObject
testSharedObject:	$(OBJ_DIR)/testSharedObject

.PHONY: vrpn_ping
vrpn_ping:	$(OBJ_DIR)/vrpn_ping

.PHONY: testLamport
testLamport:	$(OBJ_DIR)/testLamport

.PHONY: test_mutex
test_mutex:	$(OBJ_DIR)/test_mutex

.PHONY: test_imager
test_imager:	$(OBJ_DIR)/test_imager

.PHONY: c_interface_example
c_interface_example:	$(OBJ_DIR)/c_interface_example

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

$(OBJ_DIR)/vrpn_print_devices: $(OBJ_DIR)/vrpn_print_devices.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_print_devices \
		$(OBJ_DIR)/vrpn_print_devices.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/vrpn_print_messages: $(OBJ_DIR)/vrpn_print_messages.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_print_messages \
		$(OBJ_DIR)/vrpn_print_messages.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/vrpn_print_performance: $(OBJ_DIR)/vrpn_print_performance.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_print_performance \
		$(OBJ_DIR)/vrpn_print_performance.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printcereal: $(OBJ_DIR)/printcereal.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printcereal \
		$(OBJ_DIR)/printcereal.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/bdbox_client: $(OBJ_DIR)/bdbox_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/bdbox_client \
		$(OBJ_DIR)/bdbox_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/ff_client: $(OBJ_DIR)/ff_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/ff_client \
		$(OBJ_DIR)/ff_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/forcedevice_test_client: $(OBJ_DIR)/forcedevice_test_client.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forcedevice_test_client \
		$(OBJ_DIR)/forcedevice_test_client.o -lvrpn $(ARCH_LIBS)

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

$(OBJ_DIR)/vrpn_ping: $(OBJ_DIR)/vrpn_ping.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_ping \
		$(OBJ_DIR)/vrpn_ping.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/testLamport: $(OBJ_DIR)/vrpn_LamportClock.t.o \
				$(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testLamport \
		$(OBJ_DIR)/vrpn_LamportClock.t.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/test_mutex: $(OBJ_DIR)/test_mutex.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_mutex \
		$(OBJ_DIR)/test_mutex.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/test_imager: $(OBJ_DIR)/test_imager.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_imager \
		$(OBJ_DIR)/test_imager.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/c_interface_example: $(OBJ_DIR)/c_interface_example.o $(OBJ_DIR)/c_interface.o $(LIB_DIR)/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/c_interface_example \
		$(OBJ_DIR)/c_interface_example.o $(OBJ_DIR)/c_interface.o -lvrpn $(ARCH_LIBS)


# These utilities actually DON'T use libvrpn

$(OBJ_DIR)/logfilesenders: $(OBJ_DIR)/logfilesenders.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfilesenders \
		$(OBJ_DIR)/logfilesenders.o

$(OBJ_DIR)/logfiletypes: $(OBJ_DIR)/logfiletypes.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfiletypes $(OBJ_DIR)/logfiletypes.o

install: all
	-mkdir -p $(BIN_DIR)
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )
	( cd $(OBJ_DIR) ; cp $(INSTALL_APPS) $(BIN_DIR) )
	( cd $(BIN_DIR) ; strip $(INSTALL_APPS) )

uninstall:
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )

clean:
	rm -f $(OBJ_DIR)/*

$(LIB_DIR)/libvrpn.a:


