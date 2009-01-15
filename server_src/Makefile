# this is a gmake file

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
##########################

##########################
# Directories for installation

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
    # On cygwin make is gmake (and gmake doesn't exist)
    MAKE  := make -f $(MAKEFILE)
  else
    ifeq ($(UNAME), CYGWIN_98-4.10)
		HW_OS := pc_cygwin
		MAKE := make -f $(MAKEFILE)
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
ifeq ($(HW_OS),pc_cygwin)
	SYSLIBS := -lcygwin -luser32 -lgdi32 -lcomdlg32 -lwsock32 -lSetupApi
endif
ifeq ($(HW_OS),sparc_solaris)
	CC := /opt/SUNWspro/bin/CC
	SYSLIBS := -lsocket -lnsl
endif
ifeq ($(HW_OS),sparc_solaris_64)
	CC := /opt/SUNWspro/bin/CC -xarch=v9a
	SYSLIBS := -lsocket -lnsl
endif
ifeq ($(HW_OS),powerpc_aix)
	CC := /usr/ibmcxx/bin/xlC_r -g -qarch=pwr3 -w
endif

ifeq ($(HW_OS),universal_macosx)
	CC := g++ -arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
	RANLIB := :
	AR := libtool -static -o
endif

ifeq ($(HW_OS),pc_linux_ia64)
	CC := g++
	SYSLIBS := 
endif
ifeq ($(HW_OS),pc_linux64)
	CC := g++ -m64
	SYSLIBS := 
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
ifeq ($(HW_OS),pc_linux)
	SYSLIBS := 
# For GPM mouse
#	SYSLIBS := -lgpm
# For Isense libraries
#	SYSLIBS := -ldl
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
   GL := -lgl
endif

ifeq ($(HW_OS),hp700_hpux10)
	CC := CC +a1
endif
ifeq ($(HW_OS),sparc_sunos)
	CC := /usr/local/lib/CenterLine/bin/CC
endif

ifeq ($(HW_OS), hp_flow_aCC)
	CC := /opt/aCC/bin/aCC
endif

OBJ_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
LIB_DIR := ../$(OBJ_DIR)

CFLAGS = -g -I./ -I../ -I../../quat
LFLAGS = -L$(LIB_DIR) -L../../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX)
VRPN_LIBS = -lvrpnserver -lvrpnatmel -lpthread

# Uncomment the following section and point them at the
# currect diretories to compile with HDAPI under Cygwin.
# Well, to make it almost compile -- link fails to find things
# in the libraries that should be there.
#HDAPI_ROOT := "C:/Program Files/SensAble/3DTouch"
#CFLAGS = -g -I./ -I../ -I../../quat -I$(HDAPI_ROOT)/include -I$(HDAPI_ROOT)/utilities/include
#LFLAGS = -L$(LIB_DIR) -L../../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) -L$(HDAPI_ROOT)/lib
#SYSLIBS := $(SYSLIBS) -lhl -lhd

ifdef INTERSENSE
  ifeq ($(HW_OS),powerpc_macosx)
     CFLAGS = -g -I../ -I../../quat -I../../isense -DMACOSX
     LFLAGS = -L$(LIB_DIR) -L../../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) ../../isense/MacOSX/dlcompat.o
  endif
  ifeq ($(HW_OS),universal_macosx)
     CFLAGS = -g -I../ -I../../quat -I../../isense -DMACOSX
     LFLAGS = -L$(LIB_DIR) -L../../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) ../../isense/MacOSX/dlcompat.o
  endif
endif

#uncomment the following line to include the PHASESPACE library
#PHASESPACE := -L../../phasespace -lowlsock

ifdef PHASESPACE
#the rpath option may not work on all platforms, remove it if necessary.
EXTRAOPTS := -Wl,-rpath .
LFLAGS := $(LFLAGS) $(PHASESPACE) $(EXTRAOPTS)
endif

.SUFFIXES: .cpp

.C.o:
	$(CC) $(CFLAGS) -c $*.C

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.cpp.o:
	$(CC) $(CFLAGS) -c $*.cpp

