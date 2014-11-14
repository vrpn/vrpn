#############################################################################
#	Makefile for the nanoManipulator client application.  Needs to be
# built using 'gmake'.  Should run on any architecture that is currently
# supported.  It should be possible to build simultaneously on multiple
# architectures.
#
# On the sgi, both g++ and CC versions are built by default.
#
# Author: Russ Taylor, 10/2/1997
#
# modified:
# * Oliver Otto, 15/10/2003
#    * modified client makefile to create shared library libjava_vrpn.so
#    * tested on SGI and linux, make sure that the java path is right
#    * use the make_header.sh script to create all header files
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
# Try this to cross-compile on a Linux PC for an ARM embedded controller.
#HW_OS := pc_linux_arm
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
#HW_OS := sparc_solaris
#HW_OS := sparc_solaris_64
#HW_OS := powerpc_aix
#HW_OS := powerpc_macosx
#HW_OS := linux_x64
##########################

INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
INCLUDE_DIR := $(INSTALL_DIR)/include
LIB_DIR := $(INSTALL_DIR)/lib
JAVA_LIB_DIR := $(INSTALL_DIR)/share/java/

VERSION := 07.22

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

# Which C++ compiler to use.  Default is g++, but some don't use this.
#
# IF YOU CHANGE THESE, document either here or in the header comment
# why.  Multiple contradictory changes have been made recently.


CC := g++
AR := ar ruv
# need default 'ranlib' to be touch for platforms that don't use it,
# otherwise make fails.
RANLIB := touch

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

  ifeq ($(HW_OS), pc_linux64)
        CC := g++ -m64 -fPIC
        RANLIB := ranlib
  endif

  ifeq ($(HW_OS), pc_linux)
        CC := g++
        RANLIB := ranlib
  endif

  ifeq ($(HW_OS), linux_x64)
        CC := g++ -fPIC
        RANLIB := ranlib
  endif

  ifeq ($(HW_OS), powerpc_macosx)
        CC := cc
        RANLIB := ranlib
  endif

  ifeq ($(HW_OS), universal_macosx)
        CC := g++ -arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
        RANLIB := :
        AR := libtool -static -o
	SYSLIBS := -framework CoreFoundation -framework IOKit -framework System
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

  ifeq ($(HW_OS),sgi_irix)
   ifndef SGI_ABI
      SGI_ABI := n32
   endif
   ifndef SGI_ARCH
      SGI_ARCH := mips3
   endif
   OBJECT_DIR_SUFFIX := .$(SGI_ABI).$(SGI_ARCH)
	CC := CC -$(SGI_ABI) -$(SGI_ARCH)
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

#HMD_DIR 	 := /afs/cs.unc.edu/proj/hmd
#HMD_INCLUDE_DIR	 := $(HMD_DIR)/include

#BETA_DIR         := $(HMD_DIR)/beta
#BETA_INCLUDE_DIR := $(BETA_DIR)/include
#BETA_LIB_DIR     := $(BETA_DIR)/lib

# subdirectory for make
ifeq ($(FORCE_GPP),1)
OBJECT_DIR	 := $(HW_OS)$(OBJECT_DIR_SUFFIX)/g++
SOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/g++/server
else
UNQUAL_OBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)
UNQUAL_SOBJECT_DIR := $(HW_OS)$(OBJECT_DIR_SUFFIX)/server
OBJECT_DIR	 := $(HW_OS)$(OBJECT_DIR_SUFFIX)
SOBJECT_DIR      := $(HW_OS)$(OBJECT_DIR_SUFFIX)/server
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
#  SYS_INCLUDE := -I/usr/include
   SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),universal_macosx)
#  SYS_INCLUDE := -I/usr/include
   SYS_INCLUDE :=-DMACOSX -I../isense
endif

ifeq ($(HW_OS),pc_linux)
  SYS_INCLUDE := -I/opt/Embedix/arm-linux/include \
				-I/usr/lib/jvm/default-java/include/ \
				-I/usr/lib/jvm/default-java/include/linux \
				-I./ \
				-I../
endif

ifeq ($(HW_OS),pc_linux64)
  SYS_INCLUDE := -I/opt/Embedix/arm-linux/include \
				-I/usr/lib/jvm/default-java/include/ \
				-I/usr/lib/jvm/default-java/include/linux \
				-I./ \
				-I../
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
  SYS_INCLUDE := -I/usr/java2/include -I/usr/java2/include/irix -I./ -I../
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

# On the PC, place quatlib in the directory ../../quat.  No actual system
# includes should be needed.
ifeq ($(HW_OS),pc_cygwin)
  INCLUDE_FLAGS := -I. -I../../quat
