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

# check if its for pxfl
ifeq ($(HW_OS),hp700_hpux10)
  ifdef PBASE_ROOT
    HW_OS := hp_flow
  endif
  ifeq ($(PXFL_COMPILER), aCC)
    HW_OS := hp_flow_$(PXFL_COMPILER)
  endif
endif

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
ifeq ($(HW_OS), hp_flow_aCC)
	CC := /opt/aCC/bin/aCC 
endif

HMD_INCLUDE_DIR	:= /afs/unc/proj/hmd/include

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
   ifeq ($(HW_OS),hp700_hpux10) 
    SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include\
	 	   -I/usr/include/bsd
   else
    SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include
   endif
  endif
 endif
endif

INCLUDE_FLAGS := -I. $(SYS_INCLUDE) -I$(HMD_INCLUDE_DIR)

##########################
# Load flags
#

LOAD_FLAGS := -L./$(HW_OS) -L/usr/local/lib \
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

LIBS := -lv -ltracker -lad -lquat -lsound -larm -lvrpn -lsdi \
	-lXext -lX11 $(ARCH_LIBS) -lm

#
# Defines for the compilation, CFLAGS
#

CFLAGS		 := $(INCLUDE_FLAGS) -g

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
	     vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C \
	     vrpn_Ohmmeter.C vrpn_Analog.C 

LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.C=.o))

LIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h \
	       vrpn_Ohmmeter.h vrpn_Analog.h

SLIB_FILES =  vrpn_Connection.C vrpn_Tracker.C vrpn_3Space.C vrpn_Button.C \
	     vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C \
	     vrpn_Flock.C vrpn_Tracker_Fastrak.C vrpn_Dyna.C \
	     vrpn_Flock_Parallel.C  vrpn_Joystick.C vrpn_Analog.C \
	     vrpn_JoyFly.C vrpn_sgibox.C 

# Until we have tracker.h, we can't compile vrpn_Tracker_Ceiling

SLIB_OBJECTS = $(patsubst %,$(SOBJECT_DIR)/%,$(SLIB_FILES:.C=.o))

SLIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_3Space.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h \
	       vrpn_Flock.h vrpn_Flock_Parallel.h 


$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) $(LIB_OBJECTS) $(LIB_INCLUDES)
	ar ruv $(OBJECT_DIR)/libvrpn.a $(LIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpn.a

$(OBJECT_DIR)/libvrpnserver.a: $(MAKEFILE) $(SOBJECT_DIR) $(SLIB_OBJECTS) $(SLIB_INCLUDES)
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

