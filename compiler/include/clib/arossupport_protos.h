#ifndef  CLIB_AROSSUPPORT_PROTOS_H
#define  CLIB_AROSSUPPORT_PROTOS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prototypes for aros.lib
    Lang: english
*/

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_AROSSUPPORTBASE_H
#   include <aros/arossupportbase.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef  DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef  UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

#ifdef DEBUG_FreeMem
#   if DEBUG_FreeMem
#       undef FreeMem
#       define FreeMem NastyFreeMem
#   endif
#endif

/* for va_list in kprintf() proto */
#include <stdarg.h>

/*
    Prototypes
*/
#ifdef __cplusplus
extern "C" {
#endif
ULONG   CalcChecksum (APTR mem, ULONG size);
int     kprintf      (const char * fmt, ...);
int     vkprintf     (const char * fmt, va_list ap);
int     rkprintf     (const STRPTR, const STRPTR, int, const UBYTE * fmt, ...);
void    NastyFreeMem (APTR mem, ULONG size);
APTR    RemoveSList  (APTR * list, APTR node);
void    hexdump      (const void * data, IPTR offset, ULONG count);
int     strrncasecmp (const char *, const char *, int);
void    RawPutChars  (const UBYTE * string, int len);
BOOL    IsDosEntryA  (char *Name, ULONG Flags);

/* dos.library-compatible data generation */
BSTR CreateBSTR(CONST_STRPTR src);
BPTR CreateSegList(APTR function);

/*
 * Taglist parsing functions.
 * These functions are intended for use by early startup code only
 * (where utility.library is not available yet).
 * Please consider using utility.library whenever possible.
 */
struct TagItem *LibNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *LibFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR LibGetTagData(Tag tagValue, IPTR defaultVal, const struct TagItem *tagList);

/* AROS enhancements */
BOOL ReadByte    (struct Hook *, UBYTE  * dataptr, void * stream);
BOOL ReadWord    (struct Hook *, UWORD  * dataptr, void * stream);
BOOL ReadLong    (struct Hook *, ULONG  * dataptr, void * stream);
BOOL ReadFloat   (struct Hook *, FLOAT  * dataptr, void * stream);
BOOL ReadDouble  (struct Hook *, DOUBLE * dataptr, void * stream);
BOOL ReadString  (struct Hook *, STRPTR * dataptr, void * stream);
BOOL ReadStruct  (struct Hook *, APTR   * dataptr, void * stream, const IPTR * desc);
BOOL WriteByte   (struct Hook *, UBYTE  data, void * stream);
BOOL WriteWord   (struct Hook *, UWORD  data, void * stream);
BOOL WriteLong   (struct Hook *, ULONG  data, void * stream);
BOOL WriteFloat  (struct Hook *, FLOAT  data, void * stream);
BOOL WriteDouble (struct Hook *, DOUBLE data, void * stream);
BOOL WriteString (struct Hook *, STRPTR data, void * stream);
BOOL WriteStruct (struct Hook *, APTR   data, void * stream, const IPTR * desc);
void FreeStruct  (APTR s,  const IPTR * desc);

/* RastPort manipulations */
struct RastPort *CreateRastPort(void);
struct RastPort *CloneRastPort(struct RastPort *rp);
void DeinitRastPort(struct RastPort *rp);
void FreeRastPort(struct RastPort *rp);

#ifdef __cplusplus
}
#endif

/* don't use SysBase->kprintf on AmigaOS, or AROS m68k */
#if defined(__AROS__) && !defined(__mc68000)
#   define kprintf     (((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf)
#   define rkprintf    (((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf)
#   define vkprintf    (((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf)
#endif

#endif /* CLIB_AROSSUPPORT_PROTOS_H */
