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
#ifndef AROS_AROSBASE_H
#   include <aros/arosbase.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif

#ifdef DEBUG_FreeMem
#   ifndef PROTO_EXEC_H
#	include <proto/exec.h>
#   endif
#   if DEBUG_FreeMem
#	undef FreeMem
#	define FreeMem NastyFreeMem
#   endif
#endif

extern struct ExecBase * Sysbase;

/*
    Prototypes
*/
ULONG	CalcChecksum (APTR mem, ULONG size);
int	kprintf      (const UBYTE * fmt, ...);
void	NastyFreeMem (APTR mem, ULONG size);
APTR	RemoveSList  (APTR * list, APTR node);
void	hexdump      (const void * data, IPTR offset, ULONG count);

#define kprintf     (((struct AROSBase *)(SysBase->DebugData))->kprintf)

#endif /* CLIB_AROS_PROTOS_H */
