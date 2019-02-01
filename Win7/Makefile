# This makefile is for the GCC compiler which you can install
# as a standalone Windows executable, or, as part of the free
# CodeBlocks IDE system.
#
# To use it, change up one level and run make against the Makefile
# there. It will call down here if you:
#
#     make win7
#
# and will pass the correct executable extension to this makefile.
#
# To call it standalone, run:
#
#     make EXE_EXT=.exe 
#

HDR = ../

CC = gcc

OBJS = qltools.o win7.o
CFLAGS = -O3 -I$(HDR) -Wall -s
TARGET = qltools$(EXE_EXT)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

win7.o : win7.c $(HDR)/qltools.h
	$(CC) -c $(CFLAGS) win7.c

qltools.o : ../qltools.c $(HDR)/qltools.h
	$(CC) -c $(CFLAGS) $<

clean:
	@del *.o
	@del *.obj
	@del *.tds   
	@del qltools.exe


