#ifndef _POSIXC_DIRENT_H_
#define _POSIXC_DIRENT_H_

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file dirent.h
*/

#include <aros/features.h>
#include <aros/system.h>

#include <aros/types/ino_t.h>
#include <aros/types/off_t.h>

#if defined(__USE_NIXCOMMON)

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

#endif /* !_POSIX_SOURCE */

/* NB we must use the __xxx_t types here, because the xxx_t version
 * may not be defined */
struct dirent
{
#if defined(__USE_FILE_OFFSET64)
    __ino64_t d_ino;
    __off64_t d_off;
#else
    __ino_t d_ino;
    __off_t d_off;
#endif
    unsigned short int d_reclen;
    unsigned char d_type;
    /* it is not allowed to include limits.h, so we must
     * hard code the size here. names must be no longer
     * than this */
    char    d_name[256];	
};

#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_TYPE

/* defined for backwards compatibility..  */
#define d_fileno        d_ino

#if defined(__USE_LARGEFILE64)
struct dirent64
{
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
};
#endif

/* opaque structure describing directory stream objects */
struct __dirdesc;
typedef struct __dirdesc DIR;

__BEGIN_DECLS

#if defined(__USE_XOPEN2K8)
/* NOTIMPL DIR *fdopendir(int); */
#endif

DIR *opendir (const char *filename);
int closedir(DIR *dir);
#if !defined(NO_POSIX_WRAPPERS)
struct dirent *posixc_readdir (DIR *dir);
#if defined(__USE_LARGEFILE64)
struct dirent64 *posixc_readdir64 (DIR *dir);
#endif
#if defined(__USE_FILE_OFFSET64)
static inline struct dirent *readdir(DIR *dir)
{
    return (struct dirent *)posixc_readdir64(dir);
}
#else
static inline struct dirent *readdir(DIR *dir)
{
    return posixc_readdir(dir);
}
#endif

/* NOTIMPL int posixc_readdir_r (DIR *dir, struct dirent *entry, struct dirent **result); */
# ifdef __USE_LARGEFILE64
/* NOTIMPL 
int posixc_readdir64_r (DIR *dir,
                        struct dirent64 *entry,
                        struct dirent64 **result) */
# endif
#else /* NO_POSIX_WRAPPERS */
struct dirent *readdir(DIR *dir);
#if defined(__USE_LARGEFILE64)
struct dirent64 *readdir64 (DIR *dir);
#endif
#endif /* NO_POSIX_WRAPPERS */

#ifdef __USE_XOPEN2K8
int dirfd(DIR *dir);
#if !defined(NO_POSIX_WRAPPERS)
int posixc_scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **,
                            const struct dirent **));
#if defined(__USE_LARGEFILE64)
int posixc_scandir64 (const char *dir,
                      struct dirent64 ***namelist,
                      int (*select) (const struct dirent64 *),
                      int (*compar) (const struct dirent64 **,
                                    const struct dirent64 **));
#endif
#if defined(__USE_FILE_OFFSET64)
static inline int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **,
                            const struct dirent **))
{
    int (*select64)(const struct dirent64 *) = (int (*)(const struct dirent64 *))select;
    int (*compar64)(const struct dirent64 **, const struct dirent64 **) = (int (*)(const struct dirent64 **, const struct dirent64 **))compar;
    return posixc_scandir64(dir, (struct dirent64 ***)namelist, select64, compar64);
}
#else
static inline int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **,
                            const struct dirent **))
{
    return posixc_scandir(dir, namelist, select, compar);
}
#endif
int posixc_alphasort(const struct dirent **a, const struct dirent **b);
#if defined(__USE_LARGEFILE64)
int posixc_alphasort64 (const struct dirent64 **a, const struct dirent64 **b);
#endif
#if defined(__USE_FILE_OFFSET64)
static inline int alphasort(const struct dirent **a, const struct dirent **b)
{
    return posixc_alphasort64((const struct dirent64 **)a, (const struct dirent64 **)b);
}
#else
static inline int alphasort(const struct dirent **a, const struct dirent **b)
{
    return posixc_alphasort(a, b);
}
#endif
#else /* NO_POSIX_WRAPPERS */
int scandir (const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **,
                            const struct dirent **));
#if defined(__USE_LARGEFILE64)
int scandir64 (const char *dir,
                      struct dirent64 ***namelist,
                      int (*select) (const struct dirent64 *),
                      int (*compar) (const struct dirent64 **,
                                    const struct dirent64 **));
#endif
int alphasort(const struct dirent **a, const struct dirent **b);
#if defined(__USE_LARGEFILE64)
int alphasort64 (const struct dirent64 **a, const struct dirent64 **b)
#endif
#endif /* NO_POSIX_WRAPPERS */
#endif /* !__USE_XOPEN2K8 */

void rewinddir(DIR *dir);

#if defined(__USE_NIXCOMMON) || defined(__USE_XOPEN)
void seekdir(DIR *dir, off_t loc);
long telldir(DIR *dir);
#endif

__END_DECLS

#endif /* !_POSIXC_DIRENT_H_ */
