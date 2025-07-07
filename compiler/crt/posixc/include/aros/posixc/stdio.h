#ifndef _POSIXC_STDIO_H_
#define _POSIXC_STDIO_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file stdio.h
*/

#include <aros/system.h>

#include <aros/features.h>
#include <aros/stdc/stdio.h>

#include <aros/types/off_t.h>
#include <aros/types/ssize_t.h>

#define L_ctermid	FILENAME_MAX
#define P_tmpdir	"T:"

__BEGIN_DECLS

/* Core POSIX */
POSIXCFUNC(FILE *, fopen, (const char * restrict filename, const char * restrict mode));
POSIXCFUNC(int, fclose, (FILE *stream));
POSIXCFUNC(int, fflush, (FILE *stream));
POSIXCFUNC(int, fgetpos, (FILE * restrict stream, fpos_t * restrict pos));
POSIXCFUNC(int, fsetpos, (FILE *stream, const fpos_t *pos));
POSIXCFUNC(int, feof, (FILE *stream));
POSIXCFUNC(int, ferror, (FILE *stream));
POSIXCFUNC(int, ungetc, (int c, FILE *stream));
POSIXCFUNC(FILE *, tmpfile, (void));
POSIXCFUNC(char *, tmpnam, (char *s));
POSIXCFUNC(int, setvbuf, (FILE * restrict stream, char * restrict buf, int mode, size_t size));
POSIXCFUNC(void, setbuf, (FILE * restrict stream, char * restrict buf));
POSIXCFUNC(int, scanf, (const char * restrict format, ...));
POSIXCFUNC(void, rewind, (FILE *stream));
POSIXCFUNC(int, puts, (const char *s));
POSIXCFUNC(int, putchar, (int c));
POSIXCFUNC(int, printf, (const char * restrict format, ...));
POSIXCFUNC(char *, gets, (char *s));  /* Legacy */
POSIXCFUNC(int, getchar, (void));
POSIXCFUNC(size_t, fwrite, (const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream));
POSIXCFUNC(long int, ftell, (FILE *stream));
POSIXCFUNC(int, fseek, (FILE *stream, long int offset, int whence));
POSIXCFUNC(int, fscanf, (FILE * restrict stream, const char * restrict format, ...));
POSIXCFUNC(FILE *, freopen, (const char * restrict filename, const char * restrict mode, FILE * restrict stream));
POSIXCFUNC(size_t, fread, (void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream));
POSIXCFUNC(int, fputs, (const char * restrict s, FILE * restrict stream));
POSIXCFUNC(int, fputc, (int c, FILE *stream));
POSIXCFUNC(int, fprintf, (FILE * restrict stream, const char * restrict format, ...));
POSIXCFUNC(char *, fgets, (char * restrict s, int n, FILE * restrict stream));
POSIXCFUNC(int, fgetc, (FILE *stream));
POSIXCFUNC(int, vscanf, (const char * restrict format, va_list arg));
POSIXCFUNC(int, vprintf, (const char * restrict format, va_list arg));
POSIXCFUNC(int, vfscanf, (FILE * restrict stream, const char * restrict format, va_list arg));
POSIXCFUNC(int, vfprintf, (FILE * restrict stream, const char * restrict format, va_list arg));

int fileno(FILE *);
void flockfile(FILE *);
int getc_unlocked(FILE *);
void funlockfile(FILE *);

/* Extensions requiring feature guards */

/* GNU or XSI */
#if defined(_GNU_SOURCE) || defined(_XOPEN_SOURCE)
FILE *fdopen(int filedes, const char *mode);
int pclose(FILE *);
FILE *popen(const char *, const char *);
char *tempnam(const char *dir, const char *pfx);
#endif

/* GNU extensions */
#if defined(_GNU_SOURCE)
/* NOTIMPL char *ctermid(char *); */
/* NOTIMPL int dprintf(int, const char *restrict, ...); */
/* NOTIMPL FILE *fmemopen(void *restrict, size_t, const char *restrict); */
/* NOTIMPL int ftrylockfile(FILE *); */
/* NOTIMPL int getchar_unlocked(void); */
/* NOTIMPL ssize_t getdelim(char **restrict, size_t *restrict, int, FILE *restrict); */
/* NOTIMPL ssize_t getline(char **restrict, size_t *restrict, FILE *restrict); */
/* NOTIMPL FILE *open_memstream(char **, size_t *); */
/* NOTIMPL int putc_unlocked(int, FILE *); */
/* NOTIMPL int putchar_unlocked(int); */
/* NOTIMPL int renameat(int, const char *, int, const char *); */
/* NOTIMPL int vdprintf(int, const char *restrict, va_list); */
#endif

/* Large file support (non-standard) */
FILE *fopen64(const char * restrict filename, const char * restrict mode);
int fgetpos64(FILE * restrict stream, __fpos64_t * restrict pos);
int fsetpos64(FILE *stream, const __fpos64_t *pos);
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
#if defined(__off64_t_defined)
int fseeko64(FILE *stream, off64_t offset, int whence);
off64_t ftello64(FILE *stream);
#endif

/* clib2 compatibility */
int __get_default_file(int file_descriptor, long *file_handle);

/* Deprecated POSIX functions */
__header_inline int getw(FILE *stream)
{
    int word;
    return (fread(&word, sizeof(word), 1, stream) > 0) ? word : EOF;
}

__header_inline int putw(int word, FILE *stream)
{
    return (fwrite(&word, sizeof(word), 1, stream) > 0) ? 0 : EOF;
}

__END_DECLS

#endif /* _POSIXC_STDIO_H_ */
