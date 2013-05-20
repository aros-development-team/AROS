/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef CYBERGRAPHICS_INTERN_H
#define CYBERGRAPHICS_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif

#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif

#include <oop/oop.h>

struct IntCGFXBase
{
    struct Library libnode;

    struct SignalSemaphore  pixbuf_sema;
    ULONG		   *pixel_buf;

    OOP_AttrBase    	    hiddBitMapAttrBase;
    OOP_AttrBase    	    hiddGCAttrBase;
    OOP_AttrBase    	    hiddSyncAttrBase;
    OOP_AttrBase    	    hiddPixFmtAttrBase;
    OOP_AttrBase    	    hiddGfxAttrBase;

    ULONG	 	    greytab[256];	/* Grayscale palette for RECTFMT_GREY8 */
};

#define GetCGFXBase(base) ((struct IntCGFXBase *)base)

#define __IHidd_BitMap      GetCGFXBase(CyberGfxBase)->hiddBitMapAttrBase
#define __IHidd_GC          GetCGFXBase(CyberGfxBase)->hiddGCAttrBase
#define __IHidd_Sync        GetCGFXBase(CyberGfxBase)->hiddSyncAttrBase
#define __IHidd_PixFmt      GetCGFXBase(CyberGfxBase)->hiddPixFmtAttrBase
#define __IHidd_Gfx         GetCGFXBase(CyberGfxBase)->hiddGfxAttrBase

/* Pixelbuffer, the same as in graphics.library */
#ifdef __mc68000
#define NUMPIX 4096 	/* Not that much room to spare */
#else
#define NUMPIX 100000
#endif

#define PIXELBUF_SIZE (NUMPIX * 4)

#define LOCK_PIXBUF ObtainSemaphore(&CyberGfxBase->pixbuf_sema);
#define ULOCK_PIXBUF ReleaseSemaphore(&CyberGfxBase->pixbuf_sema);

/* Bitmap processing */

#define XCOORD_TO_BYTEIDX( x ) 	(( x ) >> 3)

#define COORD_TO_BYTEIDX(x, y, bytes_per_row)	\
				( ( ((LONG)(y)) * (bytes_per_row)) + XCOORD_TO_BYTEIDX(x))

#define CHUNKY8_COORD_TO_BYTEIDX(x, y, bytes_per_row)	\
				( ( ((LONG)(y)) * (bytes_per_row)) + (x) )

#define XCOORD_TO_MASK(x)   	(128L >> ((x) & 0x07))

#endif /* CYBERGRAPHICS_INTERN_H */
