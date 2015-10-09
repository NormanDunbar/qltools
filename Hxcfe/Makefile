HXCFEBASE=

HXCFEINCLUDES=$(HXCFEBASE)/libhxcfe/trunk/sources/
HXCFELIBS=$(HXCFEBASE)/libhxcfe/trunk/build/

HDR = ../

OBJS = qltools.o hxcfe.o
CFLAGS = -O3 -I$(HDR) -I$(HXCFEINCLUDES) -Wall -s -fno-strict-aliasing
TARGET = qltools-hxcfe$(EXE_EXT)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) -L$(HXCFELIBS) $(OBJS) -lhxcfe -lpthread

linux.o : linux.c 
	$(CC) -c $(CFLAGS) linux.c

qltools.o : ../qltools.c
	$(CC) -c $(CFLAGS) $<

$(OBJS) : $(HDR)/qltools.h

