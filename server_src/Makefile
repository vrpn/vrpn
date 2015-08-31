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
#HW_OS := macosx_32_64
#HW_OS := macosx_64
##########################

##########################
# Mac OS X-specific options. If HW_OS is powerpc_macosx or universal_macosx,
# uncomment one line below to choose the minimum targeted OS X version and
# corresponding SDK. If none of the lines below is commented out, 10.5 will
# be the minimum version.
##########################
#MAC_OS_MIN_VERSION := 10.4
#MAC_OS_MIN_VERSION := 10.5
#MAC_OS_MIN_VERSION := 10.6
#MAC_OS_MIN_VERSION := 10.7
#MAC_OS_MIN_VERSION := 10.8

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

  ifneq (,$(findstring macosx,$(HW_OS)))
    ifndef MAC_OS_MIN_VERSION
      MAC_OS_MIN_VERSION := 10.5
    endif

    # Select which compiler and MAC OS X SDK to use
    MAC_GCC := g++
    ifeq ($(MAC_OS_MIN_VERSION), 10.8)
      MAC_OS_SDK := MacOSX10.8.sdk
    else
    ifeq ($(MAC_OS_MIN_VERSION), 10.7)
      MAC_OS_SDK := MacOSX10.7.sdk
    else
   ifeq ($(MAC_OS_MIN_VERSION), 10.6)
      MAC_OS_SDK := MacOSX10.6.sdk
    else
      ifeq ($(MAC_OS_MIN_VERSION), 10.5)
        MAC_OS_SDK := MacOSX10.5.sdk
      else
        MAC_OS_SDK := MacOSX10.4u.sdk
        MAC_GCC := g++-4.0
      endif
    endif
  endif
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
#	CC := /usr/ibmcxx/bin/xlC_r -g -qarch=pwr3 -w
	CC := /usr/ibmcxx/bin/xlC_r $(DEBUG_FLAGS) -qarch=pwr3 -w
endif

ifeq ($(HW_OS), powerpc_macosx)
	CC := $(MAC_GCC) -arch ppc -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
	RANLIB := ranlib
	AR := libtool -static -o
	SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
endif

ifeq ($(HW_OS),universal_macosx)
        CC := $(MAC_GCC) -arch ppc -arch i386 -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
	RANLIB := :
	AR := libtool -static -o
	SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
endif

   ifeq ($(HW_OS), macosx_32_64)
        CC := $(MAC_GCC) -arch i386 -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
        RANLIB := :
        AR := libtool -static -o
	SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
  endif
  
    ifeq ($(HW_OS), macosx_64)
  		 ifeq ($(MAC_OS_SDK), MacOSX10.6.sdk)
    		CC := $(MAC_GCC) -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
       else
     		CC := $(MAC_GCC) -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
       endif
        RANLIB := :
        AR := libtool -static -o
		SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
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
# For WiiMote
#	SYSLIBS := -lwiiuse
# For GPM mouse
#	SYSLIBS := -lgpm
# For Isense libraries
#	SYSLIBS := -ldl
# For Freespace Devices
#	SYSLIBS := -lfreespace
# For HID devices
	SYSLIBS := -lusb-1.0
endif
ifeq ($(HW_OS),pc_linux64)
	SYSLIBS := 
# For WiiMote
#	SYSLIBS := -lwiiuse
# For GPM mouse
#	SYSLIBS := -lgpm
# For Isense libraries
#	SYSLIBS := -ldl
# For Freespace Devices
#	SYSLIBS := -lfreespace
	SYSLIBS := -lusb-1.0
endif

ifeq ($(HW_OS),sgi_irix)
   ifndef SGI_ABI
      SGI_ABI := n32
   endif
   ifndef SGI_ARCH
      SGI_ARCH := mips3
   endif
   OBJECT_DIR_SUFFIX := .$(SGI_ABI).$(SGI_ARCH)
   CC := CC -$(SGI_ABI) -$(SGI_ARCH) -LANG:std
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

# Jean SIMARD <jean.simard@limsi.fr>
# Definition of the 'TRUE' and 'FALSE' constant for Linux (by default, they are 
# defined in <windows.h>)
# XXX This would be better done in the .h/.C files with #ifndef
override CFLAGS := $(CFLAGS) -I./ -I../ -I../quat -I../../libfreespace/include $(DEBUG_FLAGS) $(CFLAGS) -DTRUE=true =DFALSE=false
override CXXFLAGS := $(CXXFLAGS) -I./ -I../ -I../quat -I../../libfreespace/include $(DEBUG_FLAGS) $(CXXFLAGS)
override LFLAGS := $(LFLAGS) -L$(LIB_DIR) -L../quat/$(HW_OS)$(OBJECT_DIR_SUFFIX) $(LDFLAGS)
VRPN_LIBS = -lvrpnserver -lvrpnatmel -lvrpngpsnmea -lpthread

# Uncomment the following section and point them at the
# currect diretories to compile with HDAPI under Cygwin.
# Well, to make it almost compile -- link fails to find things
# in the libraries that should be there.
#HDAPI_ROOT := "C:/Program Files/SensAble/3DTouch"

# Jean SIMARD <jean.simard@limsi.fr>
# This is only for the PHANToM server on Linux (not tested on other plateform)
# Put in comment if you don't want a PHANToM server
#override CFLAGS += -I$(HDAPI_ROOT)/include -I$(HDAPI_ROOT)/utilities/include
#override CXXFLAGS += -I$(HDAPI_ROOT)/include -I$(HDAPI_ROOT)/utilities/include
#override LFLAGS += -L$(HDAPI_ROOT)/lib
#SYSLIBS := $(SYSLIBS) -lHL -lHD -lHLU -lHDU

