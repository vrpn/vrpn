
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
# Try this to cross-compile on a Cygwin PC for an ARM embedded controller.
#HW_OS := pc_cygwin_arm
#HW_OS := pc_cygwin
#HW_OS := pc_FreeBSD
#HW_OS := sparc_solaris
#HW_OS := sparc_solaris_64
#HW_OS := powerpc_aix
#HW_OS := powerpc_macosx
#HW_OS := universal_macosx
##########################

##########################
# Mac OS X-specific options. If HW_OS is powerpc_macosx or universal_macosx,
# uncomment one line below to choose the minimum targeted OS X version and
# corresponding SDK. If none of the lines below is commented out, 10.5 will
# be the minimum version.
##########################
#MAC_OS_MIN_VERSION := 10.4
#MAC_OS_MIN_VERSION := 10.5
#MAC_OS_MIN_VERSION := 10.6

EXEC_NAME=vrpn_GTK
GTK_BUILDER_FILE := vrpn_GTK.xml

CXXFLAGS := $(shell pkg-config gtk+-3.0 --cflags) -I ./ -I ../ -I ./include -I ../quat -g -DGTK_BUILDER_FILE=$(GTK_BUILDER_FILE)
LD_FLAGS := $(shell pkg-config gtk+-3.0 --libs-only-L) -L ../$(HW_OS) -L ../quat/$(HW_OS) -g
LIBSUP   := $(shell pkg-config gtk+-3.0 --libs-only-l) -lvrpn -lquat

INSTALL_DIR := /usr/local
BIN_DIR := $(INSTALL_DIR)/bin
ETC_DIR := $(INSTALL_DIR)/etc

SRCS=main.cpp tracker_GTK.cpp analog_GTK.cpp button_GTK.cpp
OBJS=$(SRCS:%.cpp=$(HW_OS)/%.o)

NAME = $(HW_OS)/$(EXEC_NAME)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(LD_FLAGS) -o $@ $(OBJS) $(LIBSUP)

$(HW_OS)/%.o: %.cpp
	mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.C: %.cpp
	$(CXX) $(CXXFLAGS) ??? -o $@ -E $<

clean:
	rm -Rf $(HW_OS)
	find . -name "*~" -delete

depend:
	@makedepend -- $(CXXFLAGS) $(SRCS) 2> /dev/null

install: $(NAME)
	-mkdir -p $(BIN_DIR)
	( cd $(BIN_DIR) ; rm -f $(EXEC_NAME) )
	( cp $(NAME) $(BIN_DIR) )
	-mkdir -p $(ETC_DIR)
	cp $(GTK_BUILDER_FILE) $(ETC_DIR)
