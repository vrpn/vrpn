#############################################################################
# 
#  makefile for vlib virtual worlds library example programs
#
#
#  targets:
#	
#	simple- makes the very simple example application "simple"
#	example- makes the test application "example"
#	script- makes script for running an application independent of
#			architecture/os
#	subdir- makes a subdir for objects/libs/executables for this arch/os
#
#
# revision history:
#
#  rich holloway	06/10/91	new version for example code for v3.0
#  rich holloway	05/18/91	first version
#
#############################################################################

#
# common definitions
#

# commands
#CC = /usr/local/lib/CenterLine/bin/CC
#CC = g++
CC = CC
MAKE = make
makefile = Makefile
HW_OS = `/usr/local/bin/hw_os`

#
# main directories
#

# for using beta version 
HMD_DIR 	 = /usr/proj/hmd/beta
#HMD_DIR 	 = /usr/proj/hmd

LIB_DIR 	 = $(HMD_DIR)/lib/$(HW_OS)
INCLUDE_DIR 	 = $(HMD_DIR)/include

TEST_DIR = /usr/proj/hmd/src/libs/vlib

# subdirectories for make
OBJECT_DIR 	= $(HW_OS)

# for using beta version 
PXPL5_DIR 	= /unc/pxpl5/beta
#PXPL5_DIR 	= /unc/pxpl5

#
# flags
#
# {INCLUDE,LD}_FLAGS and LIBS are only defined for recursive pass
#
LINT_FLAGS 	= $(INCLUDE_FLAGS)
CFLAGS 		= -g $(INCLUDE_FLAGS)

#LIBS_5 		= -lv -ltracker -lsound -larm -lsdi -lad -lquat \
#		  -ljoy -lphake -ljoy -lm
#

LIBS_5 		= $(PXPL5_DIR)/lib/rc_mgp.o $(PXPL5_DIR)/lib/pg_gp.o \
		  -lv -ltracker -lsound -larm -lsdi -lad -lquat \
		  -ljoy -lphive -ljoy -lrc -lhostros -lring -lm

LDFLAGS_5 	= -g -L$(TEST_DIR)/$(HW_OS) -L$(LIB_DIR) -L$(PXPL5_DIR)/lib

INCLUDE_FLAGS_5 = -L$(TEST_DIR) -I$(INCLUDE_DIR) -I$(PXPL5_DIR)/include


#############################################################################
#
# implicit rule for all .c files
#
.c.o:
	$(CC) -c $(CFLAGS) $<




#############################################################################
#
# example code
#
#############################################################################

#
# example
#
EXAMPLE_FILES = example.c interaction.c readstructs.c pobjects.c \
		pc_station.fake.c
EXAMPLE_OBJECTS = $(EXAMPLE_FILES:.c=.o)
EXAMPLE_INCLUDES = example.h pc_station.fake.h


example$(SUFFIX) :
	@$(MAKE) "EXE_NAME = example" \
		"TARGET_INCLUDES = $(EXAMPLE_INCLUDES)" \
		"TARGET_OBJECTS = $(EXAMPLE_OBJECTS)" \
		"PREFIX = " \
		"SUFFIX=space_holder3" \
		doTheRest

#############################################################################
#
# simple code
#
#############################################################################

#
# simple
#
SIMPLE_FILES = simple.c
SIMPLE_OBJECTS = $(SIMPLE_FILES:.c=.o)
SIMPLE_INCLUDES =


simple$(SUFFIX) :
	@$(MAKE) "EXE_NAME = simple" \
		"TARGET_INCLUDES = $(SIMPLE_INCLUDES)" \
		"TARGET_OBJECTS = $(SIMPLE_OBJECTS)" \
		"PREFIX = " \
		"SUFFIX=space_holder3" \
		doTheRest


#############################################################################
#
# gory details:
#
#   the following rules handle most of the work of making an 
#   application for any architecture.
#
#  Explanation:
#
#   initially, PREFIX is defined to be some non-empty string so that the
#	rule $(PREFIX)$(EXE_NAME) is not defined at the same time that
#	the rule at the top with the same name is.  For example, for
#	"example":  example is defined at the top, with a rule that spawns off
#	make "blah blah" example ("example" is the eventual target, via the 
#	EXE_NAME macro and the calls to "doTheRest" and "mainTarget").
#	If there were no PREFIX and SUFFIX, one of the invocations of "make"
#	would see two rules for "example".  By setting PREFIX initially to
#	be nonblank, the second rule for example is covered up initially, 
#	and by setting SUFFIX in the first rule, the first rule is covered
#	up thereafter.
#
#   finally, the library rule is different, since the target name "lib" is
#	different from the actual executable to be made, so we leave
#	PREFIX and SUFFIX unchanged for that one.
#	
#
#############################################################################

doTheRest :
	@( if [ $(HW_OS) = "vax_ultrix" ] ; \
	   then \
	   	$(MAKE) \
		"TARGET_INCLUDES = $(TARGET_INCLUDES)" \
		"TARGET_OBJECTS = $(TARGET_OBJECTS)" \
		"PREFIX = $(PREFIX)" \
		"SUFFIX = $(SUFFIX)" \
		"EXE_NAME = $(EXE_NAME)" \
		mainTarget ; \
	   else \
	   	$(MAKE) \
		"TARGET_INCLUDES = $(TARGET_INCLUDES)" \
		"TARGET_OBJECTS = $(TARGET_OBJECTS)" \
		"INCLUDE_FLAGS = $(INCLUDE_FLAGS_5)" \
		"LDFLAGS = $(LDFLAGS_5)" \
		"LIBS = $(LIBS_5)" \
		"PREFIX = $(PREFIX)" \
		"SUFFIX = $(SUFFIX)" \
		"EXE_NAME = $(EXE_NAME)" \
		mainTarget ; \
	   fi )


