# this is a gmake file

HW_OS := $(shell /usr/local/bin/hw_os)

# check if its for pxfl
ifdef PBASE_ROOT
  HW_OS := hp_flow
  ifeq ($(PXFL_COMPILER), aCC)
    HW_OS = hp_flow_aCC
  endif
endif

CC := g++
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

CFLAGS = -g -I../ -g -I/afs/unc/proj/hmd/beta/include
LFLAGS = -L../$(HW_OS) \
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



all:  vrpn_server

.PHONY:
vrpn_server:	$(OBJ_DIR)/vrpn_server
	@echo Done.
# can't figure out a gmake trick;  PHONY doesn't do it.

.PHONY:	sample_server
sample_server:	$(OBJ_DIR)/sample_server

.PHONY:
forward:	$(OBJ_DIR)/forward
	@echo Done.
# can't figure out a gmake trick;  PHONY doesn't do it.

.PHONY:
text:	$(OBJ_DIR)/text
	@echo Done.
# can't figure out a gmake trick;  PHONY doesn't do it.

# client:		$(OBJ_DIR)/client
# player:		$(OBJ_DIR)/player
# player: vrpn_player.o
# 	$(CC) $(LFLAGS) -o player vrpn_player.o -lvrpnserver -lsdi -lm
# $(OBJ_DIR)/client: $(OBJ_DIR) $(OBJ_DIR)/client.o	
# 	$(CC) $(LFLAGS) -o $(OBJ_DIR)/client $(OBJ_DIR)/client.o -lvrpn -lsdi 

$(OBJ_DIR)/vrpn_server: $(OBJ_DIR) $(OBJ_DIR)/vrpn.o  $(OBJ_DIR)/midi.o \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/vrpn_server \
		$(OBJ_DIR)/vrpn.o $(OBJ_DIR)/midi.o \
		-lvrpnserver $(GL) -lquat -lm

$(OBJ_DIR)/forward: $(OBJ_DIR) $(OBJ_DIR)/forward.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/forward \
		$(OBJ_DIR)/forward.o $(OBJ_DIR)/midi.o \
		-lvrpnserver -lm

$(OBJ_DIR)/text: $(OBJ_DIR) $(OBJ_DIR)/text.o  \
			 ../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/text \
		$(OBJ_DIR)/text.o $(OBJ_DIR)/midi.o \
		-lvrpnserver -lm

$(OBJ_DIR)/sample_server:	$(OBJ_DIR) $(OBJ_DIR)/sample_server.o \
			../$(HW_OS)/libvrpnserver.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sample_server \
		$(OBJ_DIR)/sample_server.o \
		-lvrpnserver -lm


$(OBJ_DIR):
	-mkdir $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/vrpn_server $(OBJ_DIR)/forward \
	$(OBJ_DIR)/text $(OBJ_DIR)/sample_server

$(OBJ_DIR)/midi.o:	midi.c
$(OBJ_DIR)/vrpn.o:	vrpn.C
$(OBJ_DIR)/forward.o:	forward.C
$(OBJ_DIR)/text.o:	text.C
$(OBJ_DIR)/sample_server.o:	sample_server.C






