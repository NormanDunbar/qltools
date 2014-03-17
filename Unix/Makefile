VPATH = ../
HDR = ../

# Need to be root to do this !
INSTALL = install -s -g floppy -o root -m 02755
#INSTALL = cp

OBJS = qltools.o linux.o
CFLAGS = -O3 -I$(HDR) -Wall -s

qltools : $(OBJS)
	$(CC) $(CFLAGS) -o qltools $(OBJS)
#	strip qltools

linux.o : linux.c 
	$(CC) -c $(CFLAGS) linux.c

$(OBJS) : qltools.h

install: qltools
	$(INSTALL) qltools /usr/local/bin/

