#ifndef _DIRENT_H_
#define _DIRENT_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: header file dirent.h
    Lang: english
*/

#include <sys/types.h>

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

struct dirent
{
#define	MAXNAMLEN NAME_MAX
    char d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
};

/* structure describing an open directory. */
typedef struct _dirdesc
{
   int    fd;
   struct dirent ent;
   off_t  pos;
   void   *priv;
} DIR;


DIR *opendir (const char *name);
struct dirent *readdir (DIR *dir);
void rewinddir (DIR *dir);
int closedir (DIR *dir);

off_t telldir (const DIR *dir);
void seekdir (DIR *dir, off_t offset);
int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **));

int alphasort(const struct dirent **a, const struct dirent **b);
ssize_t getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep);

#endif /* !_DIRENT_H_ */
