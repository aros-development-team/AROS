/* dos.c */

/* <project_name> -- <project_description>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"
#include "rdb.h"
#include "dos.h"

typedef struct {
    int (*load_file) (void *this, char *filename, void *buffer);
    void (*destroy) (void *this);
    struct RdbPartition *partition;
    char *buff;
    uint32_t rootblock;
    uint32_t blocksize;
    int      capabilities;
} dos_boot_dev_t;

#define isDIRCACHE(self) ((self)->capabilities & 4) 
#define isINTL(self)     ((self)->capabilities & 2)
#define isFFS(self)      ((self)->capabilities & 1)

#define DOS_ID  (('D' << 24) | ('O' << 16) | ('S' << 8))

struct DosBootBlock {
    uint32_t    id;
    uint32_t    checksum;
    uint32_t    rootblock;
};

#define DBB(buf)        ((struct DosBootBlock *)(buf))

#define HT_SIZE         72
#define BM_SIZE         25
#define MAXNAMELEN      30

#define T_HEADER        2
#define ST_ROOT         1
#define ST_DIR          2
#define ST_FILE         -3
#define ST_LFILE        -4
#define ST_LDIR         4
#define ST_LSOFT        3
#define T_LIST          16
#define T_DATA          8
#define T_DIRC          33

struct DosDirBlock {
    uint32_t    type;
    uint32_t    header_key;
    uint32_t    high_seq;
    uint32_t    hash_table_size;
    uint32_t    first_data;
    uint32_t    checksum;
    uint32_t    hash_table[HT_SIZE];
    uint32_t    bitmap_flag;
    uint32_t    bitmap_page[BM_SIZE];
    uint32_t    bitmap_ext;
    uint32_t    ctime_days;
    uint32_t    ctime_mins;
    uint32_t    ctime_ticks;
    uint8_t     name_len;
    char        name[MAXNAMELEN+1];
    uint8_t     resv[8];
    uint32_t    atime_days;
    uint32_t    atime_mins;
    uint32_t    atime_ticks;
    uint32_t    cotime_days;
    uint32_t    cotime_mins;
    uint32_t    cotime_ticks;
    uint32_t    next_same_hash;
    uint32_t    parent;
    uint32_t    extension;
    uint32_t    sub_type;
};

#define DDB(buf)        ((struct DosDirBlock *)(buf))

#define MAX_DATABLK     72
#define MAXCOMMENTLEN   79

struct DosFileBlock {
    uint32_t    type;
    uint32_t    header_key;
    uint32_t    high_seq;
    uint32_t    hash_table_size;
    uint32_t    first_data;
    uint32_t    checksum;
    uint32_t    data_blocks[MAX_DATABLK];
    uint32_t    resv[2];
    uint32_t    access;
    uint32_t    byte_size;
    uint8_t     comment_len;
    char        comment[MAXCOMMENTLEN+1];
    uint8_t     resv_1[11];
    uint32_t    days;
    uint32_t    mins;
    uint32_t    ticks;
    uint8_t     name_len;
    char        name[MAXNAMELEN+1];
    uint32_t    resv_2;
    uint32_t    real;
    uint32_t    next_link;
    uint32_t    resv_3[5];
    uint32_t    next_same_hash;
    uint32_t    parent;
    uint32_t    extension;
    uint32_t    sub_type;
};

#define DFB(buf)        ((struct DosFileBlock *)(buf))

static int dos_loadsector(dos_boot_dev_t *self, uint32_t block, void *buffer)
{
#if DEBUG
    printf("dos_loadsector(%d) => 0x%08x\n", block, (self->partition->info->start
                          + (block * (self->blocksize / 512))) * 512);
#endif
    return loadsector(self->partition->info->start
              + (block * (self->blocksize / 512)),
              self->partition->info->blksz, self->blocksize / 512, 
              buffer);
}

static inline char intlupper(char in)
{
    unsigned char c = (unsigned char)in;
    return (c>='a' && c<='z') || (c>=224 && c<=254 && c!=247) ? c - ('a'-'A') : c;
}

static inline char dosupper(char in)
{
    unsigned char c = (unsigned char)in;
    return (c>='a' && c<='z') ? c - ('a'-'A') : c;
}

static int dos_strncmp(dos_boot_dev_t * self, const char *a, const char *b, int len)
{
    int rc = 0;

    while (len && rc == 0 ) {
        if (!*a && !*b)
            return 0;
        if (isINTL(self))
            rc = intlupper(*a) - intlupper(*b);
        else
            rc = dosupper(*a) - dosupper(*b);
        a++;
        b++;
        len--;
    }

    return rc;
}

static int dos_hash(dos_boot_dev_t * self, const char *name)
{
    uint32_t hash, len;
    char upper;
    int i;
    
    len = hash = strlen(name);
    for (i = 0; i < len; i++) {
        if (isINTL(self))
            upper = intlupper(name[i]);
        else
            upper = dosupper(name[i]);
        hash = (hash * 13 + (unsigned char)upper) & 0x7ff;
    }

    return hash % HT_SIZE;
}

static int dos_loadfile(dos_boot_dev_t * self, char *filename, void *filebuffer)
{
    char path[256], *fp, *dp;
    uint8_t *buff;

    strncpy(path, filename, sizeof(path));
    path[sizeof(path)-1] = 0;

    dp = path;

    buff = self->buff;
    dos_loadsector(self, self->rootblock, buff);

    while (dp) {
        uint32_t sector;
        int dp_len;

        fp = strchr(dp, '/');
        if (fp)
            *(fp++) = 0;

        dp_len = strlen(dp);
            
        sector = DDB(buff)->hash_table[dos_hash(self, dp)];

        while (sector != 0) {
            dos_loadsector(self, sector, buff);

            if (DDB(buff)->type != T_HEADER) {
                printf(".. corrupt T_HEADER at sector %d\n", sector);
                return -1;
            }

            if ((DDB(buff)->name_len == dp_len) &&
                (dos_strncmp(self, dp, DDB(buff)->name, dp_len) == 0)) {
                break;
            }

            sector = DDB(buff)->next_same_hash;
        }

        /* No such file */
        if (sector == 0)
            return -1;

        printf(".. found %s, %d\n", dp, DDB(buff)->sub_type);

        /* Header is in 'buff' at this point */
        if (DDB(buff)->sub_type == ST_FILE) {
            int total, size, block, blocks;
            if (fp != NULL) {
                printf(".. was expecting a directory\n");
                return -1;
            }
            total = size = DFB(buff)->byte_size;
            sector = DFB(buff)->first_data;

            /* FIXME: OFS support - this code is FFS only! */
            dos_loadsector(self, sector, filebuffer);
            filebuffer += self->blocksize;
            size -= self->blocksize;

            blocks = (size + self->blocksize - 1) / self->blocksize;
            for (block = 0; block < blocks; block++) {
                if (block == MAX_DATABLK) {
                    blocks -= MAX_DATABLK;
                    block = 0;
                    dos_loadsector(self, DFB(buff)->extension, buff);
                }
                dos_loadsector(self, DFB(buff)->data_blocks[(MAX_DATABLK - 1) - block], filebuffer);
                filebuffer += self->blocksize;
            }
            return total;
        }

        /* Header is in 'buff' at this point */
        if (DDB(buff)->sub_type != ST_DIR) {
            return -1;
        }

        dp = fp;
    }

    return -1;
}

