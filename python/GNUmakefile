
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

#PYTHON_VERSION := 2.7
#PYTHON_VERSION := 3.2
PYTHON_CONFIG := python$(PYTHON_VERSION)-config
PYTHON_MODULE_DIR := $(shell $(PYTHON_CONFIG) --exec-prefix)/lib/python$(PYTHON_VERSION)/lib-dynload

CXXFLAGS := $(shell $(PYTHON_CONFIG) --includes) -I ./ -I ../  -fPIC -DCALLBACK_CALL=
LD_FLAGS := $(shell $(PYTHON_CONFIG) --ldflags) -L ../$(HW_OS) -L ../quat/$(HW_OS)
LIBSUP   := $(shell $(PYTHON_CONFIG) --libs) -lvrpn -lquat -fPIC

SRCS=interface.cpp callback.cpp tools.cpp Base.cpp Device.cpp Connection.cpp
SRCS+=receiver/receiver.cpp receiver/Tracker.cpp receiver/Analog.cpp receiver/Button.cpp receiver/Dial.cpp receiver/Text_Receiver.cpp
SRCS+=sender/sender.cpp sender/Poser.cpp sender/Text_Sender.cpp
SRCS+=quaternion/quaternion.cpp
OBJS=$(SRCS:%.cpp=$(HW_OS)/%.o)

MODULE = $(HW_OS)/vrpn.so

COMPILE_PATH=$(HW_OS)/receiver $(HW_OS)/sender $(HW_OS)/quaternion

all: $(MODULE)

$(MODULE): $(OBJS)
	$(CXX) -shared $(LD_FLAGS) -o $@ $(OBJS) $(LIBSUP)

$(HW_OS)/%.o: %.cpp
	mkdir -p $(shell dirname $@)/.dir
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.C: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -E $<

clean:
	rm -Rf $(HW_OS)
	find . -name "*~" -delete

depend:
	@makedepend -- $(CXXFLAGS) $(SRCS) 2> /dev/null

install: $(MODULE)
	-mkdir -p $(PYTHON_MODULE_DIR)
	( cd $(PYTHON_MODULE_DIR) ; rm -f vrpn.so )
	( cd $(HW_OS) ; cp vrpn.so $(PYTHON_MODULE_DIR) )
