#############################################################################
#
# makefile for quaternion lib
#
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
# Set this to compile for ARM gcc compiler to use on embedded devices
# from a Linux box.
#HW_OS := pc_linux_arm
# Try this to compile for ARM gcc compiler to use on embedded devices
# from a Cygwin environment.
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
# be the minimum version.
##########################
#MAC_OS_MIN_VERSION := 10.4
#MAC_OS_MIN_VERSION := 10.5
#MAC_OS_MIN_VERSION := 10.6
#MAC_OS_MIN_VERSION := 10.7
#MAC_OS_MIN_VERSION := 10.8


INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
INCLUDE_INSTALL_DIR := $(INSTALL_DIR)/include
LIB_INSTALL_DIR := $(INSTALL_DIR)/lib

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

# Define defaults for the compilter, archiver, and ranlib.
CC := gcc
AR := ar ruv
RANLIB := ranlib

ifdef PBASE_ROOT
  HW_OS = hp_flow
  ifeq ($(PXFL_COMPILER), aCC)
    HW_OS = hp_flow_aCC
  endif
endif


ifeq ($(HW_OS),sgi_irix)
   ifndef SGI_ABI
      SGI_ABI := n32
   endif
   ifndef SGI_ARCH
      SGI_ARCH := mips3
   endif
   OBJECT_DIR_SUFFIX := .$(SGI_ABI).$(SGI_ARCH)
   CC = cc -$(SGI_ABI) -$(SGI_ARCH)
endif

AR := ar ruv
RANLIB := ranlib

ifeq ($(HW_OS), pc_cygwin)
endif

ifeq ($(HW_OS), hp700_hpux10)
  CC = cc -Ae
endif

ifeq ($(HW_OS), pc_linux)
endif

ifeq ($(HW_OS), pc_linux_ia64)
endif

ifeq ($(HW_OS), pc_linux64)
  CC = gcc -m64 -fPIC
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

ifeq ($(HW_OS), powerpc_macosx)
  CC = $(MAC_GCC) -arch ppc -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
  AR = libtool -static -o
  RANLIB = :
endif

ifeq ($(HW_OS), universal_macosx)
  CC = $(MAC_GCC) -arch ppc -arch i386 -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
  AR = libtool -static -o
  RANLIB = :
endif

 ifeq ($(HW_OS), macosx_32_64)
        CC = $(MAC_GCC) -arch i386 -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
        RANLIB = :
        AR = libtool -static -o
	SYSLIBS = -framework CoreFoundation -framework IOKit -framework System
  endif
  
    ifeq ($(HW_OS), macosx_64)
  		 ifeq ($(MAC_OS_SDK), MacOSX10.6.sdk)
    		CC = $(MAC_GCC) -arch x86_64 -isysroot /Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
       else
     		CC = $(MAC_GCC) -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/$(MAC_OS_SDK) -mmacosx-version-min=$(MAC_OS_MIN_VERSION)
       endif
        RANLIB = :
        AR = libtool -static -o
		SYSLIBS = -framework CoreFoundation -framework IOKit -framework System
    endif


ifeq ($(HW_OS), pc_linux_arm)
  CC = arm-linux-gcc
  AR = arm-linux-ar ruv
  RANLIB = arm-linux-ranlib
endif

ifeq ($(HW_OS), pc_cygwin_arm)
  CC = arm-unknown-linux-gnu-gcc
  AR = arm-unknown-linux-gnu-ar ruv
  RANLIB = arm-unknown-linux-gnu-ranlib
endif

ifeq ($(HW_OS), sparc_solaris)
  CC = /opt/SUNWspro/bin/CC
endif

ifeq ($(HW_OS), sparc_solaris_64)
  CC = /opt/SUNWspro/bin/CC -xarch=v9a
endif

ifeq ($(HW_OS), hp_flow)
  CC = g++
endif

ifeq ($(HW_OS), hp_flow_aCC)
  CC = /opt/aCC/bin/aCC
endif


MAKEFILE = Makefile

QUAT_LIB = libquat.a

# flags
INCLUDE_FLAGS = -I.

ifeq ($(HW_OS), pc_cygwin)
  INCLUDE_FLAGS = -I.
endif

ifeq ($(HW_OS), hp_flow)
  INCLUDE_FLAGS =  -I. -I$(INCLUDE_INSTALL_DIR) -DFLOW
endif

