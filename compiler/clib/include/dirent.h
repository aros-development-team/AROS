#ifndef _DIRENT_H_
#define _DIRENT_H_

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: header file dirent.h
    Lang: english
*/

#include <sys/cdefs.h>

#include <sys/types/ino_t.h>
#include <sys/types/off_t.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

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
    unsigned short int d_reclen;
    unsigned char d_type;
#define	MAXNAMLEN NAME_MAX
    char    d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
};

/* structure describing an open directory. */
typedef struct _dirdesc
{
   int    fd;
   struct dirent ent;
   off_t  pos;
   void   *priv;
} DIR;

__BEGIN_DECLS

int closedir(DIR *dir);
DIR *opendir(const char *filename);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
int dirfd(DIR *dir);

#if __POSIX_VISIBLE >= 200112
/* NOTIMPL int readdir_r(DIR * restrict dir , struct dirent * restrict entry,
        struct dirent * restrict result); */
#endif

#if __XSI_VISIBLE
void seekdir(DIR *dir, off_t loc);
long telldir(DIR *dir);
#endif

#if __BSD_VISIBLE

#include <sys/types/ssize_t.h>

/* NOTIMPL int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **)); */

/* NOTIMPL int alphasort(const struct dirent **a, const struct dirent **b); */
/* NOTIMPL ssize_t getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep); */
#endif

__END_DECLS

#endif /* !_DIRENT_H_ */
