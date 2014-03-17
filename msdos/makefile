VPATH = ../
HDR = ../

# qltools
# For the Linux dos-gcc cross compiler

CC=dos-gcc

OBJS = qltools.o go32.o

CFLAGS = -O2 -s -I$(HDR)

sqltools:	$(OBJS)
	$(CC) -s -o qltools $(OBJS)

go32.o : go32.c 
	$(CC) -c $(CFLAGS) go32.c

qltools.o : qltools.c 
	$(CC) -c $(CFLAGS) -o $@ $<
