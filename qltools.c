/***************************************************************************
 * QLTOOLS
 *
 * Read a QL floppy disk
 *
 * (c)1992 by Giuseppe Zanetti
 *
 * Giuseppe Zanetti
 * via Vergani, 11 - 35031 Abano Terme (Padova) ITALY
 * e-mail: beppe@sabrina.dei.unipd.it
 *
 * Valenti Omar
 * via Statale,127 - 42013 Casalgrande (REGGIO EMILIA) ITALY
 * e-mail: sinf7@imovx2.unimo.it
 * sinf7@imoax1.unimo.it
 * sinf@c220.unimo.it
 *
 * somewhat hacked by Richard Zidlicky, added formatting, -dl, -x option
 * rdzidlic@cip.informatik.uni-erlangen.de
 *
 * Seriously major rewrite (c) Jonathan R Hudson to support level 2
 * sub-directories, fix some bugs and make the code IMHO :-) rather more
 * maintainable. Split system specific code into individual directories and
 * added code for NT and OS2.
 *
 *
 * Revision 2.15.6 2021/11/19 NDunbar
 * Forced the image file to be as big as an actual floppy
 * in ZeroSomeSectors. It was only writing 36 sectors up
 * until this version. This was mentioned on the Ql Forum
 * as a potential problem. Now resolved.
 * 
 * Maybe, this should be an option? Previously it only wrote
 * as many sectors as it needed to write the data being loaded.
 *
 * Anyone know what happened to 2.15.4 and 2.15.5?
 * 
 * Revision 2.15.3 2018/11/03 NDunbar
 * More tidying up of the usage messages. Proper letter case etc.
 * Extracted common code from the three format routines to reduce duplication.
 * Version bumped to 2.15.3.
 *
 * Revision 2.15.2 2018/11/02 NDunbar
 * More variables renamed.
 * Added -t option to usage.
 * Reformatted usage output better (in my opinion!)
 * Adjusted MAXALB to 12 from 6. This is the number of 512 byte sectors needed for a
 * floppy cluster for the map. DD/HD need 6 * 512 while ED needs 3 * 2048 (or 12 * 512).
 * Added ED disks/image processing.
 *
 * Revision 2.15.1 2018/10/29 NDunbar
 * Much reformatting of code, renaming to give better names etc.
 * Fixed bug in ZeroSomeSectors (DOS/Linux only) - nothing was being done.
 * Removed many hard coded magic numbers.
 * Slight update to version number, 2.15 to 2.15.1.
 * Some error messages corrected.
 *
 * $Log: qltools.c,v $
 * Revision 2.11  1996/07/14 11:57:07  jrh
 * Tidied up code
 *
 * Revision 2.10  1996/07/14  10:28:58  jrh
 * added extra info messages
 *
 * Revision 2.9  1995/11/29  18:34:53  root
 * Changed output routine to correctly decide if an XTcc block was
 * required.
 *
 * Revision 2.8  1995/10/22  14:11:36  root
 * Added VMS support
 *
 * Revision 2.7  1995/10/22  14:02:42  root
 * Added zeroing of some sectors
 *
 * Revision 2.6  1995/09/21  17:01:03  root
 * Release version 2.4
 *
 * Revision 2.5  1995/09/18  13:13:59  root
 * Full support for Level 2 directories
 *
 * Revision 2.4  1995/09/16  18:56:48  root
 * Placeholder, system specific code in
 *
 * Revision 2.3  1995/09/09  16:38:08  root
 * Added data structures for disk objects (directories, block 0 etc)
 * Did I fix a few more bugs too ?
 *
 * Revision 2.2  1995/09/06  12:13:17  root
 * Fixed null version record in DOS, and some other bugs
 *
 * Revision 2.1  1995/09/06  12:11:26  root
 * Initial jh version, fixed lots of bugs
 *
 ****************************************************************************/

static char rcsid[] = "$Id: qltools.c,v 2.11 1996/07/14 11:57:07 jrh Exp jrh $";

#if \
  defined (__OS2__)     || \
  defined (__NT__)      || \
  defined  (__MSDOS__)  || \
  defined(__GO32__)     || \
  defined(__WINNT__)    || \
  defined(__WIN32__)    || \
  defined (__WIN64__)   || \
  defined (__MINGW32__) || \
  defined (__MINGW64__)
    # define DOS_LIKE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <stdint.h>
#include "qltools.h"

/* -------------------------- globals ----------------------------------- */
int gNumberOfSides, gNumberOfTracks, gSectorsPerTrack, gOffsetCylinder,
    gSectorsPerBlock, gNumberOfClusters, gSectorsPerCylinder, gSectorSize,
    gDirEntriesPerBlock;

BLOCK0 *b0;

/* -------------------------- 'local' globals --------------------------- */
static HANDLE fd;
static SDL *SList = NULL;	/* sub-directory list */
static unsigned int directoryEndBlock;	/* directory block offset */
static unsigned int directoryEndByte;	/*           bytes        */
static QLDIR *pdir;		    /* memory image of directory */
static long err;
static short tranql = 1;
static short list = 0;
static int ql5a;		    /* flag */
static int block_dir[64];
static int block_max = 0;
static long lac = 0;		/* last allocated cluster */
static time_t timeadjust;
static char OWopt = 0;

/* -------------------------- Prototypes ------------------------------- */
void write_cluster(void *, int);
int read_cluster(void *, int);
void make_convtable(int);
int print_entry(QLDIR *, int, void *);
void free_cluster(long);
int alloc_new_cluster(int, int, short);
void format(char *, char *);
void dir_write_back(QLDIR *, SDL *, short *);
void set_header(int, long, QLDIR *, SDL *);
void read_b0fat(int);
void write_b0fat(void);
void read_fat(void);
void read_dir(void);
void print_map(void);
int RecurseDir(int, long, void *, int (*func) (QLDIR *, int, void *));
int find_free_cluster(void);

void *xmalloc(long alot) {
    void *p = malloc(alot);

    if (p == NULL) {
        perror("! :");
        exit(ENOMEM);
    }
    else {
        memset(p, 0, alot);
    }

    return p;
}


/* --------------------- byte order conversion --------------------------- */

uint16_t swapword(uint16_t val) {
    return (uint16_t) (val << 8) + (val >> 8);
}

uint32_t swaplong(uint32_t val) {
    return (uint32_t) (((uint32_t) swapword (val & 0xFFFF) << 16) |
                       (uint32_t) swapword (val >> 16));
}

/* ----------------------------------------------------------------------- */

static int maxdir(void) {
    return (directoryEndByte >> 6) + (directoryEndBlock * gDirEntriesPerBlock);
}

uint16_t fat_file(uint16_t cluster) {
    uint8_t *base = b0->map + cluster * 3;

    return (uint16_t) (*base << 4) + (uint16_t) ((*(base + 1) >> 4) & 15);
}

uint16_t fat_cl(uint16_t cluster) {
    uint8_t *base = b0->map + cluster * 3;

    return (*(base + 1) & 15) * 256 + *(base + 2);
}

