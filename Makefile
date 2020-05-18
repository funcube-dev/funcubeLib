# Build funcubelib

# Dependencies to install (all standard Debian/Raspbian packages):
# Audio: libfftw3-dev libusb-1.0-0-dev portaudio19-dev libasound2-dev
# GTK+3: libgtk-3-dev
#
# Cross-compile support prefix
# - set CCPREFIX=<e.g. arm-linux-gnueabi-> in the environment or on command line, binaries built into
#   $(CCPREFIX)bin
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

CC=$(CCPREFIX)gcc
CCP=$(CCPREFIX)g++ -std=c++14
AFLAGS=$(shell pkg-config --cflags fftw3f libusb-1.0 portaudio-2.0) -fPIC -DLINUX -D_LINUX
SFLAGS=-fPIC -DLINUX -D_LINUX
CFLAGS=-fPIC -DLINUX -D_LINUX -Icodec2/codec2
LDFLAGS=-Wl,--MACHINE:x64,-rpath,\$$ORIGIN,--enable-new-dtags
LIBS=$(shell pkg-config --libs fftw3f libusb-1.0 portaudio-2.0) -lpthread -lrt -fPIC

BINS=$(CCPREFIX)bin

ALIB=-L $(BINS) -lfuncubelib

AUDIO_SRCS=common/funcubeLib.cpp \
        common/bpskDecoder.cpp \
        common/codecAO40.cpp \
        common/decode.cpp \
        common/decodeManager.cpp \
        common/decodeWorker.cpp \
        common/dongle.cpp \
        common/encode.cpp \
        common/encodeWorker.cpp \
        common/fec.cpp \
        common/fft.cpp \
        common/firFilter.cpp \
        common/memPool.cpp \
        common/oscillator.cpp \
        common/overlappedFft.cpp \
        common/peakDetect.cpp \
        common/peakDetectWorker.cpp \
        common/rollingAverage.cpp \
        common/stopWatch.cpp \
        common/sampleStopWatch.cpp \
        common/viterbi.cpp

AUDIO_OBJS=$(AUDIO_SRCS:common/%.cpp=$(BINS)/%.o) $(BINS)/hid-libusb.o $(BINS)/fcd.o

.PHONY: default all clean

# Default target
default: funcubelibstatic
default: CC += -O3
default: CCP += -O3

all: funcubelib funcubelibstatic

funcubelib: $(BINS) $(BINS)/libfuncube.so
funcubelibstatic: $(BINS) $(BINS)/libfuncube.a

clean:	
	-rm -f $(BINS)/*.o $(BINS)/*.so $(BINS)/*.a
	-rmdir $(BINS) 

install: default
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BINS)/libfuncube.a $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/funcubelib/
	install -m 644 common/funcubeLib.h $(DESTDIR)$(PREFIX)/include/funcubelib/
	install -m 644 common/wintypes.h $(DESTDIR)$(PREFIX)/include/funcubelib/

uninstall: default
	rm $(DESTDIR)$(PREFIX)/lib/libfuncube.a
	rm $(DESTDIR)$(PREFIX)/include/funcubelib/funcubeLib.h
	rm $(DESTDIR)$(PREFIX)/include/funcubelib/wintypes.h
	rm -r $(DESTDIR)$(PREFIX)/include/funcubelib

$(BINS):
	mkdir $(BINS)

$(BINS)/libfuncube.so: $(AUDIO_OBJS)
	$(CCP) -o $@ -shared $(AUDIO_OBJS) $(LIBS)

$(BINS)/libfuncube.a: $(AUDIO_OBJS)
	$(CC_PREFIX)ar -r $@ $(AUDIO_OBJS)

$(BINS)/%.o: common/%.cpp
	$(CCP) -o $@ $(AFLAGS) -c $<

$(BINS)/%.o: common/%.c
	$(CC) -o $@ $(AFLAGS) -c $<
