#ifndef  CLIB_AROS_PROTOS_H
#define  CLIB_AROS_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for aros.lib
    Lang: english
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
#ifndef  DOS_DOS_H
#   include <dos/dos.h>
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

/* AROS enhancements */
BOOL ReadByte	 (BPTR fh, UBYTE  * dataptr);
BOOL ReadWord	 (BPTR fh, UWORD  * dataptr);
BOOL ReadLong	 (BPTR fh, ULONG  * dataptr);
BOOL ReadFloat	 (BPTR fh, FLOAT  * dataptr);
BOOL ReadDouble  (BPTR fh, DOUBLE * dataptr);
BOOL ReadString  (BPTR fh, STRPTR * dataptr);
BOOL ReadStruct  (BPTR fh, APTR   * dataptr, const IPTR * desc);
BOOL WriteByte	 (BPTR fh, UBYTE  data);
BOOL WriteWord	 (BPTR fh, UWORD  data);
BOOL WriteLong	 (BPTR fh, ULONG  data);
BOOL WriteFloat  (BPTR fh, FLOAT  data);
BOOL WriteDouble (BPTR fh, DOUBLE data);
BOOL WriteString (BPTR fh, STRPTR data);
BOOL WriteStruct (BPTR fh, APTR   data, const IPTR * desc);
void FreeStruct  (APTR s,  const IPTR * desc);


#define kprintf     (((struct AROSBase *)(SysBase->DebugData))->kprintf)

#endif /* CLIB_AROS_PROTOS_H */