void set_fat_file(uint16_t cluster, uint16_t file) {
    uint8_t *base = b0->map + cluster * 3;

    *base = file >> 4;
    *(base + 1) = ((file & 15) << 4) | (*(base + 1) & 15);
}

void set_fat_cl(uint16_t cluster, uint16_t clnum) {
    uint8_t *base = b0->map + cluster * 3;

    *(base + 1) = (clnum >> 8) | (*(base + 1) & (~15));
    *(base + 2) = clnum & 255;
}


uint16_t FileXDir(uint16_t fnum) {
    uint16_t fno = 0;
    if(fnum) {
        fno = swapword(fnum) - 1;
    }

    return fno;
}

QLDIR *GetEntry(int fn) {
    QLDIR *entry;

    if (fn & 0x800) {
        entry = NULL;
    } else {
        entry = pdir + fn;
    }
    return entry;
}

short FindCluster(uint16_t fnum, uint16_t blkno) {
    uint16_t file, blk;
    short i;

    for (i = 0; i < gNumberOfClusters; i++) {
        file = fat_file(i);
        blk = fat_cl(i);
        if (file == fnum && blk == blkno) {
            break;
        }
    }
    return (i == gNumberOfClusters) ? -1 : i;
}

void cat_file(long fnum, QLDIR * entry) {
    long flen;
    int i, ii, s, start, end;
    long qldata = 0;
    ssize_t ignore __attribute__((unused));

#ifdef DOS_LIKE
    setmode (fileno (stdout), O_BINARY);
#endif

    if(entry->d_type == 255) {
        ignore = write(1, QLDIRSTRING, 16);
    }
    else {
        flen = swaplong (entry->d_length);	/* with the header */
        if (entry->d_type == 1) {
            qldata = entry->d_datalen;
        }

        if (flen + swapword (entry->d_szname) == 0) {
            fputs("warning: file appears to be deleted\n", stderr);
        }
        else {
            char *buffer = xmalloc (gSectorSize * gSectorsPerBlock);
            long lblk = flen / (gSectorSize * gSectorsPerBlock);
            long xblk = 0,xbyt = 0;
            short needx = 1;

            if(qldata) {
                xblk = (flen - 8) / (gSectorSize * gSectorsPerBlock);
                xbyt = (flen - 8) % (gSectorSize * gSectorsPerBlock);
            }

            for (s = 0; s <= lblk; s++) {
                if ((i = FindCluster(fnum, s)) != -1) {
                    read_cluster(buffer, i);
                    if (s == 0)
                        start = 64;
                    else
                        start = 0;
                    end = gSectorSize * gSectorsPerBlock;
                    if (s == lblk)
                        end = flen % (gSectorSize * gSectorsPerBlock);
                    err = write (1, buffer + start, end - start);
                    if (err < 0)
                        perror ("Output file: write(): ");
                    if(qldata && s == xblk) {
                        needx = memcmp(buffer+start+xbyt, "XTcc", 4);
                    }
                }
                else {
                    fprintf(stderr, "** Cluster #%d of %.*s not found **\n",
                             s, entry->d_szname, entry->d_name);
                    err = lseek(1, gSectorSize * gSectorsPerBlock, SEEK_CUR);
                    /* leave hole */
                    if (err < 0)	/* non seekable */
                        for (ii = 0; ii < gSectorsPerBlock * gSectorSize; ii++)
                            fputc ('#', stdout);
                }
            }
            free(buffer);

            if (qldata && needx) {
                static struct {
                    union {
                        char xtcc[4];
                        long x;
                    } x;
                    long dlen;
                } xtcc =  {{"XTcc"}, 0};
                xtcc.dlen = qldata;
                err = write(1, &xtcc, 8);
            }
        }
    }
}

void UpdateSubEntry(QLDIR * entry, SDL * sdl, short *off) {

    int s, start, end;
    int i, j;
    int rval = 0;
    long flen = sdl->flen;
    short fnum = sdl->fileno;
    uint8_t *buffer = xmalloc (gSectorSize * gSectorsPerBlock);

    if (off) {
        s = (*off) / (gSectorSize * gSectorsPerBlock);
        j = (*off) % (gSectorSize * gSectorsPerBlock);
        if ((i = FindCluster(fnum, s)) != -1) {
            if (flen != 128 && j != 64) {
                read_cluster(buffer, i);
            }
            memcpy((buffer + j), entry, 64);
            write_cluster (buffer, i);
        }
    }
    else {
        for (s = 0; !rval && s <= flen / (gSectorSize * gSectorsPerBlock); s++) {
            i = FindCluster (fnum, s);
            if (i != -1) {
                read_cluster(buffer, i);
                if (s == 0)
                    start = 64;
                else
                    start = 0;
                end = gSectorSize * gSectorsPerBlock;

                if (s == (flen / (gSectorSize * gSectorsPerBlock)))
                    end = (flen % (gSectorSize * gSectorsPerBlock));
                else
                    end = gSectorSize * gSectorsPerBlock;
                for (j = start; j <= end; j += 64) {
                    QLDIR *ent = (QLDIR *) (buffer + j);

                    if (ent->d_fileno == entry->d_fileno) {
                        if (entry->d_szname == 0 && entry->d_length == 0) {
                            memset(ent, '\0', 64);
                        }
                        else {
                            memcpy(ent, entry, 64);
                        }
                        write_cluster(buffer, i);
                        rval = 1;
                        break;
                    }
                }
            }
        }
    }
    free(buffer);
}

void RemoveList(SDL * sdl) {
    SDL *prev = NULL, *sl;

    for (sl = SList; sl; prev = sl, sl = sl->next) {
        if (sdl == sl) {
            if (prev == NULL) {
                SList = sl->next;
            }
            else {
                prev->next = sl->next;
            }
            free(sl);
            sl = NULL;
            break;
        }
    }
}


int CountDir(QLDIR * e, int fnum, COUNT_S *s) {
    long len;

    if (((len = e->d_length) != 0) &&  (e->d_szname != 0)) {
        s->nfiles++;
    }

    if (s->rflag == 0) {
        s->indir++;
        if (len == 0 && s->freed == 0) {
            s->freed = s->indir;
        }
    }
    else if (e->d_type == 255) {
        RecurseDir(fnum, swaplong (e->d_length), s,
                   (int (*) (QLDIR *, int, void *)) CountDir);
    }
    return 0;
}

