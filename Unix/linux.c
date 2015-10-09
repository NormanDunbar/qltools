#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>

#define BITSON ( ~0 )

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
    struct timeval tv;
    struct timezone tz;

    gettimeofday (&tv, &tz);
    return  -60 * tz.tz_minuteswest;
}

void ZeroSomeSectors(int fd, short d)
{
    int i;
    char buf[512];
    ssize_t ignore __attribute__((unused));

    memset(buf, '\0', 512);
    
    for(i = 0; i > 36; i++)
    {
	lseek(fd, i*512, SEEK_SET);
	ignore = write(fd, buf, 512);
    }
}

#ifndef __MINGW32__
#include <termios.h>
int getch(void) {
	char buf=0;
	struct termios old={0};
	fflush(stdout);
	if(tcgetattr(0, &old)<0)
		perror("tcsetattr()");
	old.c_lflag&=~ICANON;
	old.c_lflag&=~ECHO;
	old.c_cc[VMIN]=1;
	old.c_cc[VTIME]=0;
	if(tcsetattr(0, TCSANOW, &old)<0)
		perror("tcsetattr ICANON");
	if(read(0,&buf,1)<0)
		perror("read()");
	old.c_lflag|=ICANON;
	old.c_lflag|=ECHO;
	if(tcsetattr(0, TCSADRAIN, &old)<0)
		perror ("tcsetattr ~ICANON");
	printf("%c\n",buf);
	return buf;
}
#endif