# This is for the PHANToM/GHOST server on SGI (not tested on other plateform)
# Put in comment if you don't want a PHANToM server.  GHOST expects SGI to
# be defined during compile, whereas sgi seems to be the standard definition.
#override CFLAGS += -I/usr/local/SensAble/GHOST/v31/include -DSGI
#override CXXFLAGS += -I/usr/local/SensAble/GHOST/v31/include -DSGI
#override LFLAGS += -L/usr/local/SensAble/GHOST/v31/lib
#SYSLIBS := $(SYSLIBS) -lghost31_32

ifdef INTERSENSE
  ifeq ($(HW_OS),powerpc_macosx)
#     CFLAGS = -g -I../ -I../../quat -I../../isense -DMACOSX
     override CFLAGS += -I../../isense -DMACOSX
     override CXXFLAGS += -I../../isense -DMACOSX
     override LFLAGS += ../../isense/MacOSX/dlcompat.o
  endif
  ifeq ($(HW_OS),universal_macosx)
#     CFLAGS = -g -I../ -I../../quat -I../../isense -DMACOSX
     override CFLAGS += -I../../isense -DMACOSX
     override CXXFLAGS += -I../../isense -DMACOSX
     override LFLAGS += ../../isense/MacOSX/dlcompat.o
	
  endif
endif

#uncomment the following lines to include the PHASESPACE library
#PHASESPACE := -L../../phasespace -lowlsock
#CXXFLAGS += -I../../phasespace
#CFLAGS += -I../../phasespace

ifdef PHASESPACE
#the rpath option may not work on all platforms, remove it if necessary.
EXTRAOPTS := -Wl,-rpath .
override LFLAGS += $(PHASESPACE) $(EXTRAOPTS)
endif

.SUFFIXES: .cpp

.C.o:
	$(CC) $(CXXFLAGS) -c $*.C

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.cpp.o:
	$(CC) $(CXXFLAGS) -c $*.cpp

$(OBJ_DIR)/%.o:	%.c
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o:	%.C
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o:	%.cpp
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

INSTALL_APPS := vrpn_server test_vrpn
APPS := $(INSTALL_APPS) client_and_server test_mutexServer test_peerMutex \
test_radamec_spi test_analogfly testimager_server test_auxiliary_logger \
test_logging testSharedObjectServer
# test_freespace
# 

all: $(APPS)

.PHONY:	vrpn_server
vrpn_server:	$(OBJ_DIR)/vrpn_server

.PHONY:	test_radamec_spi
test_radamec_spi:	$(OBJ_DIR)/test_radamec_spi

.PHONY:	testimager_server
testimager_server:	$(OBJ_DIR)/testimager_server

.PHONY:	test_logging
test_logging:	$(OBJ_DIR)/test_logging

.PHONY:	test_freespace
test_freespace:	$(OBJ_DIR)/test_freespace

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

.PHONY:	test_mutexServer
test_mutexServer:	$(OBJ_DIR)/test_mutexServer

.PHONY:	test_peerMutex
test_peerMutex:	$(OBJ_DIR)/test_peerMutex

.PHONY:	sample_analog
sample_analog:	$(OBJ_DIR)/sample_analog

.PHONY:	forward
forward:	$(OBJ_DIR)/forward

.PHONY:	textServer
textServer:	$(OBJ_DIR)/textServer

.PHONY:	last_of_sequence
last_of_sequence:	$(OBJ_DIR)/last_of_sequence

.PHONY:	testSharedObjectServer
testSharedObjectServer:	$(OBJ_DIR)/testSharedObjectServer

$(OBJ_DIR)/InstantBuzzEffect.o: ghostEffects/InstantBuzzEffect.cpp
	$(CC) $(CXXFLAGS) -o $@ -c $<

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

$(OBJ_DIR)/test_freespace: $(OBJ_DIR)/test_freespace.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_freespace \
		$(OBJ_DIR)/test_freespace.o \
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

$(OBJ_DIR)/textServer: $(OBJ_DIR)/textServer.o  \
			 $(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/textServer \
		$(OBJ_DIR)/textServer.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/sample_server:	$(OBJ_DIR)/sample_server.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_server \
		$(OBJ_DIR)/sample_server.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

$(OBJ_DIR)/test_mutexServer:	$(OBJ_DIR)/test_mutexServer.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_mutexServer \
		$(OBJ_DIR)/test_mutexServer.o \
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


$(OBJ_DIR)/testSharedObjectServer:	$(OBJ_DIR)/testSharedObjectServer.o \
			$(LIB_DIR)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/testSharedObjectServer \
		$(OBJ_DIR)/testSharedObjectServer.o \
		$(VRPN_LIBS) $(SYSLIBS) -lm

install: all
	-mkdir -p $(BIN_DIR)
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )
	( cd $(OBJ_DIR) ; cp $(INSTALL_APPS) $(BIN_DIR) )
	( cd $(BIN_DIR) ; strip $(INSTALL_APPS) )

uninstall:
	( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )

clean:
ifeq ($(HW_OS),)
		echo "Must specify HW_OS !"
else
	rm -f $(OBJ_DIR)/*
endif

$(OBJ_DIR)/vrpn.o:	vrpn.C
$(OBJ_DIR)/vrpn_Generic_server_object.o:	vrpn_Generic_server_object.C
$(OBJ_DIR)/forward.o:	forward.C
$(OBJ_DIR)/textServer.o:	textServer.C
$(OBJ_DIR)/sample_server.o:	sample_server.C
$(OBJ_DIR)/test_mutexServer.o:	test_mutexServer.C
$(OBJ_DIR)/test_peerMutex.o:	test_peerMutex.C
$(OBJ_DIR)/sample_analog.o:	sample_analog.C