$(OBJ_DIR)/%.o:	%.c
	@[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o:	%.C
	@[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o:	%.cpp
	@[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

INSTALL_APPS := vrpn_server test_vrpn
APPS := $(INSTALL_APPS) client_and_server test_mutex test_peerMutex \
test_radamec_spi test_analogfly testimager_server test_auxiliary_logger \
test_logging
# testSharedObject

all: $(APPS)

.PHONY:	vrpn_server
vrpn_server:	$(OBJ_DIR)/vrpn_server

.PHONY:	test_radamec_spi
test_radamec_spi:	$(OBJ_DIR)/test_radamec_spi

.PHONY:	testimager_server
testimager_server:	$(OBJ_DIR)/testimager_server

.PHONY:	test_logging
test_logging:	$(OBJ_DIR)/test_logging

.PHONY:	test_auxiliary_logger
test_auxiliary_logger:	$(OBJ_DIR)/test_auxiliary_logger

.PHONY:	test_analogfly
test_analogfly:	$(OBJ_DIR)/test_analogfly

.PHONY:	test_vrpn
test_vrpn:	$(OBJ_DIR)/test_vrpn

.PHONY:	client_and_server
client_and_server:	$(OBJ_DIR)/client_and_server

.PHONY:	sample_server
sample_server:	$(OBJ_DIR)/sample_server

.PHONY:	test_mutex
test_mutex:	$(OBJ_DIR)/test_mutex

.PHONY:	test_peerMutex
test_peerMutex:	$(OBJ_DIR)/test_peerMutex

.PHONY:	sample_analog
sample_analog:	$(OBJ_DIR)/sample_analog

.PHONY:	forward
forward:	$(OBJ_DIR)/forward

.PHONY:	text
text:	$(OBJ_DIR)/text

.PHONY:	last_of_sequence
last_of_sequence:	$(OBJ_DIR)/last_of_sequence

.PHONY:	testSharedObject
testSharedObject:	$(OBJ_DIR)/testSharedObject

$(OBJ_DIR)/InstantBuzzEffect.o: ghostEffects/InstantBuzzEffect.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/vrpn_server: $(OBJ_DIR)/vrpn.o $(LIB_DIR)/libvrpnserver.a \
		$(OBJ_DIR)/buzzForceField.o $(OBJ_DIR)/constraint.o \
		$(OBJ_DIR)/forcefield.o $(OBJ_DIR)/plane.o \
		$(OBJ_DIR)/texture_plane.o $(OBJ_DIR)/trimesh.o \
		$(OBJ_DIR)/vrpn_Phantom.o \
		$(OBJ_DIR)/InstantBuzzEffect.o \
		$(OBJ_DIR)/vrpn_Generic_server_object.o 
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_server $(OBJ_DIR)/vrpn.o \
		$(OBJ_DIR)/buzzForceField.o $(OBJ_DIR)/constraint.o \
		$(OBJ_DIR)/forcefield.o $(OBJ_DIR)/plane.o \
		$(OBJ_DIR)/texture_plane.o $(OBJ_DIR)/trimesh.o \
		$(OBJ_DIR)/vrpn_Phantom.o \
		$(OBJ_DIR)/InstantBuzzEffect.o \
		$(OBJ_DIR)/vrpn_Generic_server_object.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_vrpn: $(OBJ_DIR)/test_vrpn.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_vrpn \
		$(OBJ_DIR)/test_vrpn.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_radamec_spi: $(OBJ_DIR)/test_radamec_spi.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_radamec_spi \
		$(OBJ_DIR)/test_radamec_spi.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/testimager_server: $(OBJ_DIR)/testimager_server.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testimager_server \
		$(OBJ_DIR)/testimager_server.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_auxiliary_logger: $(OBJ_DIR)/test_auxiliary_logger.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_auxiliary_logger \
		$(OBJ_DIR)/test_auxiliary_logger.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_logging: $(OBJ_DIR)/test_logging.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_logging \
		$(OBJ_DIR)/test_logging.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_analogfly: $(OBJ_DIR)/test_analogfly.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_analogfly \
		$(OBJ_DIR)/test_analogfly.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/client_and_server: $(OBJ_DIR)/client_and_server.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/client_and_server \
		$(OBJ_DIR)/client_and_server.o \
		$(VRPN_LIBS) $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/forward: $(OBJ_DIR)/forward.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forward \
		$(OBJ_DIR)/forward.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/text: $(OBJ_DIR)/text.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/text \
		$(OBJ_DIR)/text.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/sample_server:	$(OBJ_DIR)/sample_server.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_server \
		$(OBJ_DIR)/sample_server.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/test_mutex:	$(OBJ_DIR)/test_mutex.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_mutex \
		$(OBJ_DIR)/test_mutex.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/test_peerMutex:	$(OBJ_DIR)/test_peerMutex.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_peerMutex \
		$(OBJ_DIR)/test_peerMutex.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/sample_analog:	$(OBJ_DIR)/sample_analog.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_analog \
		$(OBJ_DIR)/sample_analog.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/last_of_sequence:	$(OBJ_DIR)/last_of_sequence.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/last_of_sequence \
		$(OBJ_DIR)/last_of_sequence.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm


$(OBJ_DIR)/testSharedObject:	$(OBJ_DIR)/testSharedObject.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testSharedObject \
		$(OBJ_DIR)/testSharedObject.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

install: all
	-mkdir $(BIN_DIR)
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )
	( cd $(OBJ_DIR) ; cp $(INSTALL_APPS) $(BIN_DIR) )
	( cd $(BIN_DIR) ; strip $(INSTALL_APPS) )

uninstall:
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )

clean:
	rm -f $(OBJ_DIR)/*

$(OBJ_DIR)/vrpn.o:	vrpn.C
$(OBJ_DIR)/vrpn_Generic_server_object.o:	vrpn_Generic_server_object.C
$(OBJ_DIR)/forward.o:	forward.C
$(OBJ_DIR)/text.o:	text.C
$(OBJ_DIR)/sample_server.o:	sample_server.C
$(OBJ_DIR)/test_mutex.o:	test_mutex.C
$(OBJ_DIR)/test_peerMutex.o:	test_peerMutex.C
$(OBJ_DIR)/sample_analog.o:	sample_analog.C

