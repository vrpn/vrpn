#############################################################################
#	Makefile for the nanoManipulator client application.  Needs to be
# built using 'gmake'.  Should run on any architecture that is currently
# supported.  It should be possible to build simultaneously on multiple
# architectures.
#
# On the sgi, both g++ and CC verisons are built by default.
#
# Author: Russ Taylor, 10/2/1997
#	  
# modified:
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
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
##########################

MV = /bin/mv
MVF = $(MV) -f

RM = /bin/rm
RMF = $(RM) -f

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

# Which C++ compiler to use.  Default is g++, but some don't use this.
#
# IF YOU CHANGE THESE, document either here or in the header comment
# why.  Multiple contradictory changes have been made recently.

# On the sgi, both g++ and CC versions are compiled by default.
# CC compiles default to old 32-bit format;  'gmake n32' builds -n32.


CC := g++
AR := ar

ifeq ($(FORCE_GPP),1)
  CC := g++
else

  ifeq ($(HW_OS),sgi_irix)
	SGI_CC_FLAGS := -32
	OBJECT_DIR_SUFFIX :=
	ifeq ($(SGI_ABI_N32),1)
		SGI_CC_FLAGS := -n32 -TARG:platform=ip19 -mips3
		OBJECT_DIR_SUFFIX := .n32
	endif
	CC := CC $(SGI_CC_FLAGS)
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
endif

ifeq ($(HW_OS),sparc_solaris)
  AR := /usr/ccs/bin/ar
endif

##########################
# directories
#

HMD_DIR 	 := /afs/cs.unc.edu/proj/hmd
HMD_INCLUDE_DIR	 := $(HMD_DIR)/include

BETA_DIR         := $(HMD_DIR)/beta
BETA_INCLUDE_DIR := $(BETA_DIR)/include
BETA_LIB_DIR     := $(BETA_DIR)/lib

# subdirectory for make
ifeq ($(FORCE_GPP),1)
OBJECT_DIR	 := $(HW_OS)/g++
SOBJECT_DIR      := $(HW_OS)/g++/server
else
UNQUAL_OBJECT_DIR := $(HW_OS)
UNQUAL_SOBJECT_DIR := $(HW_OS)/server
OBJECT_DIR	 := $(HW_OS)$(OBJECT_DIR_SUFFIX)
SOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/server
endif

