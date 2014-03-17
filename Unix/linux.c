#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <ctype.h>
#include <termios.h>
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

static void rl_ttyset (int Reset)
{
    static struct termios old;
    struct termios new;

    if (Reset == 0)
    {
	(void) tcgetattr (0, &old);
	new = old;
	new.c_cc[VINTR] = BITSON;
	new.c_cc[VQUIT] = BITSON;
	new.c_lflag &= ~(ECHO | ICANON);
	new.c_iflag &= ~(ISTRIP | INPCK);
	new.c_cc[VMIN] = 1;
	new.c_cc[VTIME] = 0;
	(void) tcsetattr (0, TCSANOW, &new);
    }
    else
	(void) tcsetattr (0, TCSANOW, &old);
}

int getch(void)
{
    char c;
    rl_ttyset (0);
    read (0, &c, 1);
    write(1, &c, 1);
    write(1, "\b", 1);
    rl_ttyset (1);
    return (int)c;
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
