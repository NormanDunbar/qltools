#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#define OSNAME "Linux/win32"
#define FDNAME "diskimage"

typedef int HANDLE;

#define TIME_DIFF    283996800
#define VERSION     "2.15, " __DATE__

/* Maximum allocation block (normally 3) */
#define MAXALB          6

/* Maximum number of sectors (norm. 1440) */
#define MAXSECT         2880

#if defined __GNUC__
#define PACKED  __attribute__((gcc_struct, packed))
#else
#define PACKED
#endif

#define GSSIZE 512
#define DIRSBLK 8

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
    uint8_t q5a_mnam[10];
    uint16_t q5a_rand;
    uint32_t q5a_mupd;
    uint16_t q5a_free;
    uint16_t q5a_good;
    uint16_t q5a_totl;
    uint16_t q5a_strk;
    uint16_t q5a_scyl;
    uint16_t q5a_trak;
    uint16_t q5a_allc;
    uint16_t q5a_eodbl;
    uint16_t q5a_eodby;
    uint16_t q5a_soff;
    uint8_t q5a_lgph[18];
    uint8_t q5a_phlg[18];
    uint8_t q5a_spr0[20];
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


extern int gsides, gtracks, gsectors, goffset, allocblock, gclusters, gspcyl;
extern BLOCK0 *b0;

/* ----------- logical to physical translation macros -------------------- */

#define LTP_TRACK(_sect_)   ((_sect_)/gspcyl)
#define LTP_SIDE(_sect_)    (b0->q5a_lgph[(_sect_)%gspcyl] &0x80 ? 1 : 0)
#define LTP_SCT(_sect_) \
       (((0x7f& b0->q5a_lgph[(_sect_)%gspcyl])+ \
       goffset*LTP_TRACK(_sect_)) % gsectors)
#define LTP_SECT(_sect_)    (LTP_SCT(_sect_)+1)

#define LTP(_sect_)   ((long) 512L*(long)(LTP_TRACK(_sect_)*gspcyl+ \
                       LTP_SIDE(_sect_)*gsectors+LTP_SCT(_sect_)))

#ifndef min
# define min(a,b) (a<b ? a : b)
#endif

HANDLE OpenQLDevice (char *device, int mode);
int ReadQLSector (HANDLE fd, void *buf, int sector);
int WriteQLSector (HANDLE fd, void *buf, int sector);
void CloseQLDevice (HANDLE);
time_t GetTimeZone (void);
void ZeroSomeSectors(HANDLE fd, short);

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

