/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ICON_INTERN_H
#define ICON_INTERN_H

/* Include files */
#ifndef CLIB_ALIB_PROTOS_H
#   include <proto/alib.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif
#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef PROTO_ICON_H
#   include <proto/icon.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif
#ifndef PROTO_IFFPARSE_H
#   include <proto/iffparse.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif
#ifndef CYBERGRAPHX_CYBERGRAPHICS_H
#   include <cybergraphx/cybergraphics.h>
#endif
#ifndef PROTO_CYBERGRAPHICS_H
#   include <proto/cybergraphics.h>
#endif
#ifndef WORKBENCH_WORKBENCH_H
#   include <workbench/workbench.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef _STDDEF_H_
#   include <stddef.h>
#endif

#ifndef LIBCORE_BASE_H
#ifndef __MORPHOS__
#   include <libcore/base.h>
#endif
#endif
#include <string.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

/****************************************************************************************/

/* Constants */
#define MAX_DEFICON_FILEPATH	256

/* Number of entries in the mementrys in the freelists */
#define FREELIST_MEMLISTENTRIES 10

#define ICONDUPA_JustLoadedFromDisk ICONA_Reserved1

#define ICONLIST_HASHSIZE 256

/****************************************************************************************/

/* To get right alignment we make our very own memlist structur
Look at the original struct MemList in exec/memory.h to see why */

struct IconInternalMemList
{
    struct Node     	iiml_Node;
    UWORD   	    	iiml_NumEntries;
    struct MemEntry 	iiml_ME[FREELIST_MEMLISTENTRIES];
};

struct Image35
{
    UBYTE *imagedata;
    UBYTE *palette;
    UBYTE *mask;
    WORD  numcolors;
    WORD  depth;
    WORD  flags;
    UBYTE transparentcolor;
    
};

#define IMAGE35F_HASTRANSPARENTCOLOR 1
#define IMAGE35F_HASPALETTE 	     2

struct Icon35
{
    struct Image35 img1;
    struct Image35 img2;
    WORD    	   width;
    WORD    	   height;
    WORD    	   flags;
    WORD    	   aspect;
};

#define ICON35F_FRAMELESS   	    1

struct NativeIcon
{
    struct MinNode    node;
    APTR    	      pool;
    struct DiskObject dobj;
    struct Icon35     icon35;
    APTR    	      iconbase;
    struct BitMap    *iconbm1;
    struct BitMap    *iconbm2;
    struct Screen    *iconscr;
    struct ViewPort  *iconvp;
    struct ColorMap  *iconcm;
    WORD    	      iconbmwidth;
    WORD    	      iconbmheight;
    WORD    	      iconbmdepth;    
};

#define NATIVEICON(icon) ((struct NativeIcon *)((UBYTE *)(icon) - offsetof(struct NativeIcon, dobj)))

struct IconBase
{
    struct Library   	     LibNode;
    BPTR	     	     ib_SegList;
    struct ExecBase  	    *ib_SysBase;

    struct Library  	    *utilitybase;
    struct Hook       	     dsh;
    struct IntuitionBase    *intuitionbase;
    struct Library  	    *iffparsebase;
    struct GfxBase  	    *gfxbase;
    struct Library  	    *cybergfxbase;
    
    struct SignalSemaphore   iconlistlock;
    struct MinList  	     iconlists[ICONLIST_HASHSIZE];
};

typedef struct IconBase IconBase_T;

/****************************************************************************************/

extern struct ExecBase * SysBase;
extern struct DosLibrary * DOSBase;

/****************************************************************************************/

/* Internal prototypes */
AROS_UFP3(LONG, dosstreamhook,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(BPTR,            file, A2),
    AROS_UFPA(ULONG *,         msg, A1)
);
VOID	GetDefIconName (LONG, UBYTE *);
UBYTE * WriteValue     (LONG, UBYTE *);
LONG CalcIconHash(struct DiskObject *dobj);
VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase);
VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase);
struct NativeIcon *GetNativeIcon(struct DiskObject *dobj, struct IconBase *IconBase);
BOOL ReadIcon35(struct NativeIcon *icon, struct Hook *streamhook, void *stream, struct IconBase *IconBase);
VOID FreeIcon35(struct NativeIcon *icon, struct IconBase *IconBase);

/****************************************************************************************/

typedef struct IntuitionBase IntuitionBase_T;
typedef struct GfxBase GfxBase_T;

#define LB(icon)        ((IconBase_T *)icon)
#undef UtilityBase
#define UtilityBase	(((IconBase_T *)IconBase)->utilitybase)

#undef IntuitionBase
#define IntuitionBase	(((IconBase_T *)IconBase)->intuitionbase)

#undef IFFParseBase
#define IFFParseBase	(((IconBase_T *)IconBase)->iffparsebase)

#undef CyberGfxBase
#define CyberGfxBase	(((IconBase_T *)IconBase)->cybergfxbase)

#undef GfxBase
#define GfxBase	    	(((IconBase_T *)IconBase)->gfxbase)

/****************************************************************************************/

#endif /* ICON_INTERN_H */
