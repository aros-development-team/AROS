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

ULONG CalcChecksum (APTR mem, ULONG size);
int   STRCMP	   (const UBYTE *, const UBYTE *);

#endif /* CLIB_AROS_PROTOS_H */
