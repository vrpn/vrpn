
#############################################################################
# 
# makefile for quaternion lib
#
#############################################################################

#
# common definitions
#

CC = cc
CFLAGS = -g $(INCLUDE_FLAGS)
OPT_CFLAGS = -O $(INCLUDE_FLAGS)
MAKEFILE = makefile

HW_OS = `/usr/local/bin/hw_os`
QUAT_LIB = libquat.a

# dirs
HMD_DIR = /afs/cs.unc.edu/proj/hmd
LIB_DIR = $(HMD_DIR)/lib/$(HW_OS)
INCLUDE_DIR = $(HMD_DIR)/include
BETA_DIR = $(HMD_DIR)/beta
BETA_LIB_DIR = $(BETA_DIR)/lib/$(HW_OS)
BETA_INCLUDE_DIR = $(BETA_DIR)/include

# flags
INCLUDE_FLAGS = -I. -I$(INCLUDE_DIR)

LDFLAGS = -L. -L$(LIB_DIR)
LINT_FLAGS = $(INCLUDE_FLAGS)


#############################################################################
#
# for building library
#
#############################################################################

lib:	libquat.a



#############################################################################
#
# example/test programs
#
#############################################################################

TEST_FILES = eul qmat qmult qxform qmake timer qpmult

all :
	rm $(TEST_FILES)
	$(MAKE) $(TEST_FILES)

#
# timer- time some operation
#
timer : timer.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# eul- eul to quat
#
eul : eul.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# qmult- multiply 2 quats
#
qmult : qmult.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# qpmult- multiply 2 pmatrices
#
qpmult : qpmult.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# qxform- xform a vec
#
qxform : qxform.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# qmat- matrix to quaternion 
#
qmat : qmat.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#
# qmake- tests q_make
#
qmake : qmake.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

oglconv : oglconv.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

oglmult : oglmult.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $@.c -lquat -lm

#############################################################################
#
# rules and definitions for making quaternion library
#
#############################################################################

QUAT_INCLUDES = quat.h pdefs.h
QUAT_C_FILES = quat.c matrix.c vector.c xyzquat.c
QUAT_OBJ_FILES = $(QUAT_C_FILES:.c=.o)

$(QUAT_LIB) :  $(QUAT_OBJ_FILES) $(MAKEFILE)
	@echo "Building $@..."
	ar ruv $(QUAT_LIB) $(QUAT_OBJ_FILES)
	-ranlib $(QUAT_LIB)


$(QUAT_OBJ_FILES) : $(QUAT_INCLUDES) pdefs.h



#############################################################################
#
# miscellaneous rules
#
#############################################################################


lint :
	lint $(LINT_FLAGS) $(COORD_FILES)


# rcs control
RCS_TMP = .rcs.tmp
RCS_FILES = $(QUAT_C_FILES) $(QUAT_INCLUDES)

#
# this ugly rule is a hack to get around the constant prompting "re-use the 
#   same log message?"
#
# we cat messge into a temp file, cat a quoted version into a shell var
#  (eval evaluates what's in the quotes), then use a quoted version as
#  an arg to "ci"
#
ci:
	@echo "Enter log message for ALL files (terminate with ^D):"
	@cat > $(RCS_TMP)
	@echo 'Doing check-in;  this may take a while...'
	@( eval msg='`cat $(RCS_TMP)`'; \
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

allclean :
	-/bin/rm -f *.o *.a *~ *.j foo a.out $(TEST_FILES)

clean :
	-/bin/rm -f *.o *.a *~ *.j foo a.out $(TEST_FILES)


# install into lib dir
#  do rm -f followed by mv here because mv -f in this instance causes
#    a seg fault on the sun.
install : 
	$(MAKE) clean
	$(MAKE) "CFLAGS=$(OPT_CFLAGS)" $(QUAT_LIB)
	rm -f $(LIB_DIR)/$(QUAT_LIB)
	mv $(QUAT_LIB) $(LIB_DIR)
	-ranlib $(LIB_DIR)/$(QUAT_LIB)
	@(echo -n "Replace includes? [yn] " ; \
	  read ans ; \
	  if [ $$ans = "y" ] ; \
	  then \
		( echo "includes = $(QUAT_INCLUDES)" ; \
		  echo "Moving old includes to 'old' subdirectory..." ; \
		  cd $(INCLUDE_DIR); \
		  for fn in $(QUAT_INCLUDES) ; \
		  do \
		    echo -n $$fn  ; \
		    if [ -f $$fn ] ; \
		    then \
			echo -n '  - moved' ; \
			/bin/mv -f $$fn old ; \
		    fi ; \
		    echo '' ; \
		  done ) ; \
		echo "Copying new includes..." ; \
		cp $(QUAT_INCLUDES) $(INCLUDE_DIR) ; \
	  else \
	  	echo 'Includes not replaced.' ; \
	  fi )
	$(MAKE) clean


# install into beta lib dir
beta : 
	$(MAKE) $(QUAT_LIB)
	-mv $(QUAT_LIB) $(BETA_LIB_DIR)
	-ranlib $(BETA_LIB_DIR)/$(QUAT_LIB)
	-( cd $(BETA_INCLUDE_DIR); \
		/bin/rm -f $(QUAT_INCLUDES) )
	cp $(QUAT_INCLUDES) $(BETA_INCLUDE_DIR) 


