#ifndef _STDIO_H
#define _STDIO_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file stdio.h
    Lang: english
*/
#include <stdarg.h>

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
} FILE;

extern FILE * stdin, * stdout, * stderr;

extern int printf (const char* format, ...);
extern int vprintf (const char* format, va_list args);
extern int vfprintf (FILE * fh,const char* format, va_list args);
extern int fputc (int c, FILE * stream);
extern int putc (int c, FILE * stream);

#endif /* _STDIO_H */
