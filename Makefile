#############################################################################
#	Makefile for the nanoManipulator client application.  Needs to be
# built using 'gmake'.  Should run on any architecture that is currently
# supported.  It should be possible to build simultaneously on multiple
# architectures.
#
# Author: Russ Taylor, 10/2/1997
#	  
# modified:
#############################################################################

##########################
# common definitions
#

MAKEFILE := Makefile
MAKE := gmake -f $(MAKEFILE)
HW_OS := $(shell hw_os)

# Which C++ compiler to use.  Default is g++, but some don't use this.
CC := g++
#ifeq ($(HW_OS),sgi_irix)
	CC := CC
#endif
#ifeq ($(HW_OS),hp700_hpux10)
#	CC := CC +a1
#endif
#ifeq ($(HW_OS),sparc_sunos)
#	CC := /usr/local/lib/CenterLine/bin/CC
#endif

##########################
# directories
#

HMD_DIR 	 := /afs/cs.unc.edu/proj/hmd
INCLUDE_DIR 	 := $(HMD_DIR)/include
LIB_DIR 	 := $(HMD_DIR)/lib/$(HW_OS)

BETA_DIR	 := $(HMD_DIR)/beta
BETA_INCLUDE_DIR := $(BETA_DIR)/include
BETA_LIB_DIR 	 := $(BETA_DIR)/lib/$(HW_OS)

# hook to quick go to working ARM set.
ARM_INCLUDE 	 := -I/afs/unc/proj/grip/arm/armlib/stable 
ARM_LIB		 := -L/afs/unc/proj/grip/arm/armlib/stable/$(HW_OS) 

# subdirectory for make
OBJECT_DIR	 := $(HW_OS)


##########################
# Include flags
#

ifeq ($(HW_OS),pc_linux)
    SYS_INCLUDE := -I/usr/include -I/usr/local/contrib/include \
		-I/usr/local/contrib/mod/include -I/usr/include/bsd
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

INCLUDE_FLAGS := -I. -I$(BETA_INCLUDE_DIR) -I$(INCLUDE_DIR) $(SYS_INCLUDE)

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
$(OBJECT_DIR)/%.o: %.c $(MICROSCAPE_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -o $@ -c $<

# Build objects from .C files
$(OBJECT_DIR)/%.o: %.C $(MICROSCAPE_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -o $@ -c $<

#
#
#############################################################################

all:
	$(MAKE) $(OBJECT_DIR)/libvrpn.a

$(OBJECT_DIR):
	-mkdir $(OBJECT_DIR)

#############################################################################
#
# library code
#
#############################################################################


LIB_FILES =  \
	vrpn_Connection.C vrpn_Tracker.C vrpn_3Space.C vrpn_Button.C \
	vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C \
	vrpn_Flock.C vrpn_Tracker_Fastrak.C 


LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.C=.o))

LIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_3Space.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h \
	       vrpn_Flock.h


$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) $(LIB_OBJECTS)
	ar ruv $(OBJECT_DIR)/libvrpn.a $(LIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpn.a

#############################################################################
#
# microscape code
#
#############################################################################

#
# microscape
#
MICROSCAPE_FILES = minit.c microscape.c animate.c interaction.c \
		   stm_file.c server_talk.c normal.c splat.c \
		   relax.c drift.c butt_mode.c updt_display.c \
		   x_util.c termio.c x_graph.c x_aux.c tcl_tk.c \
		   openGL.c font.c spm_gl.c globjects.c

ifeq ($(HW_OS),hp700_hpux10)
	MICROSCAPE_FILES := $(MICROSCAPE_FILES) RenderMan.c
endif

MICROSCAPE_CC_FILES = Grid.C Plane.C Point.C Position.C Debug.C String.C \
			readTopometrixFile.C Tcl_Linkvar.C \
			colormap.C active_set.C ohmeter.C PPM.C filter.C vrml.C

MICROSCAPE_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(MICROSCAPE_FILES:.c=.o)) \
	$(patsubst %,$(OBJECT_DIR)/%,$(MICROSCAPE_CC_FILES:.C=.o))

MICROSCAPE_INCLUDES = ../stm.h microscape.h x_util.h x_aux.h openGL.h \
			tcl_tk.h Tcl_Linkvar.h ../colormaps/colormap.h \
			active_set.h \
			../Grid.h ../Plane.h Makefile ../stm_cmd.h ../Point.h \
			../PPM.h ../Position.h

$(OBJECT_DIR)/microscape: $(OBJECT_DIR) $(MICROSCAPE_OBJECTS)
	$(CC) $(LOAD_FLAGS) $(MICROSCAPE_OBJECTS) $(LIBS) \
		-o $(OBJECT_DIR)/microscape

#############################################################################
#
# other stuff
#
#############################################################################

clean:
	\rm -f $(LIB_OBJECTS) $(OBJECT_DIR)/libvrpn.a

#############################################################################
#
# Dependencies that are non-obvious
#












