#ifndef TOOLLIB_MYSTREAM_H
#define TOOLLIB_MYSTREAM_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_CALLBACK_H
#   include <toollib/callback.h>
#endif

typedef struct
{
    CB	   get;
    CB	   unget;
    CB	   put;
    CB	   puts;
    char * name;
    int    line;
}
MyStream;

#define Str_Get(str,d)      CallCB(((MyStream *)str)->get, str, 0, d)
#define Str_Unget(str,c,d)  CallCB(((MyStream *)str)->unget, str, c, d)
#define Str_Put(str,c,d)    CallCB(((MyStream *)str)->put, str, c, d)

#define Str_GetName(str)    (((MyStream *)str)->name)
#define Str_GetLine(str)    (((MyStream *)str)->line)
#define Str_SetLine(str,l)  ((((MyStream *)str)->line) = (l))
#define Str_NextLine(str)   (((MyStream *)str)->line ++)

extern int Str_Init PARAMS ((MyStream * ms, const char * name));
extern void Str_Delete PARAMS ((MyStream * ms));
extern int Str_Puts PARAMS ((MyStream * ms, const char * str, CBD data));
extern void Str_PushError PARAMS ((MyStream * ms, const char * fmt, ...));
extern void Str_PushWarn PARAMS ((MyStream * ms, const char * fmt, ...));

#endif /* TOOLLIB_MYSTREAM_H */
