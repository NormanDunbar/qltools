#if defined  (__NT__) || defined (__OS2__)
# define __MSDOS__
#endif

#if defined  (__MSDOS__)
# ifndef MSDOS
#  define MSDOS
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <bios.h>
#include <dos.h>		/* for delay    */
#include <io.h>			/* for setmode  */
#include <conio.h>

static unsigned int drive;

#ifdef __WATCOMC__
typedef unsigned short ushort;

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
void usage(void)
{
    fputs("qdc in out\n", stderr);
    exit(0);
}

int main(int ac, char **av)
{
    short act;
    short err;
    int fd, mode;
    char *fn;
    struct diskinfo_t di;
    int nbuf;
    unsigned char *buf;

    if(ac >= 3)
    {
        if(*(*(av+1)+1) == ':')
        {
            di.drive = toupper(**(av+1)) - 'A';
            mode = O_CREAT|O_TRUNC|O_WRONLY;
            fn = *(av+2);
            act = _DISK_READ;
        }
        else if(*(*(av+2)+1) == ':')
        {
            di.drive = toupper(**(av+2)) - 'A';
            mode = O_RDONLY;
            fn = *(av+1);
            act = _DISK_WRITE;
        }
        else
        {
            usage();
        }
        if(ac == 4 && tolower(**(av+3) != 'd'))
        {
            di.nsectors = 18;
        }
        else
        {
            di.nsectors = 9;
        }

        if((buf = malloc(nbuf = 512*di.nsectors)))
        {
            mode |= O_BINARY;
            fd = open(fn, mode, 0666);
            if(fd > 0)
            {
                di.buffer = buf;
                di.sector = 1;
                err  = _bios_disk (_DISK_RESET, &di);
                delay(100);
                err  = _bios_disk (_DISK_RESET, &di);
                for (di.track = 0; err == 0 && di.track < 80; di.track++)
                {
                    for(di.head = 0; err == 0 && di.head < 2; di.head++)
                    {
                        printf("Side %d, track %2d\r", di.head, di.track);
                        fflush(stdout);
                        if(act == _DISK_WRITE)
                        {
                            err = (0 > read(fd, buf, nbuf));
                        }
                        if(err == 0)
                        {
                            err = 0xff00 & _bios_disk (act, &di);
        	            if (err)
                            {
	                        printf ("read block: %d %d %d %d\n",
                                    di.head, di.track, di.sector, err);
                            }
                            if(act == _DISK_READ && !err)
                            {
                                err = 0 > write(fd, buf, nbuf);
                            }
                        }
                    }
	        }
                close(fd);
                putc('\n', stdout);
            }
            else
            {
                perror("open:");
            }
        }
    }
    else
    {
        usage();
    }
    return 0;
}

