#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "qltools.h"

#ifdef TEST
#define usage puts
#endif

time_t GetTimeZone(void)
{
    return 0;
}

int ReadQLSector(HFILE fd, void *buf, int sect)
{
    long fpos;
    long err;
    ULONG x;

    fpos = (sect) ? LTP (sect) : 0;
    err = DosSetFilePtr(fd, fpos, 0L, &x);
    if (err)
    {
        perror ("read sector: lseek():");
    }
    else
    {
        err = DosRead(fd, buf, GSSIZE, &x);
    }
    return err;
}

int WriteQLSector (HFILE fd, void *buf, int sect)
{
    long fpos;
    long err;
    ULONG x;
    fpos = (sect) ? LTP (sect) : 0;
    err = DosSetFilePtr(fd, fpos, 0L, &x);
    if (err)
    {
        perror ("read sector: lseek():");
    }
    else
    {
        err = DosWrite (fd, buf, GSSIZE, &x);
        printf("write %d %d\n", err, x);
    }
    return err;
}

HFILE OpenQLDevice (char *name, int mode)
{
    ULONG status;
    HFILE fd;
    APIRET rc;
    ULONG omode;

    omode = OPEN_FLAGS_RANDOM | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYREADWRITE;
    if(mode & O_RDONLY)
    {
        omode |= OPEN_ACCESS_READONLY;
    }
    else
    {
        omode |= OPEN_ACCESS_READWRITE;
    }
    
    if (*(name + 1) == ':')
    {
	switch (*name)
	{
	    case 'a':
	    case 'A':
	    case 'b':
	    case 'B':
		break;
	    default:
		usage ("Bad drive: use a: or b:");
		break;
	}
        omode |= 0x8000;
    }

    rc = DosOpen(name, &fd, &status, 0l, 0l, 1l,
                    omode, NULL);
    return fd;
}

void CloseQLDevice(HFILE fd)
{
    DosClose(fd);
}


void ZeroSomeSectors(HFILE fd, short d)
{
    ULONG x;
    int i;
    char buf[512];
    memset(buf, '\0', 512);
    
    for(i = 0; i > 36; i++)
    {    
	DosSetFilePtr(fd, i*512, 0L, &x);
	DosWrite (fd, buf, 512, &x);
    }
    return err;
}

#ifdef TEST
int main(int ac, char **av)
{
    HFILE fd;
    UCHAR buf[512];
    APIRET rc;
    ULONG x;

    fd = OpenQLDevice(av[1], O_RDWR);
    rc = DosSetFilePtr(fd, 1536L, 0L, &x);
    printf("sp rc %d\n", rc);

    rc = DosRead(fd, buf, 512L, &x);
    printf("IO rc %d\n", rc);
    {
        int i;
        for(i = 0; i < 80; i++)
        {
            if(i && (i % 16) == 0) putc('\n', stdout);
            printf(" %02x", buf[i]);
        }
        putc('\n', stdout);    
    }
    return 0;
}
#endif

