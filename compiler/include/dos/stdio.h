#ifndef DOS_STDIO_H
#define DOS_STDIO_H

/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Some stubs for ANSI-like stdio functions.
    Lang: english
*/


#define ReadChar()           FGetC(Input())
  /* Read one character from stdin. */
#define WriteChar(c)         FPutC(Output(),(c))
  /* Write one character to stdout. */
#define UnReadChar(c)        UnGetC(Input(),(c))
  /* Put one character back to stdin. Normally this is only guaranteed to
     work once. */
#define ReadChars(buf,num)   FRead(Input(), (buf), 1, (num))
  /* Read a number of chars from stdin. */
#define ReadLn(buf,len)      FGets(Input(), (buf), (len))
  /* Read a whole line from stdin. */
#define WriteStr(s)          FPuts(Output(), (s))
  /* Write a string to stdout. */
#define VWritef(format,argv) VFWritef(Output(), (format), (argv))
  /* Write a formatted string to stdout. */

/* DOS functions will return this when they reach EOF. */
#define ENDSTREAMCH -1

/* Buffering types for SetVBuf(). */
#define BUF_LINE 0 /* Flush at the end of lines '\n'. */
#define BUF_FULL 1 /* Flush only when buffer is full. */
#define BUF_NONE 2 /* Do not buffer, read and write immediatly. */

#endif /* DOS_STDIO_H */