static int dos_destroy(dos_boot_dev_t * this)
{
    free(this->buff);
    free(this);

    return 0;
}


boot_dev_t *dos_create(struct RdbPartition *partition)
{
    dos_boot_dev_t *boot;
    void *buffer;


    boot = malloc(sizeof(dos_boot_dev_t));
    boot->partition = partition;

    boot->buff = buffer = malloc(64 * 512);
    loadsector(boot->partition->info->start, boot->partition->info->blksz, 
           16, buffer);
    if ((DBB(buffer)->id & 0xffffff00) != DOS_ID) {
        printf("** Bad dos partition or disk - %d:%d **\n",
               boot->partition->disk, boot->partition->partition);
        free(buffer);
        free(boot);
        return NULL;
    }

    boot->rootblock = (boot->partition->info->size+1)/2;
    boot->blocksize = boot->partition->info->blksz;

    dos_loadsector(boot, boot->rootblock, buffer);
    printf("sizeof(DDB) = %d\n", sizeof(*DDB(buffer)));
    printf("  type = %d, sub_type = %d\n", DDB(buffer)->type, DDB(buffer)->sub_type);

    if (DDB(buffer)->type != T_HEADER || DDB(buffer)->sub_type != ST_ROOT) {
        printf("** Root node not found - %d:%d **\n",
               boot->partition->disk, boot->partition->partition);
        free(buffer);
        free(boot);
        return NULL;
    }

    printf("Found dos partition! Name: %*s\n",
           DDB(buffer)->name_len, (char *)DDB(buffer)->name);

    boot->load_file = (int (*)(void *, char *, void *))dos_loadfile;
    boot->destroy = (void (*)(void *))dos_destroy;

    return (boot_dev_t *) boot;
}

