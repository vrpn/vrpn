# Couldn't understand or usefully modify the former makefile.
# Copied the one from ../server_src here and put the former one
# in Makefile.old

HW_OS := $(shell hw_os)

CC := g++
ARCH_LIBS :=

ifeq ($(HW_OS),pc_linux)
  CC := g++
endif
ifeq ($(HW_OS),sgi_irix)
  CC := CC -32
endif
ifeq ($(HW_OS),hp700_hpux10)
  CC := CC +a1
endif

ifeq ($(HW_OS),sparc_solaris)
  ARCH_LIBS := -lsocket -lnsl
endif

OBJ_DIR := $(HW_OS)

CFLAGS = -I../ -g
LFLAGS = -L../`hw_os` \
	 -L/afs/unc/proj/hmd/beta/lib/`hw_os` \
         -L/afs/unc/proj/hmd/lib/`hw_os`

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.C.o:
	$(CC) $(CFLAGS) -c $*.C

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: %.C
	$(CC) $(CFLAGS) -o $@ -c $<


all:	$(OBJ_DIR)/sound_client $(OBJ_DIR)/printvals \
	$(OBJ_DIR)/checklogfile $(OBJ_DIR)/logfilesenders \
	$(OBJ_DIR)/logfiletypes

$(OBJ_DIR)/sound_client: $(OBJ_DIR)/sound_client.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/sound_client \
		$(OBJ_DIR)/sound_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/midi_client: $(OBJ_DIR)/midi_client.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/midi_client \
		$(OBJ_DIR)/midi_client.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/printvals: $(OBJ_DIR)/printvals.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/printvals \
		$(OBJ_DIR)/printvals.o -lvrpn $(ARCH_LIBS)

$(OBJ_DIR)/listen_client: $(OBJ_DIR)/listen_client.o ../`hw_os`/libvrpn.a
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/listen_client \
		$(OBJ_DIR)/listen_client.o -lvrpn $(ARCH_LIBS)

# These utilities actually DON'T use libvrpn

$(OBJ_DIR)/checklogfile: $(OBJ_DIR)/checklogfile.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/checklogfile $(OBJ_DIR)/checklogfile.o

$(OBJ_DIR)/logfilesenders: $(OBJ_DIR)/logfilesenders.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfilesenders \
		$(OBJ_DIR)/logfilesenders.o

$(OBJ_DIR)/logfiletypes: $(OBJ_DIR)/logfiletypes.o
	$(CC) $(LFLAGS) -o $(OBJ_DIR)/logfiletypes $(OBJ_DIR)/logfiletypes.o


clean:
	rm -f *.o sound_client midi_client printvals \
	checklogfile logfilesenders logfiletypes \
	$(OBJ_DIR)/*

../`hw_os`/libvrpn.a:


