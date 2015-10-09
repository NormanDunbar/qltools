HDR = ../

OBJS = qltools.o linux.o
# Added -fno-strict-aliasing to avoid type punning warnings. ND09102015.
CFLAGS = -O3 -I$(HDR) -Wall -s -fno-strict-aliasing
TARGET = qltools$(EXE_EXT)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

linux.o : linux.c 
	$(CC) -c $(CFLAGS) linux.c

qltools.o : ../qltools.c
	$(CC) -c $(CFLAGS) $<

$(OBJS) : $(HDR)/qltools.h

