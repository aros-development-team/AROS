#ifndef _STDIO_H
#define _STDIO_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file stdio.h
    Lang: english
*/

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

extern int printf (const char* format, ...);

#endif /* _STDIO_H */
