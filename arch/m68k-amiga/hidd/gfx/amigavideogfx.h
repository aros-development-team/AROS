#ifndef HIDD_AMIGAVIDEO_H
#define HIDD_AMIGAVIDEO_H

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>

#include "amigavideoclass.h"

#define __IHidd_BitMap	    (csd->hiddBitMapAttrBase)
#define __IHidd_PlanarBM	(csd->hiddPlanarBitMapAttrBase)
#define __IHidd_AmigaVideoBitmap (csd->hiddAmigaVideoBitMapAttrBase)
#define __IHidd_GC			(csd->hiddGCAttrBase)
#define __IHidd_Sync	    (csd->hiddSyncAttrBase)
#define __IHidd_PixFmt		(csd->hiddPixFmtAttrBase)
#define __IHidd_Gfx 	    (csd->hiddGfxAttrBase)
#define __IHidd_Attr		(csd->hiddAttrBase)
#define __IHidd_ColorMap	(csd->hiddColorMapAttrBase)

struct amigavideo_staticdata
{
    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *bmclass;

	OOP_AttrBase hiddBitMapAttrBase;  
	OOP_AttrBase hiddPlanarBitMapAttrBase;
	OOP_AttrBase hiddAmigaVideoBitMapAttrBase;
	OOP_AttrBase hiddGCAttrBase;
	OOP_AttrBase hiddSyncAttrBase;
	OOP_AttrBase hiddPixFmtAttrBase;
	OOP_AttrBase hiddGfxAttrBase;
	OOP_AttrBase hiddAttrBase;
	OOP_AttrBase hiddColorMapAttrBase;

	UWORD width_alignment;

	UWORD *copper1;
	UWORD *copper1_pt2;
	UWORD *copper1_spritept;
	WORD sprite_width, sprite_height;
	UWORD *copper2;
	UWORD *copper2i;
	UWORD *copper2_backup;
	UWORD *copper2_palette;
	UWORD *copper2i_palette;
	UWORD *copper2_palette_aga_lo;
	UWORD *copper2i_palette_aga_lo;
	UWORD *sprite_null;
	UWORD *sprite;
	UWORD bplcon3;

	UWORD max_colors;

	UBYTE *palette;
	UBYTE depth;
	UBYTE res; // 0 = lores, 1 = hires, 2 = shres
	UBYTE interlace;
	BOOL aga;

	UBYTE initialized;
};

struct amigavideoclbase
{
    struct Library        library;
    
    struct amigavideo_staticdata csd;
};

#undef CSD
#define CSD(cl)     	(&((struct amigavideoclbase *)cl->UserData)->csd)

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    struct MinList bitmaps;		/* Currently shown bitmap objects       */
};

#endif /* HIDD_AMIGAVIDEO_H */