void del_file(long fnum, QLDIR * entry, SDL * sdl) {
    long int flen, file;
    int i, freed, blk0;
    short rdir = 0;
    COUNT_S nf;

    nf.nfiles = 0;
    nf.rflag = 1;

    if (entry->d_type == 255) {
        RecurseDir(fnum, swaplong (entry->d_length), &nf,
                   (int (*) (QLDIR *, int, void *)) CountDir);
        if (nf.nfiles) {
            fprintf(stderr, "Directory must be empty to delete (%d)\n",
                     nf.nfiles);
            exit (ENOTEMPTY);
        }
        else {
            rdir = 1;
        }
    }

    freed = 0;
    blk0 = -1;

    flen = swaplong(entry->d_length);
    if (flen == 0) {
        fprintf(stderr, "file already deleted?\n");
        exit (EBADF);
    }

    for (i = 1; i < gNumberOfClusters; i++) {
        file = fat_file(i);
        if (file == fnum) {
            if (fat_cl(i) == 0)
                blk0 = i;
            free_cluster(i);
            freed++;
        }
    }

    b0->q5a_free_sectors = swapword(freed * gSectorsPerBlock + swapword (b0->q5a_free_sectors));

    entry->d_szname = 0;
    entry->d_length = 0;
    entry->d_type = 0;

    if (blk0 > 0) {
        uint8_t *b = xmalloc (gSectorSize * gSectorsPerBlock);

        read_cluster(b, blk0);
        memcpy(b, entry, 64);
        write_cluster(b, blk0);
        free(b);
    }
    else
        fprintf(stderr, "block 0 of file missing??\n");

    write_b0fat();		/* write_cluster(b0,0); */
    dir_write_back(entry, sdl, NULL);
    if (rdir) {
        RemoveList(sdl);
    }
}

void usage(char *error) {
    static char *options[] = {
        "\nUsage: qltools device -[options] [filename]\n",
        "Options:\n",
        "    -d         List directory          -s         List short directory",
        "    -i         List disk info          -m         List disk map",
        "    -c         List conversion table   -l         List files on write\n",
        "    -w <files> Write files (query)     -W <files> (Over)write files",
        "    -r <name>  Remove file <name>      -n <flle>  Copy <file> to stdout\n",
        "    -uN        ASCII dump cluster N    -UN        Binary dump",
        "    -M <name>  Make level 2 directory <name>",
        "    -x <name> <size> Make <name> executable with dataspace <size>",
        "    -t         Do not translate '.' to '_' in filenames\n",
        "    -fxx <name> Format as xx=hd|dd|ed disk with label <name>\n",
        "QLTOOLS for " OSNAME " (version " VERSION ")\n",
        "'Device' is either a file with the image of a QL format disk",
        "or a floppy drive with a SMS/QDOS disk inserted in it (e.g. " FDNAME ")\n",
        "by Giuseppe Zanetti,Valenti Omar,Richard Zidlicky, Jonathan Hudson\n& Norman Dunbar",
        NULL
    };
    char **h;

    fprintf(stderr, "\nError : %s\n", error);
    for (h = options; *h; h++) {
        fputs(*h, stderr);
        fputc('\n', stderr);
    }
    exit (EINVAL);
}

void print_info(void) {
    short i;

    printf("Disk ID               : %.4s\n", b0->q5a_id);
    printf("Disk Label            : %.10s\n", b0->q5a_medium_name);
    printf("Number of sides       : %i\n", gNumberOfSides);
    printf("Sectors per track     : %i\n", gSectorsPerTrack);
    printf("Sectors per cylinder  : %i\n", gSectorsPerCylinder);
    printf("Number of cylinders   : %i\n", gNumberOfTracks);
    printf("Sectors per block     : %i\n", gSectorsPerBlock);
    printf("Sector offset/cylinder: %i\n", gOffsetCylinder);
    printf("Random                : %04x\n", swapword (b0->q5a_random));
    printf("Updates               : %" PRIu32 "\n", swaplong (b0->q5a_update_count));
    printf("Free sectors          : %i\n", swapword (b0->q5a_free_sectors));
    printf("Good sectors          : %i\n", swapword (b0->q5a_good_sectors));
    printf("Total sectors         : %i\n", swapword (b0->q5a_total_sectors));

    printf("Directory is          : %u sectors and %u bytes\n", directoryEndBlock, directoryEndByte);

    printf("\nLogical-to-physical sector mapping table:\n\n");
    for (i = 0; i < gSectorsPerCylinder; i++)
        printf("%x ", b0->q5a_log_to_phys[i]);
    putc('\n', stdout);

    if (ql5a) {
        printf("\nPhysical-to-logical sector mapping table:\n\n");
        for (i = 0; i < gSectorsPerCylinder; i++)
            printf("%x ", b0->q5a_phys_to_log[i]);
    }
    putc('\n', stdout);
}

int RecurseDir(int fnum, long flen, void *parm, int (*func) (QLDIR *, int, void *)) {
    int s, start, end;
    int i, j;
    int rval = 0;

    if (flen > 64) {
        uint8_t *buffer = xmalloc(gSectorSize * gSectorsPerBlock);

        for (s = 0; s <= flen / (gSectorSize * gSectorsPerBlock); s++) {

            i = FindCluster(fnum, s);
            if (i != -1) {
                read_cluster(buffer, i);
                if (s == 0)
                    start = 64;
                else
                    start = 0;
                end = gSectorSize * gSectorsPerBlock;

                if (s == (flen / (gSectorSize * gSectorsPerBlock)))
                    end = (flen % (gSectorSize * gSectorsPerBlock));
                else
                    end = gSectorSize * gSectorsPerBlock;
                for (j = start; j <= end; j += 64) {
                    QLDIR *ent = (QLDIR *) (buffer + j);
                    int fno = FileXDir(ent->d_fileno);
                    if ((rval = (func) (ent, fno, parm)) != 0) {
                        break;
                    }
                }
            }
        }
        free (buffer);
    }
    else {
        rval = 1;
    }
    return rval;
}

int print_entry(QLDIR * entry, int fnum, void *flag) {
    short j,k;
    long flen;

    if (entry == NULL)
        return 0;

    flen = swaplong(entry->d_length);

    if (flen + swapword(entry->d_szname) == 0)
        return 0;

    j = k = swapword(entry->d_szname);
    if (*((short *) flag) == 0) {
        k = 36;
    }
    printf ("%-*.*s", k, j, entry->d_name);

    if (entry->d_type == 255) {
        if (*((short *) flag)) {
            putc('\n', stdout);
        }
        else {
            if (*((short *) flag) == 0) {
                printf("(dir) %ld\n", flen - 64l);
            }
        }
        if(*((short *)flag) != 2) {
            RecurseDir(fnum, flen, flag, print_entry);
        }

    }
    else if (*((short *) flag) == 0) {
        switch (entry->d_type) {
        case 0:
            fputs(" ", stdout);
            break;
        case 1:
            fputs("E", stdout);
            break;
        case 2:
            fputs("r", stdout);
            break;
        default:
            printf("%3d", entry->d_type);
            break;
        }
        printf(" %7ld", (long) (flen - 64L)); {
            struct tm *tm;
            time_t t = swaplong(entry->d_update) - TIME_DIFF - timeadjust;
#ifdef __WIN32__
            if(t < 0) t = 0;
#elif defined (__GO32__)
            if(t & (1 << 31)) t = 0;
#endif
            tm = localtime (&t);
            printf(" %02d/%02d/%02d %02d:%02d:%02d v%-5u",
                   tm->tm_mday, tm->tm_mon + 1, tm->tm_year,
                   tm->tm_hour, tm->tm_min, tm->tm_sec,
                   swapword(entry->d_version));
        }
        if (entry->d_type == 1 && entry->d_datalen) {
            printf("%" PRId32, swaplong (entry->d_datalen));
        }
        putc('\n', stdout);
    }
    else {
        putc('\n', stdout);
    }
    return 0;
}

