#ifndef _DIRENT_H_
#define _DIRENT_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: header file dirent.h
    Lang: english
*/

#include <sys/_types.h>
#include <sys/cdefs.h>

#ifndef __AROS_INO_T_DECLARED
#define __AROS_INO_T_DECLARED
typedef __ino_t ino_t;
#endif

#ifndef __AROS_OFF_T_DECLARED
#define __AROS_OFF_T_DECLARED
typedef __off_t off_t;
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

struct dirent
{
    ino_t   d_ino;
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

#if __POSIX_VISIBLE >= 200112
/* NOTIMPL int readdir_r(DIR * restrict dir , struct dirent * restrict entry,
        struct dirent * restrict result); */
#endif

#if __XSI_VISIBLE
void seekdir(DIR *dir, long loc);
long telldir(DIR *dir);
#endif

#if __BSD_VISIBLE

#ifndef __AROS_SSIZE_T_DECLARED
#define __AROS_SSIZE_T_DECLARED
typedef __ssize_t ssize_t;
#endif

/* NOTIMPL int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **)); */

/* NOTIMPL int alphasort(const struct dirent **a, const struct dirent **b); */
/* NOTIMPL ssize_t getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep); */
#endif

__END_DECLS

#endif /* !_DIRENT_H_ */
