#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <libhxcfe.h>

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
	if (loaderid < 0) {
		printf("Loader ID returned error %d\n", loaderid);
		hxcfe_deinit(hxcfe);
		return -1;
	}

	rw_access = hxcfe_getLoaderAccess(hxcfe, loaderid);

	floppy = hxcfe_floppyLoad(hxcfe, name, loaderid, &err);
	if (!floppy) {
		printf("Failed to load image\n");
		return -1;
	}

	hxcfe_getFloppySize(hxcfe, floppy, &sectors);

	/*
	 * 1600 -> 10Sectors per track : Ql trump board seems to put an unused
	 * sector at the head of the track... (Jeff_HxC2001)
	 */
	if ((sectors != 1440) && (sectors != 2880) && (sectors != 1600)) {
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