void print_dir(short flag) {
    int d;
    QLDIR *entry;

    if (flag == 0) {
        printf("%.10s\n", b0->q5a_medium_name);
        printf("%i/%i sectors.\n\n",
               swapword(b0->q5a_free_sectors), swapword (b0->q5a_good_sectors));
    }

    for (d = 1; d < maxdir (); d++) {
        entry = pdir + d;
        if (swaplong(entry->d_length) + swapword (entry->d_szname) != 0) {
            print_entry(entry, d, &flag);
        }
    }
}

void make_convtable(int verbose) {
    int i, si, tr, se, uxs, xx, sectors;

    if (verbose) {
        printf("\nCONVERSION TABLE\n\n");
        printf("logic\ttrack\tside\tsector\tunix_dev\n\n");
    }

    sectors = gNumberOfClusters * gSectorsPerBlock;

    if (verbose) {
        for (i = 0; i < sectors; i++) {
            tr = LTP_TRACK (i);
            si = LTP_SIDE (i);
            se = LTP_SCT (i);
            uxs = tr * gSectorsPerCylinder + gSectorsPerTrack * si + se;
            xx = LTP(i);

            if (verbose) {
                printf("%i\t%i\t%i\t%i\t%i %d\n", i, tr, si, se, uxs, xx);
            }
        }
    }
}

void dump_cluster(int num, short flag) {
    int i, sect;
    unsigned char buf[gSectorSize];
    ssize_t ignore __attribute__((unused));

    for (i = 0; i < gSectorsPerBlock; i++) {
        short j, k;
        unsigned char *p;
        long fpos=0;

        sect = num * gSectorsPerBlock + i;
        err = ReadQLSector(fd, buf, sect);

        if (err < 0)
            perror("Dump_cluster: read(): ");
        if (flag == 0) {
            p = buf;
            // How many lines of 16 bytes do we print?
            for (k = 0; k < (gSectorSize / 16); k++) {
                printf("%03lx : ", k * 16 + fpos);

                for (j = 0; j < 16; j++) {
                    printf(" %02x", *p++);
                }
                fputc('\t', stdout);
                p -= 16;

                for (j = 0; j < 16; j++, p++) {
                    printf("%c", isprint (*p) ? *p : '.');
                }
                fputc('\n', stdout);
            }
        }
        else {
            ignore = write(1, buf, gSectorSize);
        }
    }
}

int read_cluster(void *p, int num) {
    int i, sect;
    int r = 0;

    for (i = 0; i < gSectorsPerBlock; i++) {
        sect = num * gSectorsPerBlock + i;
        r = ReadQLSector(fd, (char *) p + i * gSectorSize, sect);
    }
    return r;
}

void write_cluster(void *p, int num) {
    int i, sect;

    for (i = 0; i < gSectorsPerBlock; i++) {
        sect = num * gSectorsPerBlock + i;
        WriteQLSector(fd, (char *) p + i * gSectorSize, sect);
    }
}

void read_b0fat(int argconv) {
    err = ReadQLSector (fd, b0, 0);

    if (strncmp((char *)b0->q5a_id, "QL5", 3)) {
        fprintf(stderr, "\nNot an SMS disk %.4s %2.2x:%2.2x:%2.2x:%2.2x !!!\n",
                b0->q5a_id, b0->q5a_id[0], b0->q5a_id[1], b0->q5a_id[2],
                b0->q5a_id[3]);
        exit(ENODEV);
    }

    ql5a = b0->q5a_id[3] == 'A';

    gNumberOfTracks = swapword(b0->q5a_tracks);
    gSectorsPerTrack = swapword(b0->q5a_sectors_track);
    gSectorsPerCylinder = swapword(b0->q5a_sectors_cyl);
    gNumberOfSides = gSectorsPerCylinder / gSectorsPerTrack;
    gOffsetCylinder = swapword(b0->q5a_sector_offset);
    directoryEndBlock = swapword(b0->q5a_eod_block);
    directoryEndByte = swapword(b0->q5a_eod_byte);
    gSectorsPerBlock = swapword(b0->q5a_allocation_blocks);
    gNumberOfClusters = gNumberOfTracks * gSectorsPerCylinder / gSectorsPerBlock;

    /* Are we on an ED disc? */
    if ((gSectorsPerTrack == 10) && (gSectorsPerBlock == 1) && (gOffsetCylinder == 2))
        gSectorSize = 2048;

    gDirEntriesPerBlock = gSectorSize / DIRENTRYSIZE;

    make_convtable(argconv);
    read_fat();

}

void write_b0fat(void) {
    int i;

    b0->q5a_update_count = swaplong(swaplong (b0->q5a_update_count) + 1);

    for (i = 0; fat_file(i) == 0xf80; i++) {
        write_cluster((uint8_t *) b0 + i * gSectorsPerBlock * gSectorSize, i);
    }
}


void read_fat(void) {
    int i;

    for (i = 0; fat_file(i) == 0xf80; i++)
        read_cluster((uint8_t *) b0 + i * gSectorsPerBlock * gSectorSize, i);
}

short CheckFileName(QLDIR * ent, char *fname) {
    short match = 0;
    char c0, c1;
    int i, len;

    len = strlen(fname);
    if (swapword(ent->d_szname) == len) {
        match = 1;
        for (i = 0; i < len; i++) {
            c0 = *(ent->d_name + i);
            c0 = tolower(c0);
            c1 = tolower(fname[i]);

            if (c0 != c1) {
                if (!tranql || !(c1 == '.' && c0 == '_')) {
                    match = 0;
                    break;
                }
            }
        }
    }
    return match;
}

/* ARGSUSED */
int FindName(QLDIR * e, int fileno, void *llist) {
    int res;
    char **mlist = (char **) llist;

    if ((res = CheckFileName(e, mlist[0])) != 0) {
        memcpy((QLDIR *) mlist[1], e, sizeof (QLDIR));
    }
    return res;
}

long int match_file(char *fname, QLDIR ** ent, SDL ** sdl) {
    static QLDIR sd;
    int d, match = 0;
    long r = 0L;

    if (sdl) {
        *sdl = NULL;
    }
    for (d = 1; d < maxdir (); d++) {
        if ((match = CheckFileName(pdir + d, fname)) != 0) {
            if (ent) {
                *ent = pdir + d;
            }
            r = d;
            break;
        }
    }

    if (match == 0) {
        SDL *sl;

        memset (&sd, 0, 64);
        for (sl = SList; sl; sl = sl->next) {
            if (sl->flen > 64 && strncasecmp (fname, sl->name, sl->szname) == 0
                    && strlen(fname) != sl->szname) {
                void *llist[3];

                llist[0] = fname;
                llist[1] = &sd;
                llist[2] = NULL;

                if ((match = RecurseDir(sl->fileno, sl->flen, llist,
                                        FindName)) != 0) {
                    r = FileXDir(sd.d_fileno);
                    if (ent) {
                        *ent = &sd;
                    }
                    if (sdl) {
                        *sdl = sl;
                    }
                    break;
                }
            }
        }
    }
    return (r);
}

