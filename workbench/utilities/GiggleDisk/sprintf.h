#ifndef SPRINTF_H
#define SPRINTF_H 1

/*************************************************************************/

#include <stdarg.h>
#include <exec/types.h>

/*************************************************************************/

ULONG         SPrintf  ( char *format, char *target, IPTR *args );
ULONG         SPrintfnv( char *format, char *target, ULONG targetsize, ULONG numargs, ... );
ULONG         SPrintfn ( char *format, char *target, ULONG targetsize, IPTR *args );

/*************************************************************************/

#endif /* SPRINTF_H */


