#ifdef __BORLANDC__
    // Borland C/Embarcadero C compiler.
    #include "unistd_win.h"
#else
    // Gcc on any platform.
    #include <sys/time.h>
    #include <unistd.h>
#endif

#include <stdint.h>

#define OSNAME "Linux/win32"
#define FDNAME "diskimage"

typedef int HANDLE;

#define TIME_DIFF    283996800
#define VERSION     "2.15.4, " __DATE__

/*
 * Maximum allocation block (normally 3 for DD/HD but 12 for ED)
 * Used for the number of 512 byte sectors in the map.
 * DD and HD use 3 * 512 byte sectors
 * ED needs 3 * 2048 byte sectors, equates to 12 512 byte sectors.
 */
#define MAXALB          12

#if defined __GNUC__
#define PACKED  __attribute__((gcc_struct, packed))
#else
#define PACKED
#endif

#define GSSIZE 512
#define DIRENTRYSIZE 64

typedef struct PACKED
{
    int32_t d_length;		/* file length */
    unsigned char d_access;	/* file access type */
    unsigned char d_type;	/* file type */
    int32_t d_datalen;	/* data length */
    int32_t d_reserved;	/* Unused */
    short d_szname;		/* size of name */
    char d_name[36];		/* name area */
    int32_t d_update;	/* last update */
    short d_version;
    short d_fileno;
    int32_t d_backup;
} QLDIR;

typedef struct PACKED
{
    char q5a_id[4];
    uint8_t q5a_medium_name[10];
    uint16_t q5a_random;
    uint32_t q5a_update_count;
    uint16_t q5a_free_sectors;
    uint16_t q5a_good_sectors;
    uint16_t q5a_total_sectors;
    uint16_t q5a_sectors_track;
    uint16_t q5a_sectors_cyl;
    uint16_t q5a_tracks;
    uint16_t q5a_allocation_blocks;
    uint16_t q5a_eod_block;
    uint16_t q5a_eod_byte;
    uint16_t q5a_sector_offset;
    uint8_t q5a_log_to_phys[18];
    uint8_t q5a_phys_to_log[18];
    uint8_t q5a_spare[20];
    uint8_t map[1];
} BLOCK0;

typedef struct __sdl__
{
    struct __sdl__ *next;
    uint32_t flen;
    uint16_t fileno;
    short szname;
    char name[36];
} SDL;

typedef struct
{
    short nfiles;
    short rflag;
    short freed;
    short indir;
} COUNT_S;

typedef struct
{
    QLDIR *nde;
    int *nptr;
    short fnew;
} FSBLK;


extern int gNumberOfSides, gNumberTracks, gSectorsPerTrack, gOffsetCylinder,
           gSectorsPerBlock, gNumberOfClusters, gSectorsPerCylinder, gSectorSize,
           gDirEntriesPerBlock;
extern BLOCK0 *b0;

/* ----------- logical to physical translation macros -------------------- */

#define LTP_TRACK(_sect_)   ((_sect_)/gSectorsPerCylinder)

#define LTP_SIDE(_sect_)    (b0->q5a_log_to_phys[(_sect_)%gSectorsPerCylinder] &0x80 ? 1 : 0)

#define LTP_SCT(_sect_) \
       (((0x7f& b0->q5a_log_to_phys[(_sect_)%gSectorsPerCylinder])+ \
       gOffsetCylinder*LTP_TRACK(_sect_)) % gSectorsPerTrack)

#define LTP_SECT(_sect_)    (LTP_SCT(_sect_)+1)

#define LTP(_sect_)   ((long) gSectorSize*(long)(LTP_TRACK(_sect_)*gSectorsPerCylinder+ \
                       LTP_SIDE(_sect_)*gSectorsPerTrack+LTP_SCT(_sect_)))

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

HANDLE OpenQLDevice (char *device, int mode);
int ReadQLSector (HANDLE fd, void *buf, int sector);
int WriteQLSector (HANDLE fd, void *buf, int sector);
void CloseQLDevice (HANDLE fd);
time_t GetTimeZone (void);
void ZeroSomeSectors(HANDLE fd, short count);

#if defined (__linux__) || defined (VMS) || defined(__MINGW32__)
int getch(void);
# ifndef O_BINARY
# define O_BINARY 0
# endif
#else
extern void usage(char *);
#define strcasecompare stricmp
#define strncasecompare strnicmp
#endif


#define QLDIRSTRING "qltools:type255\012"
#ifdef VMS
char *strdup(const char *);
#endif