char *MakeQLName(char *fn, short *n) {
    char *p, *q;

    if ((p = strchr(fn, '=')) != NULL) {
        *p++ = '\0';
    }
    else {
        if ((p = strrchr(fn, '/')) != NULL) {
            p++;
        }
        else if ((p = strrchr(fn, '\\')) != NULL) {
            p++;
        }
    }

    if (p == NULL)
        p = fn;
    q = strdup(p);

    *n = min(strlen (q), 36);
    *(q + (*n)) = 0;

    if (tranql) {
        for (p = q; *p; p++) {
            if (*p == '.')
                *p = '_';
        }
    }

    return q;
}

SDL *CheckDirLevel(char *qlnam, short n) {
    SDL *sl;

    for (sl = SList; sl; sl = sl->next) {
        if (n > sl->szname + 1) {
            if (strncasecmp(qlnam, sl->name, sl->szname) == 0) {
                if (*(qlnam + sl->szname) == '_') {
                    break;
                }
            }
        }
    }
    return sl;
}

int AllocNewSubDirCluster(long flen, uint16_t fileno) {
    short i;
    short seqno;
    uint8_t *p;

    seqno = flen / (gSectorSize * gSectorsPerBlock);

    if ((i = alloc_new_cluster(fileno, seqno, 0)) != -1) {
        p = xmalloc(gSectorSize * gSectorsPerBlock);
        write_cluster(p, i);
        free(p);
    }
    return i;
}

QLDIR *GetNewDirEntry(SDL * sdl, int *filenew, int *nblock, short *diroff) {
    int i;
    int hole;
    int offset;
    QLDIR *ent;

    if (sdl == NULL) {
        offset = 1;
        while ((swaplong((pdir + offset)->d_length) +
                swapword((pdir + offset)->d_szname) > 0)
                && (offset < maxdir ()))
        {
            offset++;
        }

        if (offset >= maxdir ()) {
            hole = 0;
            offset = maxdir ();
        }
        else {
            hole = 1;
        }

        if ((directoryEndByte == 0) && ((directoryEndBlock % gSectorsPerBlock) == 0) && !hole) {
            i = alloc_new_cluster(0, block_max, 0);
            if (i < 0) {
                fprintf(stderr, "write file: no free cluster\n");
                exit (ENOSPC);
            }
            *nblock = *nblock + 1;
            block_dir[block_max] = i;
            block_max++;
        }

        if (!hole)
            directoryEndByte += 64;

        if (directoryEndByte == gSectorSize) {
            directoryEndByte = 0;
            directoryEndBlock += 1;
        }

        *filenew = offset;
        ent = pdir + offset;
    }
    else {
        static QLDIR newent;

        if ((sdl->flen % gSectorSize * gSectorsPerBlock) == 0) {
            if (AllocNewSubDirCluster(sdl->flen, sdl->fileno)) {
                *nblock = *nblock + 1;
                *diroff = sdl->flen;
                sdl->flen += 64;
            }
        }
        else {
            COUNT_S nf;

            nf.nfiles = nf.rflag = nf.freed = nf.indir = 0;
            RecurseDir(sdl->fileno, sdl->flen, &nf,
                       (int (*) (QLDIR *, int, void *))CountDir);

            if ((sdl->flen - 64) == 64 * nf.nfiles) {
                *diroff = sdl->flen;
                sdl->flen += 64;
            }
            else {
                *diroff = 64 * nf.freed;
            }
        }
        i = find_free_cluster();
        ent = &newent;
        *filenew = (i + 0x800);
    }
    return ent;
}

SDL * AddSlist(QLDIR *entry, int fileno) {
    SDL *sdl;
    sdl = xmalloc(sizeof (SDL));
    sdl->flen = swaplong(entry->d_length);
    sdl->fileno = fileno;
    sdl->szname = swapword(entry->d_szname);
    memcpy(sdl->name, entry->d_name, sdl->szname);
    sdl->next = SList;
    SList = sdl;

    return sdl;

}

int ProcessSubFile(QLDIR *entry, int fileno, FSBLK *fs) {

    short n = swapword(fs->nde->d_szname);
    short m = swapword(entry->d_szname);


    if((m > n + 1) &&
            strncasecmp(entry->d_name, fs->nde->d_name, n) == 0 &&
            *(entry->d_name + n) == '_') {
        uint16_t dcl, fcl;
        QLDIR nent;
        SDL *nsdl;
        short i;
        uint8_t *buf;
        long cwdlen;

        fcl = FindCluster(fileno, 0);
        nent = *entry;

        /* If its a root entry, this removes it */

        entry->d_length = 0;
        entry->d_szname = 0;

        if((nsdl = CheckDirLevel(entry->d_name, m)) != NULL) {
            /* If a sub-dir, this does */
            UpdateSubEntry(entry, nsdl, 0);
        }

        for (i = 0; i < gNumberOfClusters; i++) {
            if(fat_file (i) == fileno) {
                set_fat_file(i, fcl + 0x800);
            }
        }
        nent.d_fileno = swapword(fcl + 0x801);
        /*
         * fs->fnew is the file no (start cluster (+0x800))
         * of directory
             */

        /* Now write nent to new-ish directory */

        buf = xmalloc(gSectorSize * gSectorsPerBlock);
        cwdlen = swaplong(fs->nde->d_length);
        dcl = fs->fnew;
        if((cwdlen % gSectorSize * gSectorsPerBlock) == 0) {
            dcl = alloc_new_cluster(dcl,
                                    cwdlen / (gSectorSize * gSectorsPerBlock), 0);
            *(fs->nptr) = *(fs->nptr) + 1;
        }
        else if(cwdlen > 64) {
            read_cluster(buf, dcl);
        }
        memcpy(buf+cwdlen, &nent, 64);
        write_cluster(buf, dcl);
        fs->nde->d_length = swaplong(cwdlen + 64);
        free(buf);
    }
    else if(entry->d_type == 255) {
        RecurseDir(fileno, swaplong(entry->d_length), fs,
                   (int (*) (QLDIR *, int, void *))ProcessSubFile);
    }

    return 0;
}

int MoveSubFiles(FSBLK *fs) {
    QLDIR *entry;
    short d;

    for (d = 1; d < maxdir (); d++) {
        entry = pdir + d;
        if (swaplong(entry->d_length) + swapword (entry->d_szname) != 0) {
            ProcessSubFile(entry, d, fs);
        }
    }
    return 0;
}

