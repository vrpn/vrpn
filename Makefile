#############################################################################
#	Makefile for the nanoManipulator client application.  Needs to be
# built using 'gmake'.  Should run on any architecture that is currently
# supported.  It should be possible to build simultaneously on multiple
# architectures.
#
# Author: Russ Taylor, 10/2/1997
#	  
# modified:
#   Tom Hudson, 13 Feb 1998
#   Build two different libraries:  client (libvrpn) and server
# (libvrpnserver).  Our solution is to compile twice, once with the
# flag -DVRPN_CLIENT_ONLY and once without.  Any server-specific code
# (vrpn_3Space, vrpn_Tracker_Fastrak, vrpn_Flock) should ONLY be compiled into
# the server library!
#############################################################################

##########################
# common definitions
#

MAKEFILE := Makefile
MAKE := gmake -f $(MAKEFILE)
HW_OS := $(shell hw_os)

# Which C++ compiler to use.  Default is g++, but some don't use this.
#
# IF YOU CHANGE THESE, document either here or in the header comment
# why.  Multiple contradictory changes have been made recently.

CC := g++
ifeq ($(HW_OS),sgi_irix)
	CC := CC
endif
ifeq ($(HW_OS),hp700_hpux10)
	CC := CC +a1
endif
ifeq ($(HW_OS),sparc_sunos)
	CC := /usr/local/lib/CenterLine/bin/CC
endif

##########################
# directories
#

HMD_DIR 	 := /afs/cs.unc.edu/proj/hmd
#INCLUDE_DIR 	 := $(HMD_DIR)/include
LIB_DIR 	 := $(HMD_DIR)/lib/$(HW_OS)

BETA_DIR	 := $(HMD_DIR)/beta
#BETA_INCLUDE_DIR := $(BETA_DIR)/include
BETA_LIB_DIR 	 := $(BETA_DIR)/lib/$(HW_OS)

# hook to quick go to working ARM set.
ARM_INCLUDE 	 := -I/afs/unc/proj/grip/arm/armlib/stable 
ARM_LIB		 := -L/afs/unc/proj/grip/arm/armlib/stable/$(HW_OS) 

# subdirectory for make
OBJECT_DIR	 := $(HW_OS)
SOBJECT_DIR      := $(HW_OS)/server


##########################
# Include flags
#

ifeq ($(HW_OS),pc_linux)
    SYS_INCLUDE := -I/usr/include -I/usr/local/contrib/include \
		-I/usr/local/contrib/mod/include -I/usr/include/bsd \
		-I/usr/include/g++
else
 ifeq ($(HW_OS),sgi_irix)
  SYS_INCLUDE := -I/usr/local/contrib/mod/include
 else
  ifeq ($(HW_OS),hp700_hpux10)
   SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include\
		  -I/usr/include/bsd
  else
   SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include
  endif
 endif
endif

#INCLUDE_FLAGS := -I. -I$(BETA_INCLUDE_DIR) -I$(INCLUDE_DIR) $(SYS_INCLUDE)
INCLUDE_FLAGS := -I. $(SYS_INCLUDE)

##########################
# Load flags
#

LOAD_FLAGS := -L./$(HW_OS) $(ARM_LIB) \
		-L$(BETA_LIB_DIR) -L$(LIB_DIR) -L/usr/local/lib \
		-L/usr/local/contrib/unmod/lib -L/usr/local/contrib/mod/lib -g

ifeq ($(HW_OS),sgi_irix)
	LOAD_FLAGS := $(LOAD_FLAGS) -old_ld
endif

ifeq ($(HW_OS),pc_linux)
	LOAD_FLAGS := $(LOAD_FLAGS) -L/usr/X11R6/lib
endif

##########################
# Libraries
#

ifeq ($(HW_OS),pc_linux)
	ARCH_LIBS := -lbsd -ldl
else
	ARCH_LIBS :=
endif

ifeq ($(HW_OS),sgi_irix)
	TCL_LIBS := /usr/local/contrib/moderated/lib/libtcl.a \
		/usr/local/contrib/moderated/lib/libtk.a
else
	TCL_LIBS := -ltcl -ltk
endif

LIBS := -lv -ltracker -lad -lquat -lsound -larm -lvrpn -lsdi \
	$(TCL_LIBS) -lXext -lX11 $(ARCH_LIBS) -lm

#include the renderman library
ifeq ($(HW_OS), hp700_hpux10)
	LIBS := $(LIBS) -lribout
endif

#
# Defines for the compilation, CFLAGS
#

DEFINES		 := -DINCLUDE_CALLBACK_CODE
CFLAGS		 := $(INCLUDE_FLAGS) $(DEFINES) -g

#############################################################################
#
# implicit rule for all .c files
#
.SUFFIXES:	.c .C .o .a

.c.o:
	$(CC) -c $(CFLAGS) $<
.C.o:
	$(CC) -c $(CFLAGS) $<

# Build objects from .c files
$(OBJECT_DIR)/%.o: %.c $(LIB_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -o $@ -c $<

# Build objects from .C files
$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -DVRPN_CLIENT_ONLY -o $@ -c $<

# Build objects from .C files
$(SOBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -o $@ -c $<

#
#
#############################################################################

all:	client server

.PHONY:	client
client:
	$(MAKE) $(OBJECT_DIR)/libvrpn.a

.PHONY:	server
server:
	$(MAKE) $(OBJECT_DIR)/libvrpnserver.a

$(OBJECT_DIR):
	-mkdir $(OBJECT_DIR)

$(SOBJECT_DIR):
	-mkdir $(SOBJECT_DIR)

#############################################################################
#
# library code
#
#############################################################################

LIB_FILES =  vrpn_Connection.C vrpn_Tracker.C vrpn_Button.C \
	     vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C

LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.C=.o))

LIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h

SLIB_FILES =  vrpn_Connection.C vrpn_Tracker.C vrpn_3Space.C vrpn_Button.C \
	     vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C \
	     vrpn_Flock.C vrpn_Tracker_Fastrak.C vrpn_Dyna.C \
	     vrpn_Flock_Slave.C vrpn_Flock_Master.C

# Until we have tracker.h, we can't compile vrpn_Tracker_Ceiling

SLIB_OBJECTS = $(patsubst %,$(SOBJECT_DIR)/%,$(SLIB_FILES:.C=.o))

SLIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_3Space.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h \
	       vrpn_Flock.h


$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) $(LIB_OBJECTS)
	ar ruv $(OBJECT_DIR)/libvrpn.a $(LIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpn.a

$(OBJECT_DIR)/libvrpnserver.a: $(MAKEFILE) $(SOBJECT_DIR) $(SLIB_OBJECTS)
	ar ruv $(OBJECT_DIR)/libvrpnserver.a $(SLIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpnserver.a

#############################################################################
#
# other stuff
#
#############################################################################

clean:
	\rm -f $(LIB_OBJECTS) $(OBJECT_DIR)/libvrpn.a \
		$(SLIB_OBJECTS) $(OBJECT_DIR)/libvrpnserver.a

#############################################################################
#
# Dependencies that are non-obvious
#

