#ifndef _POSIXC_DIRENT_H_
#define _POSIXC_DIRENT_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file dirent.h
*/

#include <aros/features.h>
#include <aros/system.h>

/* FIXME: Is this allowed ? */
#include <limits.h>

#include <aros/types/ino_t.h>
#include <aros/types/off_t.h>

//#ifndef _POSIX_SOURCE
/* These defines seems quite common althoug not defined by POSIX.1-2008
   so we define these by default
*/

/* d_type */
#define DT_UNKNOWN     0
#define DT_FIFO        1
#define DT_CHR         2
#define DT_DIR         4
#define DT_BLK         6
#define DT_REG         8
#define DT_LNK        10
#define DT_SOCK       12
#define DT_WHT        14

//#endif /* !_POSIX_SOURCE */

struct dirent
{
#if defined(__USE_FILE_OFFSET64)
    __ino64_t d_fileno;
    __off64_t d_off;
#else
    __ino_t d_fileno;
    __off_t d_off;
#endif
    unsigned short int d_reclen;
    unsigned char d_type;
    /* it is not allowed to include limits.h, so we must
     * hard code the size here. names must be no longer
     * than this */
    char    d_name[PATH_MAX + 1];	
};

#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_TYPE

/* defined for backwards compatibility..  */
#define d_ino        d_fileno

struct dirent64
{
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
};

/* structure describing an open directory. */
struct __dirdesc;
typedef struct __dirdesc DIR;

__BEGIN_DECLS

int alphasort(const struct dirent **a, const struct dirent **b);
int alphasort64(const struct dirent64 **a, const struct dirent64 **b);
int closedir(DIR *dir);
int dirfd(DIR *dir);
DIR *fdopendir(int fd);
DIR *opendir(const char *filename);
struct dirent *readdir(DIR *dir);
struct dirent64 *readdir64 (DIR *dir);
/* NOTIMPL int readdir_r(DIR *restrict, struct dirent *restrict, struct dirent **restrict); */
/* NOTIMPL int readdir64_r (DIR *dir,
                        struct dirent64 *entry,
                        struct dirent64 **result) */
void rewinddir(DIR *dir);
int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **));
int scandir64 (const char *dir,
                      struct dirent64 ***namelist,
                      int (*select) (const struct dirent64 *),
                      int (*compar) (const struct dirent64 **,
                                    const struct dirent64 **));
void seekdir(DIR *dir, off_t loc);
long telldir(DIR *dir);

__END_DECLS

#endif /* !_POSIXC_DIRENT_H_ */