void writefile(char *fn, short dflag) {
    uint8_t *datbuf;
    int filenew, i, fl;
    QLDIR *entry;
    unsigned long free_sect;
    long y;
    uint32_t qdsize = 0;
    int enddat;
    int block = 0, nblock = 0;
    char *qlnam;
    short nlen;
    QLDIR *vers = NULL;
    SDL *sdl = NULL;
    short flag = 0;
    short diroff = 0;
    short nvers = -1;
    time_t t;
    struct stat s;
    short blksiz = gSectorSize * gSectorsPerBlock;
    ssize_t ignore __attribute__((unused));

    qlnam = MakeQLName(fn, &nlen);

    if(dflag != 255) {
        if(stat(fn, &s) == 0) {
            if(s.st_size == 16) {
                int fd;
                char tbuf[16];

                if((fd = open(fn, O_RDONLY|O_BINARY)) > -1) {
                    ignore = read(fd, tbuf, 16);
                    if(memcmp(tbuf, QLDIRSTRING,16) == 0) {
                        dflag = 255;
                    }
                    close (fd);
                }
            }
        }
        else {
            fprintf(stderr, "Can't stat file %s\n", fn);
            exit(errno);
        }
    }


    if (list) {
        if(isatty(1)) fputs (fn, stderr);
    }

    if ((fl = match_file (qlnam, &vers, &sdl)) != 0) {
        if (dflag == 255) {
            fputs("Exists\n", stderr);
            exit(EEXIST);
        }
        else {
            if(!isatty(1)) {
                OWopt = 'A';
            }
            if (OWopt != 'A') {
                fprintf(stderr, "file %s exists, overwrite [Y/N/A/Q] : ", qlnam);
                do {
                    OWopt = getch ();
                    OWopt = toupper (OWopt);
                }
                while (strchr("\003ANYQ", OWopt) == NULL) /* Do nothing */ ;
                fprintf(stderr, "%c\n", OWopt);
                if (OWopt == 'N') {
                    return;
                }
                else {
                    if (OWopt == 'Q' || OWopt == 3) {
                        exit(0);
                    }
                }
            }
            nvers = swapword(vers->d_version);
            del_file (fl, vers, sdl);
        }
    }

    if (dflag == 255) {
        y = 64;
    }
    else {

        if ((fl = open(fn, O_RDONLY | O_BINARY)) == -1) {
            perror("write file: could not open input file ");
            exit(errno);
        }
        y = s.st_size + 64; {
            uint32_t stuff[2];
            lseek(fl, -8, SEEK_END);
            ignore = read (fl, stuff, 8);
            if (*stuff == *(uint32_t *) "XTcc") {
                qdsize = *(stuff + 1);
            }
        }

        err = lseek(fl, 0, SEEK_SET);	/* reposition to zero!!! */
        if (err < 0)
            perror("Write file: lseek() on input file : ");
    }

    free_sect = swapword(b0->q5a_free_sectors);
    if (y > free_sect * gSectorSize) {
        fprintf(stderr, " file %s too large (%ld %ld)\n", fn, y, free_sect);
        exit(ENOSPC);
    }

    if (sdl == NULL) {
        sdl = CheckDirLevel(qlnam, strlen(qlnam));
    }

    entry = GetNewDirEntry(sdl, &filenew, &nblock, &diroff);
    memset(entry, '\0', 64);

    if (filenew & 0x800) {
        flag = filenew - 0x800;
    }
    else {
        flag = 0;
    }

    entry->d_length = swaplong(y);
    entry->d_fileno = swapword(filenew + 1);

    if (dflag != 255) {
        if (qdsize == 0) {
            entry->d_type = 0;
            entry->d_datalen = 0;
        }
        else {
            entry->d_type = 1;
            entry->d_datalen = qdsize;	/* big endian already */
        }
    }

    memcpy(entry->d_name, qlnam, nlen);
    entry->d_szname = swapword(nlen);

    t = time(NULL) + TIME_DIFF + timeadjust;

    entry->d_update = entry->d_backup = swaplong (t);
    nvers++;
    entry->d_version = swapword (nvers);

    if (dflag == 255) {
        filenew = alloc_new_cluster(filenew, 0, flag);
        nblock++;
    }
    else {
        int nwr = 0;

        datbuf = xmalloc(blksiz);
        enddat = (y < blksiz) ? y : blksiz;
        err = read (fl, datbuf + 64, enddat - 64);

        memcpy(datbuf, entry, 64);
        for (block = 0; y > 0;) {
            i = alloc_new_cluster(filenew, block, flag);
            flag = 0;

            if (i < 0) {
                fprintf(stderr, " filewrite failed : no free cluster\n");
                exit(ENOSPC);
            }
            block++;
            nblock++;

            if(list) {
                nwr += err;
                if(isatty(1)) {
                    fprintf(stderr,"%8d\b\b\b\b\b\b\b\b", nwr);
                    fflush(stderr);
                }
                else {
                    printf("%8d\n", nwr);
                    fflush(stdout);
                }
            }

            write_cluster(datbuf, i);

            y -= blksiz;

            err = read(fl, datbuf, blksiz);
            if (err < 0)
                perror("Write file: read on input file:");
        }
        close (fl);
        free(datbuf);
    }

    free (qlnam);

    b0->q5a_eod_block = swapword(directoryEndBlock);
    b0->q5a_eod_byte = swapword(directoryEndByte);

    if (dflag == 255) {
        FSBLK fs;
        fs.nde = entry;
        fs.nptr = &nblock;
        fs.fnew = filenew;
        MoveSubFiles(&fs);
        entry->d_type = 255;
    }

    b0->q5a_free_sectors = swapword(free_sect - nblock * gSectorsPerBlock);

    write_b0fat();
    dir_write_back(entry, sdl, &diroff);

    if (sdl) {
        SDL *msdl = NULL;
        QLDIR *mentry;
        char dirname[40];

        memcpy(dirname, sdl->name, sdl->szname);
        *(dirname + sdl->szname) = '\0';
        if ((fl = match_file(dirname, &mentry, &msdl)) != 0) {
            mentry->d_length = swaplong(sdl->flen);
            mentry->d_update = entry->d_update;
            dir_write_back(mentry, msdl, NULL);
        }
    }

    if(list && isatty(1))
        fputc('\n', stderr);

    if(dflag == 255) {
        AddSlist(entry, FileXDir(entry->d_fileno));
    }
}

/* ARGSUSED */
int AddDirEntry(QLDIR * entry, int fileno, void *flag) {
    if (entry->d_type == 255) {
        SDL *sdl = AddSlist(entry, fileno);
        RecurseDir(fileno, sdl->flen, NULL, AddDirEntry);
    }
    return 0;
}

void BuildSubList(void) {
    short d;
    QLDIR *entry;
    long flen;

    for (d = 1; d < maxdir (); d++) {
        entry = pdir + d;
        flen = swaplong(entry->d_length);

        if (flen + swapword(entry->d_szname) != 0) {
            if (entry->d_type == 255) {
                AddDirEntry(entry, d, NULL);
            }
        }
    }
}