else

  INCLUDE_FLAGS := -I. $(SYS_INCLUDE) -I../../quat -I../../../quat -I../

endif
##########################
# Load flags
#

LOAD_FLAGS := -L./$(HW_OS)$(OBJECT_DIR_SUFFIX) -L/usr/local/lib \
		-L/usr/local/contrib/unmod/lib -L/usr/local/contrib/mod/lib $(DEBUG_FLAGS) $(LDFLAGS)

ifeq ($(HW_OS),sgi_irix)
	LOAD_FLAGS := $(LOAD_FLAGS) -old_ld
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

ifeq ($(HW_OS),powerpc_macosx)
	LOAD_FLAGS := $(LOAD_FLAGS)
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
    ifeq ($(HW_OS),sparc_solaris)
          ARCH_LIBS := -lsocket -lnsl
    else
          ARCH_LIBS :=
    endif
  endif
endif

LIBS := -lquat -lsdi $(TCL_LIBS) -lXext -lX11 $(ARCH_LIBS) -lm

#
# Defines for the compilation, CFLAGS
#

#CFLAGS		 := $(INCLUDE_FLAGS) -g
override CFLAGS		 := $(INCLUDE_FLAGS) $(DEBUG_FLAGS) $(CFLAGS)
override CXXFLAGS     := $(INCLUDE_FLAGS) $(DEBUG_FLAGS) $(CXXFLAGS)

#############################################################################
#
# implicit rule for all .cpp files
#
.SUFFIXES:	.cpp .C .o .a

.cpp.o:
	$(CC) -c $(CXXFLAGS) $<
.C.o:
	$(CC) -c $(CFXXLAGS) $<

# Build objects from .cpp files
#$(OBJECT_DIR)/%.o: %.cpp $(LIB_INCLUDES) $(MAKEFILE)
$(OBJECT_DIR)/%.o: %.cpp $(LIB_INCLUDES) 
	@[ -d $(OBJECT_DIR) ] || mkdir -p $(OBJECT_DIR)
	$(CC) $(CXXFLAGS) -o $@ -c $<

# Build objects from .C files
#$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
#$(OBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) 
#	@[ -d $(OBJECT_DIR) ] || mkdir -p $(OBJECT_DIR)
#	$(CC) $(CXXFLAGS) -o $@ -c $< $(LIBS) -lvrpn -exceptions

# Build objects from .C files
#$(SOBJECT_DIR)/%.o: %.C $(LIB_INCLUDES) $(MAKEFILE)
#	@[ -d $(SOBJECT_DIR) ] || mkdir -p $(SOBJECT_DIR)
#	$(CC) $(CXXFLAGS) -o $@ -c $<

#
#
#############################################################################

all:	client 