PREFIX = space_holder1
TARGET_OBJECTS = space_holder2

mainTarget :
	@echo "Moving objects from $(OBJECT_DIR) to current directory..."
	@-/bin/rm -f $(EXE_NAME) *.o *.a 2> /dev/null
	@-/bin/mv -f $(OBJECT_DIR)/$(EXE_NAME) $(OBJECT_DIR)/*.o \
		$(OBJECT_DIR)/*.a . 2> /dev/null
	@echo "Making $(EXE_NAME)..."
	@-$(MAKE) \
		"TARGET_INCLUDES = $(TARGET_INCLUDES)" \
		"TARGET_OBJECTS = $(TARGET_OBJECTS)" \
		"INCLUDE_FLAGS = $(INCLUDE_FLAGS)" \
		"LDFLAGS = $(LDFLAGS)" \
		"LIBS = $(LIBS)" \
		"PREFIX = $(PREFIX)" \
		"SUFFIX = $(SUFFIX)" \
		$(EXE_NAME)
	@echo "Moving objects back to $(OBJECT_DIR) subdir..."
	@-/bin/mv -f $(EXE_NAME) *.[oa] $(OBJECT_DIR) 2> /dev/null
	@echo "Done.  Executable (if any) is in $(OBJECT_DIR)/$(EXE_NAME)."

$(PREFIX)$(EXE_NAME) : $(TARGET_OBJECTS)
	$(CC) -o $(EXE_NAME) $(TARGET_OBJECTS) $(LDFLAGS) $(LIBS)

$(TARGET_OBJECTS) : $(TARGET_INCLUDES)


#############################################################################
#
# rule for making run-scripts
#
# this rule prompts for an application name and creates a script to run it
#	called <appName>.sh
#
#############################################################################

script :
	@( echo -n 'Enter application name: ' ; \
	  read appName ; \
	  echo '#!/bin/sh' > $$appName.sh ; \
	  echo './`/usr/local/bin/hw_os`/'$$appName '$$*' >> $$appName.sh ; \
	  chmod a+x $$appName.sh )


restore :	  
	@echo "Moving objects back to $(OBJECT_DIR) subdir..."
	@-/bin/mv -f $(EXE_NAME) *.[oa] $(OBJECT_DIR) 2> /dev/null


#############################################################################
#
# rule for making subdirs of the appropriate name
#
#############################################################################

subdir :
	@mkdir $(HW_OS)
	@echo "Made subdir '$(HW_OS)'."


#############################################################################
#
# misc rules
#

up :
	@echo "Moving binaries up from $(HW_OS) directory..."
	@-mv -f $(HW_OS)/*.[oa] . 2> /dev/null

down :
	@echo "Moving binaries down to $(HW_OS) directory..."
	@-mv -f *.[oa] $(HW_OS) 2> /dev/null


lint : 
	@( if [ $(HW_OS) = "vax_ultrix" ] ; \
	   then \
	   	$(MAKE) \
		"TARGET_INCLUDES = $(TARGET_INCLUDES)" \
		"TARGET_OBJECTS = $(TARGET_OBJECTS)" \
		"PREFIX = $(PREFIX)" \
		"SUFFIX = $(SUFFIX)" \
		"TARGET = $(EXE_NAME)" \
		doLint ; \
	   else \
	   	$(MAKE) \
		"TARGET_INCLUDES = $(TARGET_INCLUDES)" \
		"TARGET_OBJECTS = $(TARGET_OBJECTS)" \
		"INCLUDE_FLAGS = $(INCLUDE_FLAGS_5)" \
		"LDFLAGS = $(LDFLAGS_5)" \
		"LIBS = $(LIBS_5)" \
		"PREFIX = $(PREFIX)" \
		"SUFFIX = $(SUFFIX)" \
		"EXE_NAME = $(EXE_NAME)" \
		doLint ; \
	   fi )

doLint :
	lint $(LINT_FLAGS) $(EXAMPLE_FILES) | more

# rcs control
RCS_TMP = .rcs.tmp
RCS_FILES = $(LIB_INCLUDES) $(EXAMPLE_FILES) $(EXAMPLE_INCLUDES) \
		$(makefile)

#
# this ugly rule is a hack to get around the constant prompting "re-use the 
#   same log message?" by rcs
#
# we cat messge into a temp file, cat a quoted version into a shell var
#  (eval evaluates what's in the quotes), then use a quoted version as
#  an arg to "ci"
#
ci:
	@echo rcs files = $(RCS_FILES)
	@echo "Enter log message for ALL files (terminate with ^D):"
	@cat > $(RCS_TMP)
	@echo 'Doing check-in;  this may take a while...'
	@-( eval msg='`cat $(RCS_TMP)`'; \
		/usr/local/bin/ci -f -u -q -m"$$msg" $(RCS_FILES) )
	@echo 'Done.'
	@/bin/rm -f $(RCS_TMP)

# check out
co :
	@echo "Checking out all files..."
	@-co -l -q $(RCS_FILES)
	@echo "Done."

# check in a copy, then check out again
cio : 
	@$(MAKE) ci
	@$(MAKE) co
		

clean :
	/bin/rm -f *.o *.a *~ *.j foo stack.gp gpdump* a.out \
		$(OBJECT_DIR)/example  $(OBJECT_DIR)/simple


allclean :
	$(MAKE) clean
	/bin/rm -f $(OBJECT_DIR)/*
	




