HDR = ../

OBJS = qltools.o linux.o
CFLAGS = -O3 -I$(HDR) -Wall -s
TARGET = qltools$(EXE_EXT)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

linux.o : linux.c 
	$(CC) -c $(CFLAGS) linux.c

$(OBJS) : $(HDR)/qltools.h