void read_dir(void) {
    int i, fn, cl;

    for (i = 0; i < gNumberOfClusters; i++) {
        cl = fat_cl (i);
        fn = fat_file (i);

        if (fn == 0) {
            block_dir[block_max] = i;
            block_max++;
            read_cluster((char *) pdir + gSectorSize * gSectorsPerBlock * cl, i);
        }
    }
    BuildSubList ();
}

void print_map(void) {
    int i, fn, cl;
    QLDIR *entry;
    short flag;

    printf ("\nblock\tfile\tpos\n\n");

    for (i = 0; i < gNumberOfClusters; i++) {
        cl = fat_cl (i);
        fn = fat_file (i);

        printf("%d\t%d\t%d\t(%03x, %03x) ", i, fn, cl, fn, cl);

        if ((fn & 0xFF0) == 0xFD0 && (fn & 0xf) != 0xF) {
            printf("erased %2d\t", fn & 0xF);
        }

        entry = NULL;
        switch (fn) {
        case 0x000:
            printf("directory");
            break;
        case 0xF80:
            printf("map");
            break;
        case 0xFDF:
            printf("unused");
            break;
        case 0xFEF:
            printf("bad");
            break;
        case 0xFFF:
            printf("not existent");
            break;
        default:
            entry = GetEntry(fn);
            break;
        }
        if (entry) {
            flag = 2;
            print_entry(entry, fn, &flag);
        }
        else {
            putc('\n', stdout);
        }
    }
}

void set_header(int ni, long h, QLDIR * entry, SDL * sdl) {
    int i;
    uint8_t *b = xmalloc (gSectorsPerBlock * gSectorSize);

    if (swaplong(entry->d_length) + swapword(entry->d_szname) == 0) {
        fprintf(stderr, "file deleted ??\n");
        exit(EBADF);
    }

    i = FindCluster(ni, 0);

    read_cluster(b, i);
    entry->d_type = ((QLDIR *) b)->d_type = 1;
    entry->d_datalen = ((QLDIR *) b)->d_datalen = swaplong(((h+1) & 0xFFFE));
    write_cluster(b, i);
    free (b);
    dir_write_back(entry, sdl, NULL);
}

void dir_write_back(QLDIR * entry, SDL * sdl, short *off) {

    if (sdl) {
        UpdateSubEntry(entry, sdl, off);
    }
    else {
        int i;

        for (i = 0; i < block_max; i++)
            write_cluster(pdir + gDirEntriesPerBlock * gSectorsPerBlock * i, block_dir[i]);
    }
}

int find_free_cluster(void) {
    short fflag, i;

    for (i = lac + 1; i < gNumberOfClusters; i++) {
        fflag = fat_file(i);
        if ((fflag >> 4) == 0xFD) {
            break;
        }
    }
    return (i < gNumberOfClusters ? i : -1);
}

int alloc_new_cluster(int fnum, int iblk, short flag) {
    short i;

    if (flag) {
        i = flag;
    }
    else {
        i = find_free_cluster();
    }

    if (i != -1) {
        set_fat_file(i, fnum);
        set_fat_cl(i, iblk);
        lac = i;
    }
    return i;
}


void free_cluster(long i) {
    int fn, dfn;

    if (i > 0) {
        fn = fat_file(i);
        dfn = 0xFD0 | (0xf & fn);
        set_fat_file(i, dfn);
    }
    else {
        fprintf(stderr, "freeing cluster 0 ??? !!!\n");
        exit(EBADF);
    }
}

void format(char *frmt, char *argfname) {
    static char ltp_dd[] = {
        0, 3, 6, 0x80, 0x83, 0x86, 1, 4,
        7, 0x81, 0x84, 0x87, 2, 5, 8, 0x82, 0x85, 0x88
    };

    static char ptl_dd[] = {
        0, 6, 0x0c, 1, 7, 0x0d,
        2, 8, 0x0e, 3, 9, 0x0f, 4, 0x0a, 0x10, 5, 0x0b, 0x11
    };

    static char ltp_hd[] = {
        0, 2, 4, 6, 8, 0xa, 0xc, 0xe, 0x10,
        0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90,
        1, 3, 5, 7, 9, 0xb, 0xd, 0xf, 0x11,
        0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d, 0x8f, 0x91
    };

    static char ltp_ed[] = {
        0, 2, 4, 6, 8, 0x80, 0x82, 0x84, 0x86, 0x88,
        1, 3, 5, 7, 9, 0x81, 0x83, 0x85, 0x87, 0x89
    };

    int cls;
    time_t t;
    char formatRequested = *frmt;
    int startCluster = 0;

    /* We must be sure we have a valid format! */
    if ((formatRequested != 'h') &&
        (formatRequested != 'e') &&
        (formatRequested != 'd')) {
        usage("Invalid format requested");
    }

    /* Good for DD and HD floppies. */
    gSectorSize = 512;

    /* Not for ED floppies though! */
    if (formatRequested == 'e')
        gSectorSize = 2048;

    t = time (NULL);
    b0->q5a_random = swapword(t & 0xffff);
    if(argfname == NULL) {
        argfname = "";
    }

    ZeroSomeSectors(fd, *frmt);

    /* Common Stuff goes here regardless of which format */
    /* Most of what follows assumes HD or ED in use. */
    /* Which will be corrected if a DD is supplied. */

    ql5a = 0;
    gNumberOfSides = 2;
    memcpy(b0, "QL5B          ", 14);
    memcpy(b0->q5a_medium_name, argfname,
                (strlen(argfname) <= 10 ? strlen(argfname) : 10));

    gNumberOfTracks = 80;
    b0->q5a_tracks = swapword(gNumberOfTracks);

    b0->q5a_eod_block = 0;
    b0->q5a_eod_byte = swapword(64);

    /* Common to DD and HD only */
    gSectorsPerBlock = 3;
    b0->q5a_allocation_blocks = swapword(gSectorsPerBlock);

    if (*frmt == 'd')		/* 720 K format */ {
        ql5a = 1;
        memcpy(b0, "QL5A", 4);
        memcpy(b0->q5a_log_to_phys, ltp_dd, 18);
        memcpy(b0->q5a_phys_to_log, ptl_dd, 18);

        gSectorsPerTrack = 9;
        b0->q5a_sectors_track = swapword(gSectorsPerTrack);

        gSectorsPerCylinder = gSectorsPerTrack * gNumberOfSides;
        b0->q5a_sectors_cyl = swapword(gSectorsPerCylinder);

        gOffsetCylinder = 5;
        b0->q5a_sector_offset = swapword(gOffsetCylinder);

        b0->q5a_free_sectors = swapword(1434);
        b0->q5a_good_sectors = b0->q5a_total_sectors = swapword(1440);

        set_fat_file(1, 0);	/*  ...  for directory */
        set_fat_cl(1, 0);

        startCluster = 2;
    }
    else if (*frmt == 'h') {

        memcpy(b0->q5a_log_to_phys, ltp_hd, 36);

        gSectorsPerTrack = 18;
        b0->q5a_sectors_track = swapword(gSectorsPerTrack);

        gSectorsPerCylinder = gSectorsPerTrack * gNumberOfSides;
        b0->q5a_sectors_cyl = swapword(gSectorsPerCylinder);

        gOffsetCylinder = 2;
        b0->q5a_sector_offset = swapword(gOffsetCylinder);

        b0->q5a_free_sectors = swapword(2871);
        b0->q5a_good_sectors = b0->q5a_total_sectors = swapword(2880);


        set_fat_file(1, 0xf80);
        set_fat_cl(1, 1);
        set_fat_file(2, 0);	/*  ...  for directory */
        set_fat_cl(2, 0);

        startCluster = 3;
    }
    else if (*frmt == 'e') {

        /* ED floppy, bigger sector size. */
        gSectorSize = 2048;

        memcpy(b0->q5a_log_to_phys, ltp_ed, 20);

        gSectorsPerTrack = 10;
        b0->q5a_sectors_track = swapword(gSectorsPerTrack);

        gSectorsPerCylinder = gSectorsPerTrack * gNumberOfSides;
        b0->q5a_sectors_cyl = swapword(gSectorsPerCylinder);

        gOffsetCylinder = 2;
        b0->q5a_sector_offset = swapword(gOffsetCylinder);

        b0->q5a_free_sectors = swapword(1596);
        b0->q5a_good_sectors = b0->q5a_total_sectors = swapword(1600);

        gSectorsPerBlock = 1;
        b0->q5a_allocation_blocks = swapword(gSectorsPerBlock);

        set_fat_file(1, 0xf80);
        set_fat_cl(1, 1);

        set_fat_file(2, 0xf80);
        set_fat_cl(2, 2);

        set_fat_file(3, 0);	/*  ...  for directory */
        set_fat_cl(3, 0);

        startCluster = 4;
    }

    /* More common stuff needs done after the above */
    /* Everything here is common to all formats */
    gDirEntriesPerBlock = gSectorSize / DIRENTRYSIZE;
    gNumberOfClusters = gNumberOfTracks * gSectorsPerCylinder / gSectorsPerBlock;

    set_fat_file(0, 0xF80);	/* FAT entry for FAT */
    set_fat_cl(0, 0);

    for (cls = startCluster; cls < gNumberOfClusters; cls++)
    {   /* init rest of FAT */
        set_fat_file(cls, 0xFDF);
        set_fat_cl(cls, 0xFFF);
    }


    make_convtable (0);
    write_b0fat ();
}

