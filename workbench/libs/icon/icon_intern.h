#ifndef ICON_INTERN_H
#define ICON_INTERN_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <exec/libraries.h>
#include <aros/asmcall.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <cybergraphx/cybergraphics.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <graphics/view.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>

#include <stddef.h>

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

/* 
    To get right alignment we make our very own memlist structure.
    Look at the original struct MemList in <exec/memory.h> to see why. 
*/

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

struct IconPNG
{
    APTR   handle;
    APTR   handle2;
    UBYTE *filebuffer;
    ULONG  filebuffersize;
    UBYTE *img1;
    UBYTE *img2;
    WORD   width;
    WORD   height;    
};

struct NativeIcon
{
    struct MinNode    node;
    APTR    	      pool;
    struct DiskObject dobj;
    ULONG   	      readstruct_state;
    struct Icon35     icon35;
    struct IconPNG    iconPNG;
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

#define RSS_OLDDRAWERDATA_READ  (1 << 0)
#define RSS_GADGETIMAGE_READ	(1 << 1)
#define RSS_SELECTIMAGE_READ	(1 << 2)
#define RSS_DEFAULTTOOL_READ	(1 << 3)
#define RSS_TOOLWINDOW_READ	(1 << 4)
#define RSS_TOOLTYPES_READ	(1 << 5)

#define NATIVEICON(icon) ((struct NativeIcon *)((UBYTE *)(icon) - offsetof(struct NativeIcon, dobj)))

struct IconBase
{
    struct Library          ib_Lib;
    struct ExecBase        *ib_SysBase;
    APTR                    ib_SegList;

    struct Library  	   *pngbase;
    struct Hook             dsh;
    struct SignalSemaphore  iconlistlock;
    struct MinList          iconlists[ICONLIST_HASHSIZE];
    
    APTR                    ib_MemoryPool;
    ULONG   	    	    ib_CRCTable[256];
    BOOL    	    	    ib_CRCTableComputed;
    
    /* Global settings -----------------------------------------------------*/
    struct Screen          *ib_Screen;
    LONG                    ib_Precision;
    struct Rectangle        ib_EmbossRectangle;
    BOOL                    ib_Frameless;
    struct Hook            *ib_IdentifyHook;
    LONG                    ib_MaxNameLength;
    BOOL                    ib_NewIconsSupport;
    BOOL                    ib_ColorIconSupport;
};

typedef struct IconBase IconBase_T;

#define PNGBase IconBase->pngbase

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

#include "support.h"

UBYTE * WriteValue     (LONG, UBYTE *);

BOOL ReadIcon35(struct NativeIcon *icon, struct Hook *streamhook, void *stream, struct IconBase *IconBase);
BOOL WriteIcon35(struct NativeIcon *icon, struct Hook *streamhook, void *stream, struct IconBase *IconBase);
VOID FreeIcon35(struct NativeIcon *icon, struct IconBase *IconBase);

BOOL ReadIconPNG(struct DiskObject **ret, BPTR file, struct IconBase *IconBase);
BOOL WriteIconPNG(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase);
VOID FreeIconPNG(struct DiskObject *dobj, struct IconBase *IconBase);


#define LB(ib)          ((struct IconBase *) (ib))

#define POOL            (((struct IconBase *) IconBase)->ib_MemoryPool)

#endif /* ICON_INTERN_H */
