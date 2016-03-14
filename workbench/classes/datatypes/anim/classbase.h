#ifndef CLASSBASE_H
#define CLASSBASE_H 1

/*
**
** $Id$
**  anim.datatype 1.12
**
**  Header file for DataTypes class
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/* amiga includes */
#include <exec/types.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/dostags.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <graphics/scale.h>
#include <intuition/classes.h>  /* must have Id tag "classes.h,v 40.0 94/02/15 17:46:35 davidj Exp Locker: davidj" */
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/animationclass.h>
#include <datatypes/animationclassext.h> /* animation.datatype V41 extensions */

/* amiga prototypes */
#ifdef __SASC
#include <clib/macros.h>
#include <clib/exec_protos.h>
#include <clib/utility_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/datatypes_protos.h>
#include <clib/dtclass_protos.h>
#include <clib/iffparse_protos.h>
#ifdef PARAMETERS_STACK
#include <clib/alib_protos.h>
#include <clib/alib_stdio_protos.h>
#endif /* PARAMETERS_STACK */

/* amiga pragmas */
#include <pragmas/exec_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/datatypes_pragmas.h>
#include <pragmas/dtclass_pragmas.h>
#include <pragmas/iffparse_pragmas.h>
#include <pragmas/alib_pragmas.h> /* amiga.lib stubs (tagcall pragmas) */
#else
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/datatypes.h>
#include <proto/dtclass.h>
#include <proto/iffparse.h>

#include <clib/alib_protos.h>
#endif

/* ANSI includes */
#include <string.h>
#include <limits.h>

/* Use asyncronous I/O below. Disabled due problems with Seek'ing */
#if 0
#define DOASYNCIO 1
#endif

/*****************************************************************************/

typedef LONG (*unpack_ilbm_t)(struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *dlta, ULONG dltasize);
typedef LONG (*unpack_xor_t)(struct AnimHeader *anhd, struct BitMap *bm, struct BitMap *deltabm);
typedef LONG (*unpack_deltabm_t)(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm);
typedef LONG (*unpack_delta_t)(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize);
typedef LONG (*unpack_delta4_t)(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags);

/*****************************************************************************/

/* Shared library base for a BOOPSI class */
struct ClassBase
{
    struct ClassLibrary         cb_Lib;
#if !defined(__AROS__)
    struct ExecBase             *cb_SysBase;
    struct Library              *cb_UtilityBase;
    struct Library              *cb_DOSBase;
    struct Library              *cb_IFFParseBase;
    struct Library              *cb_GfxBase;
    struct Library              *cb_IntuitionBase;
    struct Library              *cb_DataTypesBase;
    struct Library              *cb_SuperClassBase;
    BPTR                        cb_SegList;
#endif
    struct SignalSemaphore      cb_Lock;                /* access lock                  */

    unpack_ilbm_t               unpackilbmbody;         /* unpack function hooks ..     */
    unpack_xor_t                xorbm;
    unpack_deltabm_t            unpackanimidelta;
    unpack_deltabm_t            unpackanimjdelta;
    unpack_delta_t              unpacklongdelta;
    unpack_delta_t              unpackshortdelta;
    unpack_delta_t              unpackbytedelta;
    unpack_delta4_t             unpackanim4longdelta;
    unpack_delta4_t             unpackanim4worddelta;
    unpack_delta_t              unpackanim7longdelta;
    unpack_delta_t              unpackanim7worddelta;
    unpack_delta_t              unpackanim8longdelta;
    unpack_delta_t              unpackanim8worddelta;
};

/*****************************************************************************/

#ifdef __SASC
/* SASC specific defines */
#define DISPATCHERFLAGS __saveds __asm
#define ASM __asm
#define REGD0 register __d0
/* ... */
#define REGA0 register __a0
#define REGA1 register __a1
#define REGA2 register __a2
/* ... */
#define REGA6 register __a6
#else
#ifdef __GNUC__
#define DISPATCHERFLAGS
#define ASM
#define REGD0
#define REGA0
#define REGA1
#define REGA2
#define REGA6
#define __stdargs
#else
#error unsupported compiler
#endif
#endif /* __SASC */

/*****************************************************************************/

#if !defined(__AROS__)
#define SysBase        (cb -> cb_SysBase)
#define UtilityBase    (cb -> cb_UtilityBase)
#define DOSBase        (cb -> cb_DOSBase)
#define IFFParseBase   (cb -> cb_IFFParseBase)
#define GfxBase        (cb -> cb_GfxBase)
#define IntuitionBase  (cb -> cb_IntuitionBase)
#define DataTypesBase  (cb -> cb_DataTypesBase)
#endif

/*****************************************************************************/

#if DEBUG > 0
#define DFORMATS(...)        bug(__VA_ARGS__);
#else
#define DFORMATS(...)
#endif

/*****************************************************************************/

#if defined(__AROS__)
#define ABS(x) x
#define	MIN(a,b) (((a) < (b)) ?	(a) : (b))
#define	MAX(a,b) (((a) > (b)) ?	(a) : (b))
#endif

/* Align memory on 4 byte boundary */
#define ALIGN_LONG( mem ) ((APTR)((((IPTR)(mem)) + 3UL) & ~3UL))

/* Align memory on 16 byte boundary */
#define ALIGN_QUADLONG( mem ) ((APTR)((((IPTR)(mem)) + 15UL) & ~15UL))

/* Following ptr */
#define MEMORY_FOLLOWING( ptr )     ((void *)((ptr) + 1))

/* Memory after n bytes */
#define MEMORY_N_FOLLOWING( ptr, n ) ((void *)(((UBYTE *)ptr) + n ))

/* Memory after n bytes, longword aligned (Don't forget the 4 bytes in size for rounding !!) */
#define MEMORY_NAL_FOLLOWING( ptr, n ) (ALIGN_LONG( ((void *)(((UBYTE *)ptr) + n )) ))

/* casts */
#define V( x )    ((VOID *)(x))
#define G( o )    ((struct Gadget *)(o))
#define EXTG( o ) ((struct ExtGadget *)(o))

/* Exclude tag item */
#define XTAG( expr, tagid ) ((Tag)((expr)?(tagid):(TAG_IGNORE)))

/* Get data from pointer only if it is NOT NULL (and cast data to IPTR) */
#define XPTRDATA( x ) ((IPTR)((x)?(*(x)):(0UL)))

/* Boolean conversion */
#define MAKEBOOL( x ) ((BOOL)((x) != 0))

/*****************************************************************************/

#ifdef __SASC
#ifndef PARAMETERS_STACK
#define PARAMETERS_STACK 1
#define  CLIB_ALIB_PROTOS_H
__stdargs ULONG FastRand( unsigned long seed );
__stdargs void  NewList( struct List *list );
__stdargs ULONG DoMethodA( Object *obj, Msg message );
__stdargs ULONG DoMethod( Object *obj, unsigned long MethodID, ... );
__stdargs ULONG DoSuperMethodA( struct IClass *cl, Object *obj, Msg message );
__stdargs ULONG DoSuperMethod( struct IClass *cl, Object *obj, unsigned long MethodID, ... );
__stdargs ULONG CoerceMethodA( struct IClass *cl, Object *obj, Msg message );
__stdargs ULONG CoerceMethod( struct IClass *cl, Object *obj, unsigned long MethodID, ... );
__stdargs ULONG SetSuperAttrs( struct IClass *cl, Object *obj, unsigned long Tag1, ... );
#endif /* !PARAMETERS_STACK */

__stdargs void kprintf( STRPTR, ... );
#endif

/*****************************************************************************/

#include "class_iprotos.h"

#endif /* !CLASSBASE_H */
