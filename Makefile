#
# File: Makefile
# JACK NetUMP MIDI daemon for Zynthian
# Author: Oscar Aceña
#

TARGET  = jacknetumpd
OBJECTS = \
	$(TARGET).o \
	Endpoint.o \
	UMP_mDNS.o \
	UMP_Transcoder.o \
	NetUMP_SessionProtocol.o \
	NetUMP.o \
	SystemSleep.o \
	network.o \


CXXFLAGS = \
	-O2 -Wall -fexceptions -D__TARGET_LINUX__ \
	-Ilibs/NetUMP -Ilibs/BEBSDK

LDLIBS = -ljack

vpath %.cpp libs/NetUMP libs/BEBSDK
vpath %.c libs/NetUMP


all: $(TARGET)

debug: CXXFLAGS += -g
debug: all

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	$(RM) -frv *.o $(TARGET)

## Other helper rules

run:
	./$(TARGET)

run-with-gdb:
	gdb \
		-ex 'set print pretty on' \
		-ex run \
		$(TARGET)

package:
	ian build -c

package-clean: clean
	ian clean
