#ifndef DOS_STDIO_H
#define DOS_STDIO_H

/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI like definitions for DOS stdio functions.
    Lang: english
*/

#define ReadChar()              FGetC(Input())
#define WriteChar(c)            FPutC(Output(),(c))
#define UnReadChar(c)           UnGetC(Input(),(c))
#define ReadChars(buf,num)      FRead(Input(), (buf), 1, (num))
#define ReadLn(buf,len)         FGets(Input(), (buf), (len))
#define WriteStr(s)             FPuts(Output(), (s))
#define VWritef(format,argv)    VFWritef(Output(), (format), (argv))

/* DOS functions will return this when they reach EOF */
#define ENDSTREAMCH             -1

/* buffering types for SetVBuf() */
#define BUF_LINE    0       /* Flush at the end of lines '\n' */
#define BUF_FULL    1       /* Flush when buffer is full */
#define BUF_NONE    2       /* Do not buffer, immediate read/write */

#endif /* DOS_STDIO_H */
