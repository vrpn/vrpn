#############################################################################
#	Makefile for VRPN libraries.  Needs to be built using 'gmake'.
# Should run on any architecture that is currently supported.  It should
# be possible to build simultaneously on multiple architectures.
#
# On the sgi, both g++ and CC versions are built by default.
#
# Author: Russ Taylor, 10/2/1997
#
# modified:
# * Andrew Roth June 16/2012
#	 * Updates for MacOSX builds for 32/64, 64 bit and Xcode 4.3
# * Jeff Juliano, 10/99
#    * added "make depend"  (see comments at end of this makefile)
#    * changed to use RM, RMF, MV, and MVF
#      these are overridable as follows:
#          gmake RM=/mybin/rm,  or  gmake RMF="/bin/rm -option"
# * Jeff Juliano, 9/1999
#     support for parallel make (see WARNING below)
# * Tom Hudson, 25 Jun 1998
#     Support for n32 ABI on sgi.  (gmake n32)
# * Hans Weber, ???
#     Support for both g++ and native compilers on sgi.
# * Tom Hudson, 13 Feb 1998
#     Build two different libraries:  client (libvrpn) and server
#     (libvrpnserver).  Our solution is to compile twice, once with the
#     flag -DVRPN_CLIENT_ONLY and once without.  Any server-specific code
#     (vrpn_3Space, vrpn_Tracker_Fastrak, vrpn_Flock) should ONLY be
#     compiled into the server library!
#############################################################################

##########################
# common definitions. For non-UNC sites, uncomment one of the lines
# that defines hw_os for the machine you are on in the section just
# below. Then, the code should compile in your environment.
#
#HW_OS := sgi_irix
#HW_OS := pc_linux
#HW_OS := pc_linux64
#HW_OS := pc_linux_ia64
# Try this to cross-compile on a Linux PC for an ARM embedded controller.
#HW_OS := pc_linux_arm
# Try this to cross-compile on a Cygwin PC for an ARM embedded controller.
#HW_OS := pc_cygwin_arm
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
#HW_OS := sparc_solaris
#HW_OS := sparc_solaris_64
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
# be the minimum version.  10.7/4.3 removes ppc support.  In Xcode 4.3 - you 
# must now manually install command line tools from: 
# Xcode menu > Preferences > Downloads.
##########################
#MAC_OS_MIN_VERSION := 10.4
#MAC_OS_MIN_VERSION := 10.5
#MAC_OS_MIN_VERSION := 10.6
#MAC_OS_MIN_VERSION := 10.7
#MAC_OS_MIN_VERSION := 10.8

INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
INCLUDE_DIR := $(INSTALL_DIR)/include
LIB_DIR := $(INSTALL_DIR)/lib

MV = /bin/mv
MVF = $(MV) -f

RM = /bin/rm
RMF = $(RM) -f

ifndef HW_OS
  # hw_os does not exist on FreeBSD at UNC or on CYGWIN
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
        ifeq ($(UNAME), CYGWIN_NT-5.0)
	    HW_OS := pc_cygwin
	    MAKE := make -f $(MAKEFILE)
        else
	    HW_OS := $(shell hw_os)
        endif
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

ifeq (,$(strip $(HW_OS)))
$(error You must define HW_OS when calling make. See the makefile for example values)
all:
	@echo "ERROR: You must define HW_OS when calling make. See the makefile for example values." 1>&2
endif

# Which C++ compiler to use.  Default is g++, but some don't use this.
#
# IF YOU CHANGE THESE, document either here or in the header comment
# why.  Multiple contradictory changes have been made recently.

# These should be defaulting to useful values on the platform.
# If they aren't, use gcc.

# C compiler
CC ?= gcc

# C++ compiler
CXX ?= g++

# These probably aren't defaulting to anything useful.
AR := ar ruv
# need default 'ranlib' to be touch for platforms that don't use it,
# otherwise make fails.
RANLIB ?= touch

ifeq ($(FORCE_GPP),1)
  CC := g++