# directories that we can do an rm -f on because they only contain
# object files and executables
SAFE_KNOWN_ARCHITECTURES :=	hp700_hpux/* hp700_hpux10/* mips_ultrix/* \
	pc_linux/* sgi_irix/* sgi_irix.n32/* sparc_solaris/* sparc_sunos/* pc_cygwin/*

CLIENT_SKA = $(patsubst %,client_src/%,$(SAFE_KNOWN_ARCHITECTURES))
SERVER_SKA = $(patsubst %,server_src/%,$(SAFE_KNOWN_ARCHITECTURES))

##########################
# Include flags
#

SYS_INCLUDE := -I/usr/local/contrib/include -I/usr/local/contrib/mod/include

ifeq ($(HW_OS),pc_linux)
  SYS_INCLUDE := -I/usr/include -I/usr/local/contrib/include \
	  	 -I/usr/local/contrib/mod/include -I/usr/include/bsd \
		 -I/usr/include/g++
endif

ifeq ($(HW_OS),sgi_irix)
  SYS_INCLUDE := -I/usr/local/contrib/mod/include
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

# On the PC, place quatlib in the directory ../quat.  No actual system
# includes should be needed.
ifeq ($(HW_OS),pc_cygwin)
  INCLUDE_FLAGS := -I. -I../quat
else

  INCLUDE_FLAGS := -I. $(SYS_INCLUDE) -I$(BETA_INCLUDE_DIR) -I$(HMD_INCLUDE_DIR) -I../quat

endif
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
#  ifeq ($(HW_OS),sparc_solaris)
#	ARCH_LIBS := -lsocket -lnsl
#  else
	ARCH_LIBS :=
#  endif
endif


LIBS := -lquat -lsdi $(TCL_LIBS) -lXext -lX11 $(ARCH_LIBS) -lm

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
#$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES)
	$(CC) $(CFLAGS) -DVRPN_CLIENT_ONLY -o $@ -c $<

# Build objects from .C files
$(SOBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
	$(CC) $(CFLAGS) -o $@ -c $<

#
#
#############################################################################

# If we're building for sgi_irix, we need both g++ and non-g++ versions,
# unless we're building for one of the weird ABIs, which are only supported
# by the native compiler.

ifeq ($(HW_OS),sgi_irix)
  ifeq ($(SGI_ABI_N32),1)
all:	client server
  else
all:	client server client_g++ server_g++
  endif
else
  ifeq ($(HW_OS),pc_cygwin)
all:	client
  else
all:	client server
  endif
endif

.PHONY:	n32
n32:
	$(MAKE) SGI_ABI_N32=1 all

.PHONY:	client_g++
client_g++:
	$(MAKE) FORCE_GPP=1 $(UNQUAL_OBJECT_DIR)/g++/libvrpn.a
	$(MV) $(UNQUAL_OBJECT_DIR)/g++/libvrpn.a $(UNQUAL_OBJECT_DIR)/libvrpn_g++.a

.PHONY:	server_g++
server_g++:
	$(MAKE) FORCE_GPP=1 $(UNQUAL_OBJECT_DIR)/g++/libvrpnserver.a
	$(MV) $(UNQUAL_OBJECT_DIR)/g++/libvrpnserver.a $(UNQUAL_OBJECT_DIR)/libvrpnserver_g++.a

.PHONY:	client
client: $(OBJECT_DIR)/libvrpn.a
#	$(MAKE) $(OBJECT_DIR)/libvrpn.a

.PHONY:	server
server:
	$(MAKE) $(OBJECT_DIR)/libvrpnserver.a

$(OBJECT_DIR):
	-mkdir $(OBJECT_DIR)

$(SOBJECT_DIR):
	-mkdir $(SOBJECT_DIR)

################
#  work-in-progress
################

.PHONY:	newclient
newclient: 
	@echo new objects are: $(NEW_LIB_OBJECTS)
	@echo 
	$(MAKE) $(NEW_LIB_OBJECTS)

#############################################################################
#
# library code
#
#############################################################################

# files to be compiled into the client library

LIB_FILES =  vrpn_Connection.C vrpn_Tracker.C vrpn_Button.C \
	     vrpn_Sound.C vrpn_ForceDevice.C vrpn_Clock.C vrpn_Shared.C \
	     vrpn_Ohmmeter.C vrpn_Analog.C vrpn_FileConnection.C \
             vrpn_FileController.C vrpn_Forwarder.C vrpn_Text.C \
             vrpn_ForwarderController.C vrpn_Serial.C vrpn_Dial.C \
             vrpn_SharedObject.C

LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.C=.o))

LIB_INCLUDES = vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h \
	       vrpn_Sound.h vrpn_ForceDevice.h vrpn_Clock.h vrpn_Shared.h \
	       vrpn_Ohmmeter.h vrpn_Analog.h vrpn_FileConnection.h \
               vrpn_FileController.h vrpn_Forwarder.h vrpn_Text.h \
               vrpn_ForwarderController.h vrpn_Serial.h vrpn_Dial.h \
               vrpn_SharedObject.h

# additional files for the new connection work-in-progress

NEW_LIB_FILES = \
      vrpn_BaseConnectionController.C \
      vrpn_ClientConnectionController.C \
      vrpn_ServerConnectionController.C \
      vrpn_BaseConnection.C \
      vrpn_NetConnection.C \
      vrpn_NewFileConnection.C \
      vrpn_NewFileController.C \
      vrpn_FileLogger.C \
      vrpn_ConnectionOldCommonStuff.C \

NEW_LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(NEW_LIB_FILES:.C=.o))

NEW_LIB_INCLUDES = \
      vrpn_BaseConnectionController.h \
      vrpn_ClientConnectionController.h \
      vrpn_ServerConnectionController.h \
      vrpn_BaseConnection.h \
      vrpn_NetConnection.h \
      vrpn_FileConnectionControllerInterface.h\
      vrpn_NewFileConnection.h \
      vrpn_NewFileController.h \
      vrpn_FileLogger.h \
      vrpn_ConnectionOldCommonStuff.h \
      vrpn_ConnectionCommonStuff.h


# Additional files to be compiled into the server library

# We aren't going to use architecture-dependent sets of files.
# If vrpn_sgibox isn't supposed to be compiled on any other architecture,
# then put all of it inside "#ifdef sgi"!

SLIB_FILES =  $(LIB_FILES) vrpn_3Space.C \
	     vrpn_Flock.C vrpn_Tracker_Fastrak.C vrpn_Dyna.C \
	     vrpn_Flock_Parallel.C  vrpn_Joystick.C \
	     vrpn_JoyFly.C vrpn_sgibox.C vrpn_CerealBox.C \
             vrpn_Tracker_AnalogFly.C vrpn_raw_sgibox.C vrpn_Dial.C

SLIB_OBJECTS = $(patsubst %,$(SOBJECT_DIR)/%,$(SLIB_FILES:.C=.o))

SLIB_INCLUDES = $(LIB_INCLUDES) vrpn_3Space.h \
	       vrpn_Flock.h vrpn_Tracker_Fastrak.h vrpn_Dyna.h \
	       vrpn_Flock_Parallel.h vrpn_Joystick.h \
	       vrpn_JoyFly.h vrpn_sgibox.h vrpn_raw_sgibox.h \
               vrpn_CerealBox.h vrpn_Tracker_AnalogFly.h


#$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) \
#                         lib_depends \
#                         $(LIB_OBJECTS) $(LIB_INCLUDES)
#$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) \

$(OBJECT_DIR)/libvrpn.a: $(MAKEFILE) $(OBJECT_DIR) $(LIB_OBJECTS)
	$(AR) ruv $(OBJECT_DIR)/libvrpn.a $(LIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpn.a

$(OBJECT_DIR)/libvrpnserver.a: $(MAKEFILE) $(SOBJECT_DIR) $(SLIB_OBJECTS) \
			$(SLIB_INCLUDES)
	$(AR) ruv $(OBJECT_DIR)/libvrpnserver.a $(SLIB_OBJECTS)
	-ranlib $(OBJECT_DIR)/libvrpnserver.a

#############################################################################
#
# other stuff
#
#############################################################################

.PHONY:	clean
clean:
	$(RMF) $(LIB_OBJECTS) $(OBJECT_DIR)/libvrpn.a \
               $(OBJECT_DIR)/libvrpn_g++.a $(SLIB_OBJECTS) \
               $(OBJECT_DIR)/libvrpnserver.a \
               $(OBJECT_DIR)/libvrpnserver_g++.a \
               .depend .depend-old
ifneq (xxx$(FORCE_GPP),xxx1)
	@echo -----------------------------------------------------------------
	@echo -- Wart: type \"$(MAKE) clean_g++\" to clean up after g++
	@echo -- I don\'t do it automatically in case you don\'t have g++
	@echo -----------------------------------------------------------------
endif
#ifneq ($(CC), g++)
#	$(MAKE) FORCE_GPP=1 clean
#endif

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


.PHONY:	beta
beta :
	$(MAKE) clean
	$(MAKE) all
	-$(MV) $(OBJECT_DIR)/libvrpn.a $(OBJECT_DIR)/libvrpn_g++.a \
	    $(OBJECT_DIR)/libvrpnserver.a $(OBJECT_DIR)/libvrpnserver_g++.a \
            $(BETA_LIB_DIR)/$(OBJECT_DIR)
	-ranlib $(BETA_LIB_DIR)/$(OBJECT_DIR)/libvrpn.a
	-ranlib $(BETA_LIB_DIR)/$(OBJECT_DIR)/libvrpnserver.a
	-( cd $(BETA_INCLUDE_DIR); $(RMF) $(SLIB_INCLUDES) )
	cp $(SLIB_INCLUDES) $(BETA_INCLUDE_DIR) 

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

include .depend

.PHONY: depend
depend:
	-$(MVF) .depend .depend-old
	$(MAKE) .depend
.depend:
	@echo ----------------------------------------------------------------
	@echo -- Making dependency file.  If you add files to the makefile,
	@echo -- or add/remove includes from a .h or .C file, then you should
	@echo -- remake the dependency file by typing \"$(MAKE) depend\"
	@echo ----------------------------------------------------------------
ifeq ($(HW_OS),hp700_hpux10)
	@echo -- $(HW_OS): Using g++ since HP CC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(CFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > .depend'
else
  ifeq ($(HW_OS),hp_flow_aCC)
	@echo -- $(HW_OS): Using g++ since HP aCC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(CFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > .depend'
  else
	$(SHELL) -ec '$(CC) -M $(CFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > .depend'
  endif
endif
	@echo ----------------------------------------------------------------
