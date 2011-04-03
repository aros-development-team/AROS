
/*
** sprintf.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "sprintf.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include <stdarg.h>
#include <exec/types.h>
#include <SDI/SDI_misc.h>

#include "version.h"

/*************************************************************************/

/*
** This structure keeps our sprintf vars during RawDoFmt()
*/

struct SPrintfStream
{
    char    *Target;
    ULONG    TargetSize;
};

#define MAX_SPRINTFNV_ARGS 32  /* number of arguments kept on stack */

/*************************************************************************/

/* /// SPrintf_DoChar()
**
*/

/*************************************************************************/

PUTCHARPROTO( SPrintf_DoChar, char c, struct SPrintfStream *s )
{
	if( (s->Target) ) {  /* this is somehow needed, or MOS version will break appart */
		*(s->Target++) = c;
	}
}
/* \\\ */

/* /// SPrintf_DoStream()
**
*/

/*************************************************************************/

PUTCHARPROTO( SPrintf_DoStream, char c, struct SPrintfStream *s )
{
    if( s->TargetSize ) {
        if( (--s->TargetSize) ) {
            *(s->Target++) = c;
        } else {
            *(s->Target++) = 0;
        }
    }
}
/* \\\ */

/* /// SPrintf()
**
*/

/*************************************************************************/

ULONG SPrintf(char *format, char *target, ULONG *args)
{
struct SPrintfStream s;

    s.Target  = target;

    RawDoFmt( format, args, ENTRY( SPrintf_DoChar), &s);

return( s.Target - target);
}
/* \\\ */

/* /// SPrintfnv()
**
** SPrintfnv ( n is output limit, v means varargs)
*/

/*************************************************************************/

ULONG STDARGS SPrintfnv(char *format, char *target, ULONG targetsize, ULONG numargs, ...)
{
struct SPrintfStream s;
ULONG args[MAX_SPRINTFNV_ARGS];
ULONG i = 0;
va_list va;

#ifdef DEBUG
    if( numargs > MAX_SPRINTFNV_ARGS) {
		debug("\n\n%78m*\n%s: Warning! Too much arguments in SPrintfnv()\n%78m*\n", APPLICATIONNAME );
        numargs = MAX_SPRINTFNV_ARGS;
    }
#endif

    va_start(va, numargs);
    while( i<numargs ) { args[i] = va_arg( va, ULONG); i++; };
    va_end(va);

    s.Target     = target;
    s.TargetSize = targetsize;

    RawDoFmt(format,args, ENTRY( SPrintf_DoStream) ,&s);

return( s.Target - target );
}
/* \\\ */

/* /// SPrintfn()
**
*/

/*************************************************************************/

/*
** Printf with pre specified targetbuffer size
*/

ULONG STDARGS SPrintfn(char *format, char *target, ULONG targetsize, ULONG *args)
{
struct SPrintfStream s;

    s.Target     = target;
    s.TargetSize = targetsize;

    RawDoFmt(format, args, ENTRY( SPrintf_DoStream) ,&s);

return( s.Target - target );
}
/* \\\ */