else

  ifeq ($(HW_OS),sparc_solaris)
    CC := /opt/SUNWspro/bin/CC
    AR := /opt/SUNWspro/bin/CC -xar -o
  endif

  ifeq ($(HW_OS),sparc_solaris_64)
    CC := /opt/SUNWspro/bin/CC -xarch=v9a
    AR := /opt/SUNWspro/bin/CC -xarch=v9a -xar -o
  endif

  ifeq ($(HW_OS),powerpc_aix)
    CC := /usr/ibmcxx/bin/xlC_r -g -qarch=pwr3 -w
    RANLIB := ranlib
  endif

  ifeq ($(HW_OS),pc_linux64)
    STANDARD_CFLAGS := -m64 -fPIC
    RANLIB := ranlib
  endif

  ifeq ($(HW_OS),pc_linux)
    STANDARD_CFLAGS := -fPIC
    RANLIB := ranlib
  endif

  ifeq ($(HW_OS), pc_linux_ia64)
    RANLIB := ranlib
  endif

  ifneq (,$(findstring macosx,$(HW_OS)))
    ifndef MAC_OS_MIN_VERSION
      MAC_OS_MIN_VERSION := 10.5
    endif

    # Select which compiler and MAC OS X SDK to use
    MAC_CC := gcc
    MAC_CXX := g++
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
            MAC_CC := gcc-4.0
            MAC_CXX := g++-4.0
          endif
        endif
       endif
      endif
    endif
 
ifneq (,$(findstring macosx,$(HW_OS)))
  ifeq ($(wildcard /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK)),/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK))
    PATH_TO_DEV := /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK)
    $(info ---> Xcode 4.3+: Platform SDK found.  Setting dev path to $(PATH_TO_DEV)) 
  else
    PATH_TO_DEV := /Developer/SDKs/$(MAC_OS_SDK)
    $(info Xcode 4.3+: Platform SDK not found in /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK)!  Attempting to locate SDK in $(PATH_TO_DEV))
  endif
  CC := $(MAC_CC)
  CXX := $(MAC_CXX)
  STANDARD_CFLAGS := -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
  SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
endif

  ifeq ($(HW_OS),powerpc_macosx)
    STANDARD_CFLAGS := -arch ppc $(STANDARD_CFLAGS)
    RANLIB := ranlib
    AR := libtool -static -o
  endif

  ifeq ($(HW_OS),universal_macosx)
    STANDARD_CFLAGS := -arch ppc -arch i386 -arch x86_64 $(STANDARD_CFLAGS)
    RANLIB := :
    AR := libtool -static -o
  endif
  
  ifeq ($(HW_OS),macosx_32_64)
    STANDARD_CFLAGS := -arch i386 -arch x86_64 $(STANDARD_CFLAGS)
    RANLIB := :
    AR := libtool -static -o
  endif
  
  ifeq ($(HW_OS),macosx_64)
    STANDARD_CFLAGS := -arch x86_64 $(STANDARD_CFLAGS)
    RANLIB := :
    AR := libtool -static -o
  endif
       
  ifeq ($(HW_OS),pc_linux_arm)
    CC := arm-linux-gcc
    CXX := arm-linux-g++
    RANLIB := arm-linux-ranlib
    AR := arm-linux-ar ruv
  endif

  ifeq ($(HW_OS),pc_cygwin_arm)
    CC := arm-unknown-linux-gnu-gcc
    CXX := arm-unknown-linux-gnu-g++
    RANLIB := arm-unknown-linux-gnu-ranlib
    AR := arm-unknown-linux-gnu-ar ruv
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
   RANLIB := :
  endif

  ifeq ($(HW_OS),hp700_hpux10)
	CC := CC +a1
  endif
  ifeq ($(HW_OS),pc_cygwin)
	CC := g++
  endif
  ifeq ($(HW_OS),sparc_sunos)
	CC := /usr/local/lib/CenterLine/bin/CC
  endif
  ifeq ($(HW_OS), hp_flow_aCC)
	CC := /opt/aCC/bin/aCC
  endif
endif

#ifeq ($(HW_OS),sparc_solaris)
#  AR := /usr/ccs/bin/ar
#endif

##########################
# directories
#

# subdirectory for make
ifeq ($(FORCE_GPP),1)
OBJECT_DIR	 := $(HW_OS)$(OBJECT_DIR_SUFFIX)/g++
SOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/g++/server
AOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/atmellib
GOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/gpsnmealib
else
UNQUAL_OBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
UNQUAL_SOBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)/server
UNQUAL_AOBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)/atmellib
UNQUAL_GOBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)/gpsnmealib
OBJECT_DIR	 := $(HW_OS)$(OBJECT_DIR_SUFFIX)
SOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/server
AOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/atmellib
GOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/gpsnmealib
endif

