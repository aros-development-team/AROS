#ifndef  CLIB_AROS_PROTOS_H
#define  CLIB_AROS_PROTOS_H

/*
**	$VER: aros_protos.h 1.0 (26.10.95)
**
**	C prototypes. For use with 32 bit integers only.
**
*/

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifdef DEBUG_FreeMem
#   ifndef CLIB_EXEC_PROTOS_H
#	include <clib/exec_protos.h>
#   endif
#   if DEBUG_FreeMem
#	undef FreeMem
#	define FreeMem NastyFreeMem
#   endif
#endif

/*
    Prototypes
*/
ULONG CalcChecksum (APTR mem, ULONG size);
int   STRCMP	   (const UBYTE *, const UBYTE *);
int   kprintf	   (const UBYTE *, ...);
void  NastyFreeMem (void *, ULONG);

#endif /* CLIB_AROS_PROTOS_H */
