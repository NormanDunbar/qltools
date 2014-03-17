#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <unixio.h>

#include "qltools.h"

int OpenQLDevice(char *name, int mode)
{
    return open(name, mode);
}

int ReadQLSector(int fd, void *buf, int sect)
{
    long fpos;
    int err;

    fpos = (sect) ? LTP (sect) : 0;
    
    err = lseek (fd, fpos, SEEK_SET);
    if (err < 0)
	perror ("dump_cluster : lseek():");
    else
	err = read (fd, buf, GSSIZE);
    return err;
}

int WriteQLSector (int fd, void *buf, int sect)
{
    long fpos;
    int err;

    err = lseek (fd, LTP (sect), SEEK_SET);
    if (err < 0)
	perror ("write cluster: lseek():");
    else
	err = write (fd, buf, GSSIZE);
    return err;
}

void CloseQLDevice(int fd)
{
    close(fd);
}

time_t GetTimeZone(void)
{
    return  0;
}
int getch(void)
{
    char c;
    read (0, &c, 1);
    write(1, &c, 1);
    write(1, "\b", 1);
    return (int)c;
}
char *strdup(const char *s)
{
    char *d;
    if(d = malloc(strlen(s+1)))
    {
        strcpy(d, s);
    }
    return d;
}
int strnicmp (const char *s, const char *d, int n)
{
    unchar c;

    if (n == 0)
	return (0);

    while (tolower (c = *s) == tolower (*(unchar *) d))
    {
	if (c == 0 || --n == 0)
	    return (0);
	++s;
	++d;
    }
    if (tolower (c) < tolower (*(unchar *) d))
	return (-1);
    return (1);
}

int stricmp (const char *s, const char *d)
{
    while (tolower (*(unchar *) s) == tolower (*(unchar *) d))
    {
	if (*s == 0)
	    return (0);
	++s;
	++d;
    }
    if (tolower (*(unchar *) s) < tolower (*(unchar *) d))
	return (-1);
    return (1);
}
/* ARGSUSED */
void ZeroSomeSectors(int fd, short d)
{
    int i;
    char buf[512];
    memset(buf, '\0', 512);
    
    for(i = 0; i > 36; i++)
    {
	lseek(fd, i*512, SEEK_SET);
	write(fd, buf, 512);
    }
}
