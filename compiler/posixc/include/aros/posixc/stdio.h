#ifndef _POSIXC_STDIO_H_
#define _POSIXC_STDIO_H_

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file stdio.h
*/

#include <aros/features.h>

/* C99 */
#include <aros/stdc/stdio.h>

#include <aros/types/off_t.h>
#include <aros/types/ssize_t.h>

#define L_ctermid	FILENAME_MAX	/* Max filename for controlling tty */

#define P_tmpdir	"T:"		/* Default temporary path */

__BEGIN_DECLS

#if !defined(NO_POSIX_WRAPPERS)
FILE *__posixc_fopen(const char * restrict filename, const char * restrict mode);
FILE *fopen64(const char * restrict filename, const char * restrict mode);
#if defined(__USE_FILE_OFFSET64)
static inline FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    return fopen64(filename, mode);
}
#else
static inline FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    return __posixc_fopen(filename, mode);
}
#endif
#else  /* NO_POSIX_WRAPPERS */
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *fopen64(const char * restrict filename, const char * restrict mode);
#endif /* NO_POSIX_WRAPPERS */

/* NOTIMPL char	*ctermid(char *); */
/* NOTIMPL int dprintf(int, const char *restrict, ...) */
FILE *fdopen (int filedes, const char *mode);
int fileno(FILE *);
void flockfile(FILE *);
/* NOTIMPL FILE *fmemopen(void *restrict, size_t, const char *restrict) */
#if !defined(NO_POSIX_WRAPPERS)
int __posixc_fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int __posixc_fsetpos(FILE *stream, const fpos_t *pos);
int fgetpos64(FILE * restrict stream, __fpos64_t * restrict pos);
int fsetpos64(FILE *stream, const __fpos64_t *pos);
#if defined(__USE_FILE_OFFSET64)
static inline int fgetpos(FILE * restrict stream, fpos_t * restrict pos)
{
    return fgetpos64(stream, (__fpos64_t *)pos);
}
static inline int fsetpos(FILE *stream, const fpos_t *pos)
{
    return fsetpos64(stream, (__fpos64_t *)pos);
}
#else
static inline int fgetpos(FILE * restrict stream, fpos_t * restrict pos)
{
    return __posixc_fgetpos(stream, pos);
}
static inline int fsetpos(FILE *stream, const fpos_t *pos)
{
    return __posixc_fsetpos(stream, pos);
}
#endif
int __posixc_fseeko(FILE *stream, off_t offset, int whence);
#if defined(__off64_t_defined)
int fseeko64(FILE *stream, off64_t offset, int whence);
#endif
#if defined(__USE_FILE_OFFSET64)
static inline int fseeko(FILE *stream, off_t offset, int whence)
{
    return fseeko64(stream, (__off64_t)offset, whence);
}
#else
static inline int fseeko(FILE *stream, off_t offset, int whence)
{
    return __posixc_fseeko(stream, offset, whence);
}
#endif
off_t __posixc_ftello(FILE *stream);
#if defined(__off64_t_defined)
off64_t ftello64(FILE *stream);
#endif
#if defined(__USE_FILE_OFFSET64)
static inline off_t ftello(FILE *stream)
{
    return (off_t)ftello64(stream);
}
#else
static inline off_t ftello(FILE *stream)
{
    return __posixc_ftello(stream);
}
#endif
#else  /* NO_POSIX_WRAPPERS */
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fsetpos(FILE *stream, const fpos_t *pos);
int fgetpos64(FILE * restrict stream, __fpos64_t * restrict pos);
int fsetpos64(FILE *stream, const __fpos64_t *pos);
int fseeko(FILE *stream, off_t offset, int whence);
#if defined(__off64_t_defined)
int fseeko64(FILE *stream, off64_t offset, int whence);
#endif
off_t ftello(FILE *stream);
#if defined(__off64_t_defined)
off64_t ftello64(FILE *stream);
#endif
#endif /* NO_POSIX_WRAPPERS */
/* NOTIMPL int ftrylockfile(FILE *); */
void funlockfile(FILE *);
int getc_unlocked(FILE *);
/* NOTIMPL int getchar_unlocked(void); */
/* NOTIMPL ssize_t getdelim(char **restrict, size_t *restrict, int, FILE *restrict); */
/* NOTIMPL ssize_t getline(char **restrict, size_t *restrict, FILE *restrict); */
/* NOTIMPL FILE *open_memstream(char **, size_t *); */
int pclose(FILE *);
FILE *popen(const char *, const char *);
/* NOTIMPL int      putc_unlocked(int, FILE *); */
/* NOTIMPL int      putchar_unlocked(int); */
/* NOTIMPL int      renameat(int, const char *, int, const char *); */
char *tempnam(const char *dir, const char *pfx);
/* NOTIMPL int      vdprintf(int, const char *restrict, va_list); */

/* Following function is here for clib2 compatibility and abc-shell
   Don't use in new code
*/
int __get_default_file (int file_descriptor, long * file_handle);

/* Implement deprecated POSIX.1-2001 functions as static inline functions. */
static __inline__ int getw(FILE *stream)
{
    int word;
    
    if (fread(&word, sizeof(word), 1, stream) > 0) return word;
    else                                           return EOF;
}

static __inline__ int putw(int word, FILE *stream)
{
    if (fwrite(&word, sizeof(word), 1, stream) > 0) return 0;
    else                                            return EOF;
}

__END_DECLS

#endif /* _POSIXC_STDIO_H_ */