# directories that we can do an rm -f on because they only contain
# object files and executables
SAFE_KNOWN_ARCHITECTURES :=	\
	hp700_hpux/* \
	hp700_hpux10/* \
	mips_ultrix/* \
	pc_linux/* \
	sgi_irix.32/* \
	sgi_irix.n32/* \
	sparc_solaris/* \
	sparc_solaris_64/* \
	sparc_sunos/* \
	pc_cygwin/* \
	powerpc_aix/* \
	pc_linux_arm/* \
	powerpc_macosx/* \
	universal_macosx/* \
	macosx_32_64/* \
	macosx_64/* \
	pc_linux64/* \
	pc_linux_ia64/*

CLIENT_SKA = $(patsubst %,client_src/%,$(SAFE_KNOWN_ARCHITECTURES))
SERVER_SKA = $(patsubst %,server_src/%,$(SAFE_KNOWN_ARCHITECTURES))

##########################
# Include flags
#

#SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include
SYS_INCLUDE :=

ifeq ($(HW_OS),powerpc_macosx)
#   SYS_INCLUDE := -I/usr/include
    SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),universal_macosx)
#   SYS_INCLUDE := -I/usr/include
    SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),macosx_32_64)
#   SYS_INCLUDE := -I/usr/include
    SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),macosx_64)
#   SYS_INCLUDE := -I/usr/include
    SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),pc_linux)
    # The following is for the InterSense and Freespace libraries.
    SYS_INCLUDE := -DUNIX -DLINUX -I../libfreespace/include -I./submodules/hidapi/hidapi -I/usr/include/libusb-1.0
	# On linux, build this as c, not c++.
    $(HW_OS)/server/vrpn_Local_HIDAPI.o: EXTRA_FLAGS += -x c
endif

ifeq ($(HW_OS),pc_linux64)
    # The following is for the InterSense and Freespace libraries.
    SYS_INCLUDE := -DUNIX -DLINUX -I../libfreespace/include -I./submodules/hidapi/hidapi -I/usr/include/libusb-1.0
	# On linux, build this as c, not c++.
    $(HW_OS)/server/vrpn_Local_HIDAPI.o: EXTRA_FLAGS += -x c
endif

ifeq ($(HW_OS),pc_linux_arm)
    SYS_INCLUDE := -I/opt/Embedix/arm-linux/include
#   -I/usr/local/contrib/include \
#	  	 -I/usr/local/contrib/mod/include -I/usr/include/bsd \
#		 -I/usr/include/g++
endif

ifeq ($(HW_OS),pc_cygwin_arm)
   SYS_INCLUDE := -I/opt/Embedix/arm-linux/include
#   -I/usr/local/contrib/include \
#	  	 -I/usr/local/contrib/mod/include -I/usr/include/bsd \
#		 -I/usr/include/g++
endif

ifeq ($(HW_OS),sgi_irix)
#  SYS_INCLUDE := -I/usr/local/contrib/mod/include
  SYS_INCLUDE :=
endif

ifeq ($(HW_OS),hp700_hpux10)
  SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include \
                 -I/usr/include/bsd
endif

ifeq ($(HW_OS),hp_flow)
  SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include \
                 -I/usr/include/bsd -DFLOW
endif

ifeq ($(HW_OS),hp_flow_aCC)
  SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include \
                 -I/usr/include/bsd -DFLOW
endif

# On the PC, place quatlib in the directory ./quat.  No actual system
# includes should be needed.
ifeq ($(HW_OS),pc_cygwin)
    SYS_INCLUDE := -I./submodules/hidapi/hidapi -I/usr/include/libusb-1.0
    INCLUDE_FLAGS := -I. -I./quat -I./atmellib -I./gpsnmealib ${SYS_INCLUDE}
else
    INCLUDE_FLAGS := -I. $(SYS_INCLUDE) -I./quat -I../quat -I./atmellib -I./gpsnmealib
endif


##########################
# Load flags
#

#LOAD_FLAGS := -L./$(HW_OS)$(OBJECT_DIR_SUFFIX) -L/usr/local/lib \
#		-L/usr/local/contrib/unmod/lib -L/usr/local/contrib/mod/lib -g
LOAD_FLAGS := -L./$(HW_OS)$(OBJECT_DIR_SUFFIX) -L/usr/local/lib \
              -L/usr/local/contrib/unmod/lib -L/usr/local/contrib/mod/lib $(DEBUG_FLAGS) $(LDFLAGS)

ifeq ($(HW_OS),sgi_irix)
    LOAD_FLAGS := $(LOAD_FLAGS) -old_ld -LANG:std
endif

ifeq ($(HW_OS),pc_linux)
    LOAD_FLAGS := $(LOAD_FLAGS) -L/usr/X11R6/lib
endif

ifeq ($(HW_OS),pc_linux_ia64)
    LOAD_FLAGS := $(LOAD_FLAGS) -L/usr/X11R6/lib
endif

ifeq ($(HW_OS),pc_linux64)
    LOAD_FLAGS := $(LOAD_FLAGS) -L/usr/X11R6/lib
endif


##########################
# Libraries
#

ifeq ($(HW_OS),pc_linux64)
      ARCH_LIBS := -lbsd -ldl
else
  ifeq ($(HW_OS),pc_linux)
      ARCH_LIBS := -lbsd -ldl
  else
    ifeq ($(HW_OS),pc_linux_ia64)
      ARCH_LIBS := -lbsd -ldl
    else
      ifeq ($(HW_OS),sparc_solaris)
        ARCH_LIBS := -lsocket -lnsl
      else
        ifeq ($(HW_OS),sparc_solaris_64)
          ARCH_LIBS := -lsocket -lnsl
        else
          ARCH_LIBS :=
        endif
      endif
    endif
  endif
endif

LIBS := -lquat -lsdi $(TCL_LIBS) -lXext -lX11 $(ARCH_LIBS) -lm

#
# Defines for the compilation, CFLAGS
#

#CFLAGS		 := $(INCLUDE_FLAGS) -g
ALL_CFLAGS   = $(STANDARD_CFLAGS) $(INCLUDE_FLAGS) $(DEBUG_FLAGS) $(CFLAGS) $(EXTRA_FLAGS)
ALL_CXXFLAGS = $(STANDARD_CFLAGS) $(INCLUDE_FLAGS) $(DEBUG_FLAGS) $(CXXFLAGS) $(EXTRA_FLAGS)

# If we're building for sgi_irix, we need both g++ and non-g++ versions,
# unless we're building for one of the weird ABIs, which are only supported
# by the native compiler.

all:	client server atmellib gpsnmealib
ifeq ($(HW_OS),sgi_irix)
  ifeq ($(SGI_ABI),32)
# Also build the G++ versions
all:	client_g++ server_g++
  endif
endif

.PHONY:	client_g++
client_g++:
	$(MAKE) FORCE_GPP=1 $(UNQUAL_OBJECT_DIR)/g++/libvrpn.a
	$(MV) $(UNQUAL_OBJECT_DIR)/g++/libvrpn.a $(UNQUAL_OBJECT_DIR)/libvrpn_g++.a

.PHONY:	server_g++
server_g++:
	$(MAKE) FORCE_GPP=1 $(UNQUAL_OBJECT_DIR)/g++/libvrpnserver.a
	$(MV) $(UNQUAL_OBJECT_DIR)/g++/libvrpnserver.a $(UNQUAL_OBJECT_DIR)/libvrpnserver_g++.a

.PHONY:	client
client: $(OBJECT_DIR) $(OBJECT_DIR)/libvrpn.a

.PHONY:	server
server: $(SOBJECT_DIR) $(OBJECT_DIR)/libvrpnserver.a

.PHONY: atmellib
atmellib: $(AOBJECT_DIR) $(OBJECT_DIR)/libvrpnatmel.a

.PHONY: gpsnmealib
gpsnmealib: $(GOBJECT_DIR) $(OBJECT_DIR)/libvrpngpsnmea.a

$(OBJECT_DIR):
	-mkdir -p $@

$(SOBJECT_DIR):
	-mkdir -p $@

$(AOBJECT_DIR):
	-mkdir -p $@

$(GOBJECT_DIR):
	-mkdir -p $@

#############################################################################
#
# implicit rule for all .c files
#
.SUFFIXES:	.c .C .o .a

.c.o:
	$(CC) $(ALL_CFLAGS) -c $< -o $@
.C.o:
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

# Build client-only objects from .c files
$(OBJECT_DIR)/%.o: %.c $(LIB_INCLUDES) $(MAKEFILE) $(OBJECT_DIR)
	$(CC) $(ALL_CFLAGS) -DVRPN_CLIENT_ONLY -o $@ -c $<

# Build client-only objects from .C files
$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE) $(OBJECT_DIR)
	$(CC) $(ALL_CXXFLAGS) -DVRPN_CLIENT_ONLY -o $@ -c $<

# # Build objects from .C files
$(SOBJECT_DIR)/%.o: %.C $(SLIB_INCLUDES) $(MAKEFILE) $(SOBJECT_DIR)
	$(CXX) $(ALL_CFLAGS) -o $@ -c $<

# # Build objects from .C files
$(AOBJECT_DIR)/%.o: %.C $(ALIB_INCLUDES) $(MAKEFILE) $(AOBJECT_DIR)
	$(CXX) $(ALL_CXXFLAGS) -o $@ -c $<

#
#
#############################################################################

#############################################################################
#
# library code
#
#############################################################################

# files to be compiled into the client library

# Please keep sorted (case-sensitive)!
LIB_FILES =  \
	vrpn_Analog.C \
	vrpn_Analog_Output.C \
	vrpn_Auxiliary_Logger.C \
	vrpn_BaseClass.C \
	vrpn_Button.C \
	vrpn_Connection.C \
	vrpn_Dial.C \
	vrpn_EndpointContainer.C \
	vrpn_FileConnection.C \
	vrpn_FileController.C \
	vrpn_ForceDevice.C \
	vrpn_Forwarder.C \
	vrpn_ForwarderController.C \
	vrpn_Imager.C \
	vrpn_LamportClock.C \
	vrpn_Mutex.C \
	vrpn_Poser.C \
	vrpn_RedundantTransmission.C \
	vrpn_Serial.C \
	vrpn_Shared.C \
	vrpn_SharedObject.C \
	vrpn_Sound.C \
	vrpn_Text.C \
	vrpn_Thread.C \
	vrpn_Tracker.C

LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.C=.o))

# Please keep sorted (case-sensitive)!
LIB_INCLUDES = \
	vrpn_Analog.h \
	vrpn_Analog_Output.h \
	vrpn_Auxiliary_Logger.h \
	vrpn_BaseClass.h \
	vrpn_BufferUtils.h \
	vrpn_Button.h \
	vrpn_Connection.h \
	vrpn_Dial.h \
	vrpn_EndpointContainer.h \
	vrpn_FileConnection.h \
	vrpn_FileController.h \
	vrpn_ForceDevice.h \
	vrpn_Forwarder.h \
	vrpn_ForwarderController.h \
	vrpn_Imager.h \
	vrpn_LamportClock.h \
	vrpn_MainloopContainer.h \
	vrpn_MainloopObject.h \
	vrpn_Mutex.h \
	vrpn_Poser.h \
	vrpn_SendTextMessageStreamProxy.h \
	vrpn_Serial.h \
	vrpn_Shared.h \
	vrpn_SharedObject.h \
	vrpn_Sound.h \
	vrpn_Text.h \
	vrpn_Thread.h \
	vrpn_Tracker.h \
	vrpn_WindowsH.h \


# $(LIB_OBJECTS):
$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(LIB_OBJECTS)
	$(AR) $(OBJECT_DIR)/libvrpn.a $(LIB_OBJECTS)
	-$(RANLIB) $(OBJECT_DIR)/libvrpn.a

# We aren't going to use architecture-dependent sets of files.
# If vrpn_sgibox isn't supposed to be compiled on any other architecture,
# then put all of it inside "#ifdef sgi"!

# Please keep sorted (case-sensitive)!
SLIB_FILES =  $(LIB_FILES) \
	vrpn_3DConnexion.C \
	vrpn_3DMicroscribe.C \
	vrpn_3Space.C \
	vrpn_5DT16.C \
	vrpn_ADBox.C \
	vrpn_Analog_5dt.C \
	vrpn_Analog_5dtUSB.C \
	vrpn_Analog_Radamec_SPI.C \
	vrpn_Analog_USDigital_A2.C \
	vrpn_Atmel.C \
	vrpn_BiosciencesTools.C \
	vrpn_Button_NI_DIO24.C \
	vrpn_CHProducts_Controller_Raw.C \
	vrpn_CerealBox.C \
	vrpn_Contour.C \
	vrpn_DreamCheeky.C \
	vrpn_Dyna.C \
	vrpn_Event.C \
	vrpn_Event_Analog.C \
	vrpn_Event_Mouse.C \
	vrpn_Flock.C \
	vrpn_Flock_Parallel.C \
	vrpn_ForceDeviceServer.C \
	vrpn_Freespace.C \
	vrpn_FunctionGenerator.C \
	vrpn_Futaba.C \
	vrpn_GlobalHapticsOrb.C \
	vrpn_Griffin.C \
	vrpn_HumanInterface.C \
	vrpn_IDEA.C \
	vrpn_Imager_Stream_Buffer.C \
	vrpn_ImmersionBox.C \
	vrpn_JoyFly.C \
	vrpn_Joylin.C \
	vrpn_Keyboard.C \
	vrpn_Laputa.C \
	vrpn_LUDL.C \
	vrpn_Local_HIDAPI.C \
	vrpn_Logitech_Controller_Raw.C \
	vrpn_Magellan.C \
	vrpn_Microsoft_Controller_Raw.C \
	vrpn_Mouse.C \
	vrpn_NationalInstruments.C \
	vrpn_nVidia_shield_controller.C \
	vrpn_Oculus.C \
	vrpn_OmegaTemperature.C \
	vrpn_Poser_Analog.C \
	vrpn_Poser_Tek4662.C \
	vrpn_Retrolink.C \
	vrpn_Saitek_Controller_Raw.C \
	vrpn_Spaceball.C \
	vrpn_Streaming_Arduino.C \
	vrpn_Tng3.C \
	vrpn_Tracker_3DMouse.C \
	vrpn_Tracker_AnalogFly.C \
	vrpn_Tracker_ButtonFly.C \
	vrpn_Tracker_Crossbow.C \
	vrpn_Tracker_DTrack.C \
	vrpn_Tracker_Fastrak.C \
	vrpn_Tracker_Filter.C \
	vrpn_Tracker_GPS.C \
	vrpn_Tracker_GameTrak.C \
	vrpn_Tracker_IMU.C \
	vrpn_Tracker_Isotrak.C \
	vrpn_Tracker_Liberty.C \
	vrpn_Tracker_LibertyHS.C \
	vrpn_Tracker_MotionNode.C \
	vrpn_Tracker_NDI_Polaris.C \
	vrpn_Tracker_NovintFalcon.C \
	vrpn_Tracker_OSVRHackerDevKit.C \
	vrpn_Tracker_PDI.C \
	vrpn_Tracker_PhaseSpace.C \
	vrpn_Tracker_RazerHydra.C \
	vrpn_Tracker_SpacePoint.C \
	vrpn_Tracker_TrivisioColibri.C \
	vrpn_Tracker_ViewPoint.C \
	vrpn_Tracker_WiimoteHead.C \
	vrpn_Tracker_Wintracker.C \
	vrpn_Tracker_isense.C \
	vrpn_UNC_Joystick.C \
  vrpn_Vality.C \
	vrpn_VPJoystick.C \
	vrpn_Wanda.C \
	vrpn_WiiMote.C \
	vrpn_Xkeys.C \
	vrpn_YEI_3Space.C \
	vrpn_Zaber.C \
	vrpn_inertiamouse.C \
	vrpn_nikon_controls.C \
	vrpn_raw_sgibox.C \
	vrpn_sgibox.C \


SLIB_OBJECTS = $(patsubst %,$(SOBJECT_DIR)/%,$(SLIB_FILES:.C=.o))

# Please keep sorted (case-sensitive)!
SLIB_INCLUDES = $(LIB_INCLUDES) \
	vrpn_3DConnexion.h \
	vrpn_3DMicroscribe.h \
	vrpn_3Space.h \
	vrpn_5DT16.h \
	vrpn_ADBox.h \
	vrpn_Analog_5dt.h \
	vrpn_Analog_5dtUSB.h \
	vrpn_Analog_Radamec_SPI.h \
	vrpn_Analog_USDigital_A2.h \
	vrpn_Atmel.h \
	vrpn_BiosciencesTools.h \
	vrpn_Button_NI_DIO24.h \
	vrpn_CHProducts_Controller_Raw.h \
	vrpn_CerealBox.h \
	vrpn_Contour.h \
	vrpn_DreamCheeky.h \
	vrpn_Dyna.h \
	vrpn_Event.h \
	vrpn_Event_Analog.h \
	vrpn_Event_Mouse.h \
	vrpn_Flock.h \
	vrpn_Flock_Parallel.h \
	vrpn_ForceDeviceServer.h \
	vrpn_Freespace.h \
	vrpn_Futaba.C \
	vrpn_GlobalHapticsOrb.h \
	vrpn_Griffin.h \
	vrpn_HumanInterface.h \
	vrpn_IDEA.h \
	vrpn_Imager_Stream_Buffer.h \
	vrpn_ImmersionBox.h \
	vrpn_JoyFly.h \
	vrpn_Joylin.h \
	vrpn_Keyboard.h \
	vrpn_Laputa.h \
	vrpn_LUDL.h \
	vrpn_Logitech_Controller_Raw.h \
	vrpn_Magellan.h \
	vrpn_Microsoft_Controller_Raw.h \
	vrpn_Mouse.h \
	vrpn_NationalInstruments.h \
	vrpn_nVidia_shield_controller.h \
	vrpn_Oculus.h \
	vrpn_OmegaTemperature.h \
	vrpn_OneEuroFilter.h \
	vrpn_Poser_Analog.h \
	vrpn_Poser_Tek4662.h \
	vrpn_Retrolink.h \
	vrpn_Saitek_Controller_Raw.h \
	vrpn_Spaceball.h \
	vrpn_Streaming_Arduino.h \
	vrpn_Tng3.h \
	vrpn_Tracker_3DMouse.h \
	vrpn_Tracker_AnalogFly.h \
	vrpn_Tracker_ButtonFly.h \
	vrpn_Tracker_Crossbow.h \
	vrpn_Tracker_DTrack.h \
	vrpn_Tracker_Fastrak.h \
	vrpn_Tracker_Filter.h \
	vrpn_Tracker_GPS.h \
	vrpn_Tracker_GameTrak.h \
	vrpn_Tracker_IMU.h \
	vrpn_Tracker_Isotrak.h \
	vrpn_Tracker_Liberty.h \
	vrpn_Tracker_LibertyHS.h \
	vrpn_Tracker_MotionNode.h \
	vrpn_Tracker_NDI_Polaris.h \
	vrpn_Tracker_NovintFalcon.h \
	vrpn_Tracker_OSVRHackerDevKit.h \
	vrpn_Tracker_PDI.h \
	vrpn_Tracker_PhaseSpace.h \
	vrpn_Tracker_RazerHydra.h \
	vrpn_Tracker_SpacePoint.h \
	vrpn_Tracker_TrivisioColibri.h \
	vrpn_Tracker_ViewPoint.h \
	vrpn_Tracker_WiimoteHead.h \
	vrpn_Tracker_Wintracker.h \
	vrpn_Tracker_isense.h \
	vrpn_UNC_Joystick.h \
	vrpn_Vality.h \
	vrpn_VPJoystick.h \
	vrpn_Wanda.h \
	vrpn_WiiMote.h \
	vrpn_Xkeys.h \
	vrpn_YEI_3Space.h \
	vrpn_Zaber.h \
	vrpn_inertiamouse.h \
	vrpn_nikon_controls.h \
	vrpn_raw_sgibox.h \
	vrpn_sgibox.h \

ifeq ($(HW_OS), pc_linux64)
	SLIB_FILES += vrpn_DevInput.C
	SLIB_INCLUDES += vrpn_DevInput.h
endif
ifeq ($(HW_OS), pc_linux)
	SLIB_FILES += vrpn_DevInput.C
	SLIB_INCLUDES += vrpn_DevInput.h
endif

# $(SLIB_OBJECTS):
$(OBJECT_DIR)/libvrpnserver.a: $(MAKEFILE) $(SLIB_OBJECTS)
	$(AR) $(OBJECT_DIR)/libvrpnserver.a $(SLIB_OBJECTS)
	-$(RANLIB) $(OBJECT_DIR)/libvrpnserver.a

# atmellib files.

ALIB_FILES = \
	atmellib/vrpn_atmellib_helper.C \
	atmellib/vrpn_atmellib_iobasic.C \
	atmellib/vrpn_atmellib_openclose.C \
	atmellib/vrpn_atmellib_register.C \
	atmellib/vrpn_atmellib_tester.C

ALIB_OBJECTS = $(patsubst %,$(AOBJECT_DIR)/../%,$(ALIB_FILES:.C=.o))

ALIB_INCLUDES =  \
	vrpn_atmellib.h \
	vrpn_atmellib_helper.h \
	vrpn_atmellib_errno.h

$(ALIB_OBJECTS):
$(OBJECT_DIR)/libvrpnatmel.a: $(MAKEFILE) $(ALIB_OBJECTS)
	$(AR) $(OBJECT_DIR)/libvrpnatmel.a $(ALIB_OBJECTS)
	-$(RANLIB) $(OBJECT_DIR)/libvrpnatmel.a

# gpsnmealib files.

GLIB_FILES = \
	gpsnmealib/latLonCoord.C \
	gpsnmealib/nmeaParser.C \
	gpsnmealib/typedCoord.C \
	gpsnmealib/utmCoord.C

GLIB_OBJECTS = $(patsubst %,$(GOBJECT_DIR)/../%,$(GLIB_FILES:.C=.o))

GLIB_INCLUDES =  \
	latLonCoord.h \
	nmeaParser.h \
	typeCoord.h \
	utmCoord.h

$(GLIB_OBJECTS):
$(OBJECT_DIR)/libvrpngpsnmea.a: $(MAKEFILE) $(GLIB_OBJECTS)
	$(AR) $(OBJECT_DIR)/libvrpngpsnmea.a $(GLIB_OBJECTS)
	-$(RANLIB) $(OBJECT_DIR)/libvrpngpsnmea.a

#############################################################################
#
# other stuff
#
#############################################################################

.PHONY:	clean
clean:
ifeq ($(HW_OS),)
		echo "Must specify HW_OS !"
else
	$(RMF) $(LIB_OBJECTS) $(OBJECT_DIR)/libvrpn.a \
               $(OBJECT_DIR)/libvrpn_g++.a \
               $(SLIB_OBJECTS) \
               $(ALIB_OBJECTS) \
               $(GLIB_OBJECTS) \
               $(OBJECT_DIR)/libvrpnserver.a \
               $(OBJECT_DIR)/libvrpnatmel.a \
               $(OBJECT_DIR)/libvrpngpsnmea.a \
               $(OBJECT_DIR)/libvrpnserver_g++.a \
               $(OBJECT_DIR)/.depend \
               $(OBJECT_DIR)/.depend-old
ifneq (xxx$(FORCE_GPP),xxx1)
	@echo -----------------------------------------------------------------
	@echo -- Wart: type \"$(MAKE) clean_g++\" to clean up after g++
	@echo -- I don\'t do it automatically in case you don\'t have g++
	@echo -----------------------------------------------------------------
endif
#ifneq ($(CC), g++)
#	$(MAKE) FORCE_GPP=1 clean
#endif
endif

.PHONY:	clean
clean_g++:
	$(MAKE) FORCE_GPP=1 clean


# clobberall removes the object directory for EVERY architecture.
# One problem - the object directory for pc_win32 also contains files
# that must be saved.
# clobberall also axes left-over CVS cache files.

.PHONY:	clobberall
clobberall:	clobberwin32
	$(RMF) -r $(SAFE_KNOWN_ARCHITECTURES)
	$(RMF) -r $(CLIENT_SKA)
	$(RMF) -r $(SERVER_SKA)
	$(RMF) .#* server_src/.#* client_src/.#*

.PHONY:	clobberwin32
clobberwin32:
	$(RMF) -r pc_win32/DEBUG/*
	$(RMF) -r pc_win32/vrpn/Debug/*
	$(RMF) -r client_src/pc_win32/printvals/Debug/*
	$(RMF) -r server_src/pc_win32/vrpn_server/Debug/*

install: all
	-mkdir -p $(LIB_DIR)
	( cd $(LIB_DIR) ; rm -f libvrpn*.a )
	( cd $(OBJECT_DIR) ; cp *.a $(LIB_DIR) )
	-mkdir -p $(INCLUDE_DIR)
	cp vrpn*.h $(INCLUDE_DIR)

uninstall:
	( cd $(LIB_DIR) ; rm -f libvrpn*.a )
	( cd $(INCLUDE_DIR) ; rm -f vrpn*.h )

#############################################################################
#
# Dependencies
#
#   If it doesn't already exist, this makefile automatically creates
#   a dependency file called .depend.  Then it includes it so that
#   the build will know the dependency information.
#
#   to recreate a dependencies file, type  "make depend"
#   do this any time you add a file to the project,
#   or add/remove #include lines from a source file
#
#   if you are on an SGI and want g++ to make the dependency file,
#   then type:    gmake CC=g++ depend
#
#   if you don't want a dependency file, then remove .depend if it exists,
#   and type "touch .depend".  if it exists (and is empty), make will not
#   automatically create it or automatically update it (unless you type
#   make depend)
#

###############
### this way works better
###    you type "make depend" anytime you add a file or
###    add/remove #includes from a file
########

include $(OBJECT_DIR)/.depend

.PHONY: depend
depend:
	-$(MVF) $(OBJECT_DIR)/.depend $(OBJECT_DIR)/.depend-old
	$(MAKE) $(OBJECT_DIR)/.depend

$(OBJECT_DIR)/.depend:
	@echo ----------------------------------------------------------------
	@echo -- Making dependency file.  If you add files to the makefile,
	@echo -- or add/remove includes from a .h or .C file, then you should
	@echo -- remake the dependency file by typing \"$(MAKE) depend\"
	@echo ----------------------------------------------------------------
	-mkdir -p $(OBJECT_DIR)
ifeq ($(HW_OS),hp700_hpux10)
	@echo -- $(HW_OS): Using g++ since HP CC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(ALL_CXXFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
else
  ifeq ($(HW_OS),hp_flow_aCC)
	@echo -- $(HW_OS): Using g++ since HP aCC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(ALL_CXXFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
  else
    ifeq ($(HW_OS),powerpc_aix)
	@$(RMF) *.u
	$(SHELL) -ec '$(CC) -E -M $(CFLAGS) $(LIB_FILES) > /dev/null 2>&1'
	cat *.u > .depend
	@$(RMF) *.u
    else
	$(SHELL) -ec '$(CXX) -M $(ALL_CXXFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
    endif
  endif
endif
	@echo ----------------------------------------------------------------
