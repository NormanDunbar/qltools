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
#include <libhxcfe.h>

#define BITSON ( ~0 )

#include "qltools.h"

FLOPPY *floppy;
HXCFLOPPYEMULATOR *hxcfe;
int ifmode;
char *filename;
int loaderid;
int rw_access;
int sector_written;

int OpenQLDevice(char *name, int mode)
{
	int sectors, err;

	hxcfe = hxcfe_init();
	//hxcfe_setOutputFunc(hxcfe, &CUI_affiche);

	loaderid = hxcfe_autoSelectLoader(hxcfe, name, 0);

	rw_access = hxcfe_getLoaderAccess(hxcfe, loaderid);

	floppy = hxcfe_floppyLoad(hxcfe, name, loaderid, &err);

	hxcfe_getFloppySize(hxcfe, floppy, &sectors);

	if ((sectors != 1440) && (sectors != 2880)) {
		hxcfe_floppyUnload(hxcfe, floppy);
		hxcfe_deinit(hxcfe);

		return -1;
	}

	ifmode = hxcfe_floppyGetInterfaceMode(hxcfe, floppy);

	filename = name;
	sector_written = 0;

	return 0;
}

int ReadQLSector(int fd, void *buf, int sect)
{
	SECTORSEARCH *ss;
	int status;
	
	int track, side, sector;

	ss = hxcfe_initSectorSearch(hxcfe, floppy);
	
	if (!sect) {
		track = 0;
		side = 0;
		sector = 1;
	} else {
		track = LTP_TRACK(sect);
		side = LTP_SIDE(sect);
		sector = LTP_SECT(sect);
	}

	hxcfe_readSectorData(ss, track, side, sector,1, GSSIZE,
			ISOIBM_MFM_ENCODING, buf, &status);

	hxcfe_deinitSectorSearch(ss);

	return 512;
}

int WriteQLSector (int fd, void *buf, int sect)
{
	SECTORSEARCH *ss;
	int status;
	
	int track, side, sector;

	ss = hxcfe_initSectorSearch(hxcfe, floppy);
	
	if (!sect) {
		track = 0;
		side = 0;
		sector = 1;
	} else {
		track = LTP_TRACK(sect);
		side = LTP_SIDE(sect);
		sector = LTP_SECT(sect);
	}

	hxcfe_writeSectorData(ss, track, side, sector,1, GSSIZE,
			ISOIBM_MFM_ENCODING, buf, &status);

	hxcfe_deinitSectorSearch(ss);

	sector_written = 1;

	return 512;
}

void CloseQLDevice(int fd)
{
	if (sector_written && (rw_access & 0x2))
		hxcfe_floppyExport(hxcfe, floppy, filename, loaderid);
	else if (sector_written)
		printf("This was not a libhxcfe writable format\n");

	hxcfe_floppyUnload(hxcfe, floppy);
	hxcfe_deinit(hxcfe);
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
		WriteQLSector (fd, buf, i);
	}
}
