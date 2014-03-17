VPATH = ../
HDR = ../
# qltools
# For the w95/NT mingw32 (gcc) compiler

CC=gcc

OBJS = qltools.o nt.o

CFLAGS = -O2 -s -I$(HDR) -D__NT__ -D__WIN32__

qltools: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

nt.o : nt.c 
	$(CC) -c $(CFLAGS) nt.c

qltools.o : qltools.c 
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o *.exe
