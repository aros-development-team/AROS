#ifndef _STDIO_H
#define _STDIO_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file stdio.h
    Lang: english
*/
#include <stdarg.h>
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL (void*)0
#endif /* __cplusplus */
#endif /* NULL */

#ifndef EOF
#define EOF (-1)
#endif

typedef struct __FILE
{
    void * fh;
    long   flags;
} FILE;

#define _STDIO_FILEFLAG_EOF	1L

extern FILE * stdin, * stdout, * stderr;

extern int printf (const char * format, ...);
extern int vprintf (const char * format, va_list args);
extern int fprintf (FILE * fh, const char * format, ...);
extern int vfprintf (FILE * fh, const char * format, va_list args);
extern int fputc (int c, FILE * stream);
extern int fputs (const char * str, FILE * stream);
extern int puts (const char * str, FILE * stream);
extern int fflush (FILE * stream);
extern int fgetc (FILE * stream);
extern int feof (FILE * stream);
extern int sprintf (char * str, const char * format, ...);
extern int vsprintf (char * str, const char * format, va_list args);
extern int snprintf (char * str, size_t n, const char * format, ...);
extern int vsnprintf (char * str, size_t n, const char * format, va_list args);

#ifdef AROS_ALMOST_COMPATIBLE
extern int __vcformat (void * data, int (*uc)(int, void *),
			const char * format, va_list args);
#endif

#define putc fputc
#define getc fgetc

#endif /* _STDIO_H */
