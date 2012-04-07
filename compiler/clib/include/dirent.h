#ifndef _DIRENT_H_
#define _DIRENT_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: header file dirent.h
    Lang: english
*/

#include <aros/system.h>

#include <aros/types/ino_t.h>
#include <aros/types/off_t.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#define	MAXNAMLEN NAME_MAX

#define DT_UNKNOWN     0
#define DT_FIFO        1
#define DT_CHR         2
#define DT_DIR         4
#define DT_BLK         6
#define DT_REG         8
#define DT_LNK        10
#define DT_SOCK       12
#define DT_WHT        14

struct dirent
{
    ino_t   d_ino;
    char    d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
    unsigned short int d_reclen;
    unsigned char d_type;
};

/* structure describing an open directory. */
struct __dirdesc;
typedef struct __dirdesc DIR;

__BEGIN_DECLS

int closedir(DIR *dir);
DIR *opendir(const char *filename);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
int dirfd(DIR *dir);

/* NOTIMPL int readdir_r(DIR * restrict dir , struct dirent * restrict entry,
        struct dirent * restrict result); */

void seekdir(DIR *dir, off_t loc);
long telldir(DIR *dir);

#if __BSD_VISIBLE

#include <aros/types/ssize_t.h>

/* NOTIMPL int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **)); */

/* NOTIMPL int alphasort(const struct dirent **a, const struct dirent **b); */
/* NOTIMPL ssize_t getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep); */
#endif

__END_DECLS

#endif /* !_DIRENT_H_ */
