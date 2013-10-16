#ifndef ICON_INTERN_H
#define ICON_INTERN_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

/* This must be a power of 2 */
#ifdef __mc68000
#define ICONLIST_HASHSIZE 32	/* Save space on small memory m68k machines */
#else
#define ICONLIST_HASHSIZE 256
#endif

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

#define IMAGE35F_HASTRANSPARENTCOLOR 1
#define IMAGE35F_HASPALETTE 	     2

#define ICON35F_FRAMELESS   	    1

struct NativeIcon
{
    struct MinNode    ni_Node;
    struct DiskObject ni_DiskObject;
    struct FreeList   ni_FreeList;
    ULONG   	      ni_ReadStruct_State;

    /* Source icon data */
    struct {
        APTR          Data;       /* Raw IFF or PNG stream */
        ULONG         Size;
        struct {
            LONG        Offset;
            LONG        Size;
        } PNG[2];
    } ni_Extra;

    /* Parameters */
    BOOL              ni_IsDefault;
    BOOL              ni_Frameless;
    ULONG             ni_ScaleBox;

    /* The 'laid out' icon. The laid out data will
     * also be resized for the screen's aspect ratio,
     * so the nil_Width and nil_Height will probably
     * be different than the original image on some
     * screens.
     */
    struct Screen    *ni_Screen;      /* Screen for the layout */
    ULONG             ni_Width;       /* Dimension of the aspect */
    ULONG             ni_Height;      /* ratio corrected icon */

    /* Pens for drawing the border and frame */
    UWORD ni_Pens[NUMDRIPENS];        /* Copied from DrawInfo for the screen */

    struct NativeIconFace {
        UBYTE         Aspect;        /* Source aspect ratio */
        ULONG         Width;
        ULONG         Height;
    } ni_Face;

    struct NativeIconImage {
        /* This data was either allocated during icon load
         * (and is in the ni_FreeList), or was provided by
         * the user via IconControlA().
         */
        LONG           TransparentColor;
        ULONG          Pens;        /* Pens allocated for the layout */
        const struct ColorRegister *Palette;      /*  one entry per pen */
        const UBYTE         *ImageData;   /* 'ChunkyPixels' image */
        const ULONG         *ARGB;        /* RGB+Alpha (A 0=transparent) */

        /* Dynamically allocated by LayoutIconA(), and are
         * _not_ in the ni_FreeList.
         *
         * You must call LayoutIconA(icon, NULL, NULL) or
         * FreeDiskObject() to free this memory.
         */
        ULONG         *Pen;         /* Palette n to Pen m mapping */
        struct BitMap *BitMap;      /* 'friend' of the Screen */
        PLANEPTR       BitMask;     /* TransparentColor >= 0 bitmask */
        APTR           ARGBMap;     /* ARGB, rescaled version */

        /* For m68k legacy support, the struct Image render
         * is stored here.
         */
        struct Image   Render;
    } ni_Image[2];
};

#define RSS_OLDDRAWERDATA_READ  (1 << 0)
#define RSS_GADGETIMAGE_READ	(1 << 1)
#define RSS_SELECTIMAGE_READ	(1 << 2)
#define RSS_DEFAULTTOOL_READ	(1 << 3)
#define RSS_TOOLWINDOW_READ	(1 << 4)
#define RSS_TOOLTYPES_READ	(1 << 5)

#define NATIVEICON(icon) ((struct NativeIcon *)((UBYTE *)(icon) - offsetof(struct NativeIcon, ni_DiskObject)))

struct IconBase
{
    struct Library          ib_Lib;

    struct Hook             dsh;
    struct SignalSemaphore  iconlistlock;
    struct MinList          iconlists[ICONLIST_HASHSIZE];
    
    ULONG   	    	    *ib_CRCTable;
    
    /* Global settings -----------------------------------------------------*/
    struct Screen          *ib_Screen;
    LONG                    ib_Precision;
    struct Rectangle        ib_EmbossRectangle;
    BOOL                    ib_Frameless;
    ULONG                   ib_ScaleBox;
    struct Hook            *ib_IdentifyHook;
    LONG                    ib_MaxNameLength;
    BOOL                    ib_NewIconsSupport;
    BOOL                    ib_ColorIconSupport;
    BPTR                    ib_SegList;

    /* Required External libraries */
    APTR                    ib_DOSBase;
    APTR                    ib_GfxBase;
    APTR                    ib_IntuitionBase;
    APTR                    ib_UtilityBase;

    /* Optional external libraries */
    APTR                    ib_CyberGfxBase;
    APTR                    ib_IFFParseBase;
    APTR                    ib_DataTypesBase;
    APTR                    ib_WorkbenchBase;
};

typedef struct IconBase IconBase_T;

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#define DOSBase		(IconBase->ib_DOSBase)
#define GfxBase		(IconBase->ib_GfxBase)
#define IntuitionBase	(IconBase->ib_IntuitionBase)
#define UtilityBase	(IconBase->ib_UtilityBase)

/****************************************************************************************/

extern struct ExecBase *SysBase;

/****************************************************************************************/
/* On-demand open of optional libraries. Just that is
 * non-NULL before you use it!
 */
extern const LONG IFFParseBase_version,
                  GfxBase_version,
                  CyberGfxBase_version,
                  DataTypesBase_version;

static inline APTR DemandOpenLibrary(struct Library **libp, CONST_STRPTR libname, ULONG version)
{
    if (*libp == NULL)
        *libp = OpenLibrary(libname, version);

    return *libp;
}

#define IFFParseBase	DemandOpenLibrary((struct Library **)&IconBase->ib_IFFParseBase, "iffparse.library", IFFParseBase_version)
#define CyberGfxBase	DemandOpenLibrary((struct Library **)&IconBase->ib_CyberGfxBase, "cybergraphics.library", CyberGfxBase_version)
#define DataTypesBase	DemandOpenLibrary((struct Library **)&IconBase->ib_DataTypesBase, "datatypes.library", DataTypesBase_version)
#define WorkbenchBase	DemandOpenLibrary((struct Library **)&IconBase->ib_WorkbenchBase, "workbench.library", 0)

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
VOID MakeMask35(UBYTE *imgdata, UBYTE *imgmask, UBYTE transpcolor, LONG imagew, LONG imageh);

BOOL ReadIconNI(struct NativeIcon *icon, struct Hook *streamhook, void *stream, struct IconBase *IconBase);
BOOL WriteIconNI(struct NativeIcon *icon, struct Hook *streamhook, void *stream, struct IconBase *IconBase);
VOID FreeIconNI(struct NativeIcon *icon, struct IconBase *IconBase);

/* Returns an ARGB image.
 * Set &width == ~0 and &height == ~0 to get the size.
 * Otherwise, sets the image size of width & height
 */
ULONG *ReadMemPNG(struct DiskObject *icon, APTR stream, LONG *width, LONG *height, const CONST_STRPTR *chunknames, APTR *chunkpointer, struct IconBase *IconBase);

BOOL ReadIconPNG(struct DiskObject *dobj, BPTR file, struct IconBase *IconBase);
BOOL WriteIconPNG(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase);
VOID FreeIconPNG(struct DiskObject *dobj, struct IconBase *IconBase);


#define LB(ib)          ((struct IconBase *) (ib))

#endif /* ICON_INTERN_H */
