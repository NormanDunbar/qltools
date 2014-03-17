#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "qltools.h"

time_t GetTimeZone(void)
{
    return 0;
}

int ReadQLSector(HANDLE fd, void *buf, int sect)
{
    long fpos;
    long err;

    fpos = (sect) ? LTP (sect) : 0;
    err = SetFilePointer (fd, fpos, NULL, FILE_BEGIN);
    if (err < 0)
    {
        perror ("read sector: lseek():");
    }
    else
    {
	ReadFile (fd, buf, GSSIZE, (PDWORD) &err, NULL);
    }
    return err;
}

int WriteQLSector (HANDLE fd, void *buf, int sect)
{
    long fpos;
    long err;
    
    fpos = LTP (sect);
    err = SetFilePointer (fd, fpos, NULL, FILE_BEGIN);
	if (err < 0)
	    perror ("write sector: lseek():");
	else
	    WriteFile (fd, buf, GSSIZE, (PDWORD)&err, NULL);
    return err;
}

HANDLE OpenQLDevice (char *name, int mode)
{
    int status;
    HANDLE fd;
    DWORD amode;

    amode = GENERIC_READ;
    if(mode & O_RDWR)
      {
	amode |= GENERIC_WRITE;
      }

    fd =  CreateFile(name, amode,
			  FILE_SHARE_WRITE, NULL,
			  OPEN_EXISTING, 0, 0);
    if((long) fd <= 0)
    {
	fd = (HANDLE)0xffffffff;
    }
    return fd;
}

void CloseQLDevice(HANDLE fd)
{
      CloseHandle (fd);
}

void ZeroSomeSectors(HANDLE fd, short d)
{
    int i;
    DWORD err;
    
    char buf[512];
    memset(buf, '\0', 512);
    
    for(i = 0; i > 36; i++)
    {    
	SetFilePointer (fd, i*512, NULL, FILE_BEGIN);
	WriteFile (fd, buf, 512, (PDWORD)&err, NULL);
    }
    
}