ifeq ($(HW_OS), hp_flow_aCC)
  INCLUDE_FLAGS =  -I. -I$(INCLUDE_INSTALL_DIR) -DFLOW
endif

override CFLAGS := -g $(INCLUDE_FLAGS) $(CFLAGS)
override OPT_CFLAGS := -O $(INCLUDE_FLAGS) $(CFLAGS)

override LDFLAGS := -L. -L$(HW_OS) $(LDFLAGS)
LINT_FLAGS = $(INCLUDE_FLAGS)

#############################################################################
#
# for building library
#
#############################################################################

lib:	$(HW_OS)$(OBJECT_DIR_SUFFIX) libquat.a

$(HW_OS)$(OBJECT_DIR_SUFFIX):
	-mkdir -p $(HW_OS)$(OBJECT_DIR_SUFFIX)

#############################################################################
#
# example/test programs
#
#############################################################################

TEST_FILES = eul matrix_to_posquat qmat qmult qxform qmake timer qpmult

all :
	-rm $(TEST_FILES)
	$(MAKE) $(TEST_FILES)

#
# timer- time some operation
#
timer : timer.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# matrix_to_posquat- matrix_to_posquat to quat
#
matrix_to_posquat : testapps/matrix_to_posquat.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# eul- eul to quat
#
eul : testapps/eul.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# qmult- multiply 2 quats
#
qmult : testapps/qmult.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# qpmult- multiply 2 pmatrices
#
qpmult : testapps/qpmult.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# qxform- xform a vec
#
qxform : testapps/qxform.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# qmat- matrix to quaternion
#
qmat : qmat.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) testapps/$@.c -lquat -lm

#
# qmake- tests q_make
#
qmake : qmake.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

oglconv : oglconv.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

oglmult : oglmult.c
	$(CC) -o $(HW_OS)/$@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#############################################################################
#
# rules and definitions for making quaternion library
#
#############################################################################

QUAT_INCLUDES = quat.h
QUAT_C_FILES = quat.c matrix.c vector.c xyzquat.c
QUAT_OBJ_FILES = $(QUAT_C_FILES:.c=.o)

$(QUAT_LIB) :  $(QUAT_OBJ_FILES) $(MAKEFILE)
	@echo "Building $@..."
	$(AR) $(QUAT_LIB) $(QUAT_OBJ_FILES)
	-$(RANLIB) $(QUAT_LIB)
	mv $(QUAT_LIB) *.o $(HW_OS)$(OBJECT_DIR_SUFFIX)

$(QUAT_OBJ_FILES) : $(QUAT_INCLUDES)

#############################################################################
#
# miscellaneous rules
#
#############################################################################

lint :
	lint $(LINT_FLAGS) $(COORD_FILES)

install: lib
	#-mkdir $(BIN_DIR)
	#( cd $(BIN_DIR) ; rm -f $(INSTALL_APPS) )
	#( cd $(OBJ_DIR) ; cp $(INSTALL_APPS) $(BIN_DIR) )
	#( cd $(BIN_DIR) ; strip $(INSTALL_APPS) )
	-mkdir -p $(LIB_DIR)
	( cd $(HW_OS)$(OBJECT_DIR_SUFFIX) ; cp $(QUAT_LIB) $(LIB_DIR) )
	-mkdir -p $(INCLUDE_DIR)
	cp quat.h $(INCLUDE_DIR)

allclean :
	-/bin/rm -f *.o *.a *~ *.j foo a.out $(TEST_FILES)
	-/bin/rm -fr $(HW_OS)$(OBJECT_DIR_SUFFIX)/*.o $(HW_OS)$(OBJECT_DIR_SUFFIX)/*.a

clean :
	-/bin/rm -f *.o *.a *~ *.j foo a.out $(TEST_FILES)
	-/bin/rm -fr $(HW_OS)$(OBJECT_DIR_SUFFIX)/*.o $(HW_OS)$(OBJECT_DIR_SUFFIX)/*.a


install: lib
	-mkdir -p $(LIB_INSTALL_DIR)
	( cd $(LIB_INSTALL_DIR) ; rm -f libquat.a )
	( cd $(HW_OS)$(OBJECT_DIR_SUFFIX) ; cp lib*.a $(LIB_INSTALL_DIR) )
	-mkdir -p $(INCLUDE_INSTALL_DIR)
	cp quat.h $(INCLUDE_INSTALL_DIR)