.PHONY:	client
client: $(OBJECT_DIR)/libjava_vrpn.a
	@echo -----------------------------------------------------------------
	@echo create shared library libjava_vrpn.so
	$(CC) -shared -L../$(OBJECT_DIR) -L../quat/$(OBJECT_DIR) -o $(OBJECT_DIR)/libjava_vrpn.so $(OBJECT_DIR)/*.o -lvrpnserver -lquat



$(OBJECT_DIR):
	-mkdir -p $(OBJECT_DIR)

$(SOBJECT_DIR):
	-mkdir -p $(SOBJECT_DIR)

#############################################################################
#
# library code
#
#############################################################################

# files to be compiled into the client library

LIB_FILES =  java_vrpn.cpp vrpn_AnalogOutputRemote.cpp vrpn_AnalogRemote.cpp \
		vrpn_AuxiliaryLoggerRemote.cpp vrpn_ButtonRemote.cpp \
		vrpn_ForceDeviceRemote.cpp vrpn_PoserRemote.cpp vrpn_TextReceiver.cpp \
		vrpn_TextSender.cpp vrpn_TrackerRemote.cpp vrpn_VRPNDevice.cpp \
		vrpn_FunctionGeneratorRemote.cpp

LIB_OBJECTS = $(patsubst %,$(OBJECT_DIR)/%,$(LIB_FILES:.cpp=.o))

LIB_INCLUDES = java_vrpn.h vrpn_AnalogOutputRemote.h vrpn_AnalogRemote.h \
		vrpn_AuxiliaryLoggerRemote.h vrpn_ButtonRemote.h \
		vrpn_ForceDeviceRemote.h vrpn_PoserRemote.h vrpn_TextReceiver.h \
		vrpn_TextSender.h vrpn_TrackerRemote.h vrpn_VRPNDevice.h \
		 vrpn_FunctionGeneratorRemote.h


# additional files for the new connection work-in-progress

# Additional files to be compiled into the server library

# We aren't going to use architecture-dependent sets of files.
# If vrpn_sgibox isn't supposed to be compiled on any other architecture,
# then put all of it inside "#ifdef sgi"!


#$(OBJECT_DIR)/java_vrpn.a: $(MAKEFILE) $(OBJECT_DIR) \
#                         lib_depends \
#                         $(LIB_OBJECTS) $(LIB_INCLUDES)

$(LIB_OBJECTS):
$(OBJECT_DIR)/libjava_vrpn.a: $(MAKEFILE) $(LIB_OBJECTS)
	$(AR) $(OBJECT_DIR)/libjava_vrpn.a $(LIB_OBJECTS)
	-$(RANLIB) $(OBJECT_DIR)/libjava_vrpn.a

#$(OBJECT_DIR)/libjava_vrpn.so: gcc -shared -L../$(OBJECT_DIR) -o $(OBJECT_DIR)/libjava_vrpn.so $(OBJECT_DIR)/*.o -lvrpn




#############################################################################
#
# other stuff
#
#############################################################################

.PHONY:	clean
clean:
	$(RMF) $(LIB_OBJECTS) $(OBJECT_DIR)/libjava_vrpn.a \
               $(OBJECT_DIR)/.depend $(OBJECT_DIR)/.depend-old
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

install: all
	-mkdir -p $(LIB_DIR)
	( cp -f $(OBJECT_DIR)/libjava_vrpn.so $(LIB_DIR) )
	-mkdir -p $(JAVA_LIB_DIR)
	cp -f bin/vrpn.jar $(JAVA_LIB_DIR)/vrpn-$(VERSION).jar
	ln -s $(JAVA_LIB_DIR)/vrpn.jar $(JAVA_LIB_DIR)/vrpn-$(VERSION).jar
	#-mkdir -p $(INCLUDE_DIR)
	#cp vrpn*.h $(INCLUDE_DIR)

uninstall:
	( cd $(LIB_DIR) ; rm -f libjava_vrpn.so )
	( rm -f $(JAVA_LIB_DIR)/vrpn.jar $(JAVA_LIB_DIR)/vrpn-$(VERSION).jar )

.PHONY:	beta
beta :
#	$(MAKE) clean
#	$(MAKE) all
#	-$(MV) $(OBJECT_DIR)/java_vrpn.a  \
#	    $(OBJECT_DIR)/libvrpnserver.a $(OBJECT_DIR)/libvrpnserver_g++.a \
#            $(BETA_LIB_DIR)/$(OBJECT_DIR)
#	-$(RANLIB) $(BETA_LIB_DIR)/$(OBJECT_DIR)/libvrpn.a
#	-$(RANLIB) $(BETA_LIB_DIR)/$(OBJECT_DIR)/libvrpnserver.a
#	-( cd $(BETA_INCLUDE_DIR); $(RMF) $(SLIB_INCLUDES) )
#	cp $(SLIB_INCLUDES) $(BETA_INCLUDE_DIR)

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
	@echo -- or add/remove includes from a .h or .cpp file, then you should
	@echo -- remake the dependency file by typing \"$(MAKE) depend\"
	@echo ----------------------------------------------------------------
	-mkdir -p $(OBJECT_DIR)
ifeq ($(HW_OS),hp700_hpux10)
	@echo -- $(HW_OS): Using g++ since HP CC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(CXXLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
else
  ifeq ($(HW_OS),hp_flow_aCC)
	@echo -- $(HW_OS): Using g++ since HP aCC does not understand -M
	@echo -- if this causes an error, then delete .depend and type
	@echo -- \"touch .depend\" to create an empty file
	@echo ----------------------------------------------------------------
	$(SHELL) -ec 'g++ -MM $(CXXFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
  else
    ifeq ($(HW_OS),powerpc_aix)
	@$(RMF) *.u
	$(SHELL) -ec '$(CC) -E -M $(CFLAGS) $(LIB_FILES) > /dev/null 2>&1'
	cat *.u > .depend
	@$(RMF) *.u
    else
	$(SHELL) -ec '$(CC) -M $(CFLAGS) $(LIB_FILES) \
	    | sed '\''s/\(.*\.o[ ]*:[ ]*\)/$(OBJECT_DIR)\/\1/g'\'' > $(OBJECT_DIR)/.depend'
    endif
  endif
endif
	@echo ----------------------------------------------------------------
