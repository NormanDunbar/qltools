#ifdef __unix__
# include <sys/time.h>
# include <unistd.h>
#else
# define NEED_DEFS
# if defined (__VMS) || defined(__ALPHA) || defined(VMS) 
# ifndef __DECC
#  error "You need a real 'C' compiler"
# endif
# include <sys/time.h>
# include <unixio.h>
# include <fcntl.h>
# define FDNAME "diskimage"
# ifdef __ALPHA
#  define OSNAME "Alpha-AXP"
# else
#  define OSNAME "VAX-VMS"
# endif
# else
#  include <conio.h>
#  include <io.h>
#  include <malloc.h>
#  include <time.h>
#  endif
#endif
#if defined(NEED_DEFS) || defined (__GO32__)
 typedef unsigned char unchar;
 typedef unsigned short ushort;
 typedef unsigned long ulong;
# define inline
#endif

#ifdef __NT__
#include <windows.h>
#define OSNAME "Windows NT"
#define FDNAME "\\\\.\\a:"
#define DOS_LIKE
#endif

#ifdef __OS2__
#include <os2.h>
typedef HFILE HANDLE;
#define OSNAME "OS/2"
#define FDNAME "a:"
#define DOS_LIKE
#else
# ifndef __NT__
 typedef int HANDLE;
 #endif
#endif

#ifdef __GO32__
# define OSNAME "DOS/WIN"
# ifndef FDNAME
#  define FDNAME "a:"
# endif
#else
# ifdef __unix__
#  ifdef __linux__
#   define OSNAME "Linux"
    typedef unsigned char unchar;
#  else
#   define OSNAME "Unix"
#  endif
#  define FDNAME "/dev/fd0H720"
# else
#  ifndef OSNAME
#   define OSNAME "MSDOS"
#   define DOS_LIKE
#  endif
#  ifndef FDNAME
#   define FDNAME "a:"
#  endif
# endif
#endif

#define TIME_DIFF    283996800
#define VERSION     "2.14, " __DATE__

/* Maximum allocation block (normally 3) */
#define MAXALB          6

/* Maximum number of sectors (norm. 1440) */
#define MAXSECT         2880
#ifndef __MINGW32__
#if defined __GNUC__
#define PACKED  __attribute__ ((packed))
#else
#define PACKED
#endif
#endif

#define GSSIZE 512
#define DIRSBLK 8

typedef struct
{
    long d_length;		/* file length */
    unsigned char d_access;	/* file access type */
    unsigned char d_type;	/* file type */
    long d_datalen PACKED;	/* data length */
    long d_reserved PACKED;	/* Unused */
    short d_szname;		/* size of name */
    char d_name[36];		/* name area */
    long d_update PACKED;	/* last update */
    short d_version;
    short d_fileno;
    long d_backup;
} QLDIR;

typedef struct
{
    char q5a_id[4];
    unchar q5a_mnam[10];
    ushort q5a_rand;
    ulong q5a_mupd;
    ushort q5a_free;
    ushort q5a_good;
    ushort q5a_totl;
    ushort q5a_strk;
    ushort q5a_scyl;
    ushort q5a_trak;
    ushort q5a_allc;
    ushort q5a_eodbl;
    ushort q5a_eodby;
    ushort q5a_soff;
    unchar q5a_lgph[18];
    unchar q5a_phlg[18];
    unchar q5a_spr0[20];
    unchar map[1];
} BLOCK0;

typedef struct __sdl__
{
    struct __sdl__ *next;
    ulong flen;
    ushort fileno;
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

#if defined (__linux__) || defined (VMS)
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

