HXCFEBASE=

HXCFEINCLUDES=$(HXCFEBASE)/libhxcfe/trunk/sources/
HXCFELIBS=$(HXCFEBASE)/libhxcfe/trunk/build/

VPATH = ../
HDR = ../

OBJS = qltools.o hxcfe.o
CFLAGS = -O3 -I$(HDR) -I$(HXCFEINCLUDES) -Wall -s
TARGET = qltools$(EXE_EXT)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) -L$(HXCFELIBS) $(OBJS) -lhxcfe

linux.o : linux.c 
	$(CC) -c $(CFLAGS) linux.c

$(OBJS) : $(HDR)/qltools.h

