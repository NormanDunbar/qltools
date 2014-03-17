# qltools

OBJS = qltools.obj,vms.obj
CFLAGS = /INCLUDE=[]
LD = LINK

qltools : qltools.exe
    @ continue

qltools.exe : $(OBJS)
	$(LD)/exec=qltools $(OBJS)

vms.o : [.vms]vms.c 
	$(CC) -c $(CFLAGS) [.vms]vms.c

$(OBJS) : qltools.h

