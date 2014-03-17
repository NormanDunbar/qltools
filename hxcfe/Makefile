HXCFEBASE=

HXCFEINCLUDES=$(HXCFEBASE)/libhxcfe/trunk/sources/
HXCFELIBS=$(HXCFEBASE)/libhxcfe/trunk/build/

VPATH = ../
HDR = ../

# Need to be root to do this !
INSTALL = install -s -g floppy -o root -m 02755
#INSTALL = cp

OBJS = qltools.o hxcfe.o
CFLAGS = -ggdb -O0 -I$(HDR) -I$(HXCFEINCLUDES) -L$(HXCFELIBS) -Wall

qltools : $(OBJS)
	$(CC) $(CFLAGS) -o qltools $(OBJS) -lhxcfe -lpthread
#	strip qltools

linux.o : hxcfe.c 
	$(CC) -c $(CFLAGS) hxcfe.c

$(OBJS) : qltools.h

install: qltools
	$(INSTALL) qltools /usr/local/bin/

