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

int OpenQLDevice(char *name, mode_t mode, mode_t permissions) {
    return open(name, mode, permissions);
}

int ReadQLSector(int fd, void *buf, int sect) {
    long fpos;
    int err;

    fpos = (sect) ? LTP (sect) : 0;

    err = lseek (fd, fpos, SEEK_SET);
    if (err < 0)
	    perror("ReadQLSector : lseek()");
    else
	    err = read(fd, buf, gSectorSize);
    return err;
}

int WriteQLSector (int fd, void *buf, int sect) {
    int err;

    err = lseek(fd, LTP (sect), SEEK_SET);
    if (err < 0)
	    perror("WriteQLSector: lseek()");
    else
	    err = write(fd, buf, gSectorSize);
        if (err < 0)
            perror("WriteQLSector: write()");
    return err;
}

void CloseQLDevice(int fd) {
    close(fd);
}

time_t GetTimeZone(void) {
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return  -60 * tz.tz_minuteswest;
}

void ZeroSomeSectors(int fd, short d) {
    int i;
    char buf[gSectorSize];
    ssize_t ignore __attribute__((unused));
    int writeSectorCount = 0;

    memset(buf, '\0', gSectorSize);

    /* Some weirdness from Richard??? "short d" is actually
     * a 2 byte format indicator, dd, hd, ed which converts
     * to:
     * 
     * dd = 100 = 1440 sectors
     * ed = 101 = 1600 sectors
     * hd = 104 = 2880 sectors
     * 
     * Because we don't yet have gNumberofTracks and
     * gSectorsPerCylinder worked out, we can use the
     * value in d to write out the whole image file.
     */

    switch (d) {
        case 'd':   writeSectorCount = 1440; break;
        case 'e':   writeSectorCount = 1600; break;
        case 'h':   writeSectorCount = 2880; break;
        default:    writeSectorCount = 36; break;
    }


    for(i = 0; i < writeSectorCount; i++) {
	    lseek(fd, i * gSectorSize, SEEK_SET);
	    ignore = write(fd, buf, gSectorSize);
    }
}

#ifndef __MINGW32__
#include <termios.h>
int getch(void) {
	char buf=0;
	struct termios old={0};
	fflush(stdout);

	if(tcgetattr(0, &old) < 0)
		perror("tcsetattr()");

	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;

	if(tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");

	if(read(0, &buf, 1) < 0)
		perror("read()");

	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;

	if(tcsetattr(0, TCSADRAIN, &old) < 0)
		perror ("tcsetattr ~ICANON");

	printf("%c\n", buf);
	return buf;
}
#endif
