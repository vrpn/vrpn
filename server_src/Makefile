# this is a gmake file

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

CC := g++
ifeq ($(HW_OS),pc_cygwin)
	SYSLIBS := -lcygwin -luser32 -lgdi32 -lcomdlg32 -lwsock32
endif
ifeq ($(HW_OS),sparc_solaris)
	SYSLIBS := -lsocket
endif
ifeq ($(HW_OS),sgi_irix)
	CC := CC -32
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


OBJ_DIR := $(HW_OS)

CFLAGS = -g -I../ -I../../quat -I/afs/unc/proj/hmd/beta/include
LFLAGS = -L../$(HW_OS) -L../../quat/$(HW_OS) \
	 -L/afs/unc/proj/hmd/beta/lib/$(HW_OS) \
         -L/afs/unc/proj/hmd/lib/$(HW_OS)

.C.o:
	$(CC) $(CFLAGS) -c $*.C

.c.o:
	$(CC) $(CFLAGS) -c $*.c

$(OBJ_DIR)/%.o:	%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o:	%.C
	$(CC) $(CFLAGS) -o $@ -c $<



all:  vrpn_server test_vrpn client_and_server

.PHONY:	vrpn_server
vrpn_server:	$(OBJ_DIR)/vrpn_server

.PHONY:	test_vrpn
test_vrpn:	$(OBJ_DIR)/test_vrpn

.PHONY:	client_and_server
client_and_server:	$(OBJ_DIR)/client_and_server

.PHONY:	sample_server
sample_server:	$(OBJ_DIR)/sample_server

.PHONY:	sample_analog
sample_analog:	$(OBJ_DIR)/sample_analog

.PHONY:	forward
forward:	$(OBJ_DIR)/forward

.PHONY:	text
text:	$(OBJ_DIR)/text

# client:		$(OBJ_DIR)/client
# player:		$(OBJ_DIR)/player
# player: vrpn_player.o
# 	$(CC) $(LFLAGS) -o player vrpn_player.o -lvrpnserver -lsdi -lm
# $(OBJ_DIR)/client: $(OBJ_DIR) $(OBJ_DIR)/client.o	
# 	$(CC) $(LFLAGS) -o $(OBJ_DIR)/client $(OBJ_DIR)/client.o -lvrpn -lsdi 

$(OBJ_DIR)/vrpn_server: $(OBJ_DIR) $(OBJ_DIR)/vrpn.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_server \
		$(OBJ_DIR)/vrpn.o \
		-lvrpnserver $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/test_vrpn: $(OBJ_DIR) $(OBJ_DIR)/test_vrpn.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/test_vrpn \
		$(OBJ_DIR)/test_vrpn.o \
		-lvrpnserver $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/client_and_server: $(OBJ_DIR) $(OBJ_DIR)/client_and_server.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/client_and_server \
		$(OBJ_DIR)/client_and_server.o \
		-lvrpnserver $(GL) -lquat $(SYSLIBS) -lm

$(OBJ_DIR)/forward: $(OBJ_DIR) $(OBJ_DIR)/forward.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forward \
		$(OBJ_DIR)/forward.o \
		-lvrpnserver -lm

$(OBJ_DIR)/text: $(OBJ_DIR) $(OBJ_DIR)/text.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/text \
		$(OBJ_DIR)/text.o \
		-lvrpnserver -lm

$(OBJ_DIR)/sample_server:	$(OBJ_DIR) $(OBJ_DIR)/sample_server.o \
			../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_server \
		$(OBJ_DIR)/sample_server.o \
		-lvrpnserver -lm

$(OBJ_DIR)/sample_analog:	$(OBJ_DIR) $(OBJ_DIR)/sample_analog.o \
			../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_analog \
		$(OBJ_DIR)/sample_analog.o \
		-lvrpnserver -lm


$(OBJ_DIR):
	-mkdir $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/vrpn_server $(OBJ_DIR)/forward \
	$(OBJ_DIR)/text $(OBJ_DIR)/sample_server

$(OBJ_DIR)/vrpn.o:	vrpn.C
$(OBJ_DIR)/forward.o:	forward.C
$(OBJ_DIR)/text.o:	text.C
$(OBJ_DIR)/sample_server.o:	sample_server.C
$(OBJ_DIR)/sample_analog.o:	sample_analog.C