int main(int argc, char **argv) {
    int i=0;
    QLDIR *entry;
    SDL *sdl;
    mode_t mode = O_RDONLY;
    mode_t permissions = 0;
    int dofmt = 0;
    long np1 = 0, np2 = 0;
    char *pd,dev[64];

    if (argc < 2) {
        usage ("too few parameters");
    }

    for (i =1; i < argc; i++) {
        if ((argv[i][0] == '-')
#ifdef DOS_LIKE
                || (argv[i][0] == '/')
#endif
           ) {
            switch (argv[i][1]) {
            case 'f':
                dofmt = i;
            case 'x':
            case 'r':
            case 'w':
            case 'W':
            case 'm':
            case 'M':
                mode = O_RDWR;
                i = argc;
                break;
            case 'v':
                puts("QLTOOLS for "OSNAME" (version "VERSION")");
                exit(0);
                break;
            }

            // Did we request a format? Allow file creation if not there.
            if (dofmt) {
                mode |= O_CREAT;
                permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            }
        }
    }

    pd = argv[1];

#if defined(__NT__) || defined(__MINGW32__) || defined(__WIN32__)
    if(*(pd+1) ==  ':' && isalpha(*pd)) {
        strcpy(dev, "\\\\.\\a:");
        *(dev+4) = *pd;
        pd = dev;
    }
#endif
#ifdef __linux__
    if(*(pd+1) ==  ':' && isalpha(*pd)) {
        strcpy(dev, "/dev/fd0");
        *(dev+7) += tolower(*pd) - 'a';
        pd = dev;
    }
#endif

    fd = OpenQLDevice(pd, mode, permissions);

    if ((int) fd < 0) {
        perror("Could not open image file");
        usage("image file not opened");
    }

    timeadjust = GetTimeZone();

    if ((b0 = xmalloc (GSSIZE * MAXALB)) != NULL) {
        if (dofmt) {
            format (argv[dofmt] + 2, argv[dofmt + 1]);
            CloseQLDevice(fd);
            /* Gcc creates without write permissions */
            chmod(pd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            exit (0);
        }

        /* As MAXALB is defined in terms of 512 byte sectors, we need this.
         * before we read the FAT
         */
        gSectorSize = GSSIZE;
        read_b0fat (0);

        pdir = xmalloc(gSectorSize * gSectorsPerBlock * (directoryEndBlock + 6));
        read_dir ();

        for (i = 2; i < argc; i++) {
            char c = 0;

            if ((argv[i][0] == '-')
#ifdef DOS_LIKE
                || (argv[i][0] == '/')
#endif
              ){
                OWopt = 0;
                switch (c = argv[i][1]) {
                case 'l':
                    list = 1;
                    break;
                case 't':
                    tranql ^= 1;
                    break;
                case 'U':
                case 'u':
                    if (argv[i][2]) {
                        np1 = atol(argv[i] + 2);
                    }
                    else {
                        i++;
                        np1 = atol(argv[i]);
                    }
                    dump_cluster(np1, c == 'U');
                    break;
                case 'd':
                    print_dir(0);       // Print_dir with stats.
                    break;
                case 's':
                    print_dir(1);       // Print dir with no stats.
                    break;
                case 'i':
                    print_info();
                    break;
                case 'm':
                    print_map();
                    break;
                case 'c':
                    make_convtable(1);
                    break;
                case 'n':
                    i++;
                    np1 = match_file(argv[i], &entry, NULL);
                    if (np1) {
                        cat_file(np1, entry);
                    }
                    else {
                        fprintf(stderr, "Invalid file\n");
                    }
                    break;
                case 'W':
                    OWopt = 'A';
                case 'w':
                    while (argv[i + 1] && *argv[i + 1] != '-') {
                        i++;
                        writefile(argv[i], 0);
                    }
                    break;
                case 'r':
                    i++;
                    np1 = match_file(argv[i], &entry, &sdl);
                    if (np1) {
                        del_file(np1, entry, sdl);
                    }
                    break;
                case 'M':
                    i++;
                    writefile(argv[i], 255);
                    break;

                case 'x':
                    i++;
                    np1 = match_file(argv[i], &entry, &sdl);
                    if (np1) {
                        i++;
                        np2 = strtol(argv[i], NULL, 0);
                        if (np2) {
                            set_header(np1, np2, entry, sdl);
                        }
                    }
                    break;
                default:
                    usage ("bad option");
                    break;
                }
            }
            else {
                np1 = match_file(argv[i], &entry, NULL);
                if (np1) {
                    cat_file(np1, entry);
                }
            }
        }
        CloseQLDevice(fd);
    }
    return (0);
    /* This is to prevent compiler warning !! */
    /* NOTREACHED */
    (void)rcsid;
}
