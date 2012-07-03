#ifndef HIDD_AMIGAVIDEO_H
#define HIDD_AMIGAVIDEO_H

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <graphics/gfxbase.h>

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

struct copper2data
{
	UWORD *copper2;
	UWORD *copper2_palette;
	UWORD *copper2_palette_aga_lo;
	UWORD *copper2_scroll;
	UWORD *copper2_bplcon0;
	UWORD *copper2_bpl;
	UWORD *copper2_fmode;
};

struct NativeChipsetMode
{
	struct Node node;
	ULONG modeid;
	UWORD width, height, depth;
	OOP_Object *pf;
	OOP_Object *sync;
	UBYTE special;
};

struct amigavideo_staticdata
{
    OOP_Class 	    	    *amigagfxclass;
    OOP_Class 	    	    *amigabmclass;

	OOP_AttrBase hiddBitMapAttrBase;  
	OOP_AttrBase hiddPlanarBitMapAttrBase;
	OOP_AttrBase hiddAmigaVideoBitMapAttrBase;
	OOP_AttrBase hiddGCAttrBase;
	OOP_AttrBase hiddSyncAttrBase;
	OOP_AttrBase hiddPixFmtAttrBase;
	OOP_AttrBase hiddGfxAttrBase;
	OOP_AttrBase hiddAttrBase;
	OOP_AttrBase hiddColorMapAttrBase;
	
	struct List nativemodelist;
	BOOL superforward;

	struct amigabm_data *disp;
	ULONG modeid;
	struct Interrupt inter;
	volatile UWORD framecounter;
	struct amigabm_data *updatescroll;

	WORD width_alignment;
	WORD startx, starty;
	WORD width, height;

	UWORD *copper1;
	UWORD *copper1_pt2;
	UWORD *copper1_spritept;
	UWORD *copper2_backup;
	UWORD spritedatasize;
	WORD sprite_width, sprite_height;
	UWORD spritepos, spritectl;
	UWORD *sprite_null;
	UWORD *sprite;
	WORD  spritex, spritey;
	BYTE sprite_offset_x, sprite_offset_y;
	BYTE sprite_res;
	UWORD bplcon0_null, bplcon3;
	UBYTE fmode_bpl, fmode_spr;
	UWORD ddfstrt, ddfstop;
	WORD modulo;
	struct copper2data copper2;
	struct copper2data copper2i;

	UWORD max_colors;
	UWORD use_colors;

	UBYTE *palette;
	UBYTE depth;
	UBYTE res; // 0 = lores, 1 = hires, 2 = shres
	UBYTE interlace;
	UBYTE extralines;
	BOOL ecs_agnus, ecs_denise, aga;
	BOOL cursorvisible;
	BOOL palmode;

	UBYTE initialized;
	UBYTE bploffsets[8];

	void (*acb)(void *data, void *bm);
	APTR acbdata;

    BPTR  cs_SegList;
    struct Library *cs_OOPBase;
    struct Library *cs_GfxBase;
    struct Library *cs_UtilityBase;

    OOP_MethodID    cs_HiddGfxBase;
    OOP_MethodID    cs_HiddBitMapBase;
};

struct amigavideoclbase
{
    struct Library        library;
    
    struct amigavideo_staticdata csd;
};

#undef CSD
#define CSD(cl)     	(&((struct amigavideoclbase *)cl->UserData)->csd)

/* Private instance data for Gfx hidd class */
struct amigagfx_data
{
    struct MinList bitmaps;		/* Currently shown bitmap objects       */
};

#endif /* HIDD_AMIGAVIDEO_H */
