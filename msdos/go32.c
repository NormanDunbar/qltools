#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <bios.h>
#include <dos.h>
#include <io.h>
#include <conio.h>

#include "qltools.h"

static short use_fd = 0;

#ifdef __WATCOMC__
ushort biosdisk (ushort service, ushort drive, ushort side, ushort track,
		 ushort sector, ushort nsector, void *buffer)
{
    struct diskinfo_t di;
    ushort status;

    di.drive = drive;
    di.head = side;
    di.track = track;
    di.sector = sector;
    di.nsectors = nsector;
    di.buffer = buffer;
    status = _bios_disk (service, &di);
    return status;
}
#endif

time_t GetTimeZone(void)
{
    return 0;
}

int ReadQLSector(int fd, void *buf, int sect)
{
    long fpos;
    long err;
    if (!use_fd)
    {
	short side;
	short track;
	short sector;
	if(sect)
	{
	    side = LTP_SIDE(sect);
	    track = LTP_TRACK(sect);
	    sector = LTP_SECT(sect);
	}
	else
	{
	    side = track = 0;
	    sector = 1;
	}
	
	if (biosdisk (_DISK_READ, fd, side, track, sector, 1, buf) & 0xff00)
	    err = -1;
    }
    else
    {
	fpos = (sect) ? LTP (sect) : 0;
	err = lseek (fd, fpos, SEEK_SET);
	if (err < 0)
	{
	    perror ("read sector: lseek():");
	}
	else
	{
	    err = read (fd, buf, GSSIZE);
	}
    }
    return err;
}

int WriteQLSector (int fd, void *buf, int sect)
{
    long fpos;
    long err;
    
    if (!use_fd)
    {
	if ((err = biosdisk (_DISK_WRITE, fd, 
			     LTP_SIDE (sect), LTP_TRACK (sect), 
			     LTP_SECT (sect), 1, buf)) & 0xff00)
	    if (err == 0x300)
	    {
		fputs ("Write Protected\n", stderr);
		exit (0);
	    }
	    else
		err = -1;
    }
    else
    {
	err = lseek (fd, LTP (sect), SEEK_SET);
	if (err < 0)
	    perror ("write sector: lseek():");
	else
	    err = write (fd, buf, GSSIZE);
    }
    return err;
}

int OpenQLDevice (char *name, int mode)
{
    int status;
    int fd = -1;
    char buf[512];
    
    if (*(name + 1) == ':')
    {
	switch (*name)
	{
	    case 'a':
	    case 'A':
		fd = 0;
		break;
	    case 'b':
	    case 'B':
		fd = 1;
		break;
	    default:
                if(*(name + 2) == '\\')
                {
                    fd = 2;
                }
		break;
	}
    }
    else fd = 2;
    
    if(fd == 0 || fd == 1)
    {
            /* reset the disk */
        status = biosdisk (_DISK_READ, fd, 0, 10, 1, 1, buf);

        /* change signal ? */
        if (status == 0x06)
	    status = biosdisk (_DISK_READ, fd, 0, 0, 1, 1, buf);

	status = biosdisk (_DISK_RESET, fd, 0, 0, 0, 0, NULL);
	if (status != 0)
        {
	    fprintf (stderr, "Disk not ready (continuing...)\n");
	    fd = -1;
        }
    }
    else if(fd == 2)
    {
	use_fd = 1;
	mode |= O_BINARY;
	fd = open (name, mode);
    }
    return fd;
}

void CloseQLDevice(int fd)
{
    if(use_fd)
    {
	close(fd);
    }
}

void ZeroSomeSectors(int fd, short flag)
{
    int i, t,h,s;
    char buf[512];

    memset(buf, '\0', 512);
    if(flag == 'd')
    {
	flag = 9;
    }
    else
    {
	flag = 18;
    }
    
    for(i = t = 0; ; t++)
    {
	for(h = 0; h < 2; h++)
	{
	    for(s = 0; s < flag; s++)
	    {
		biosdisk (_DISK_WRITE, fd, h, t, s, 1, buf);
		i++;
		if(i == 36) return;
	    }
	}
    }
    
}
