#ifndef HIDD_UAEGFX_H
#define HIDD_UAEGFX_H

#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>

#include "uaegfxclass.h"

#define __IHidd_BitMap	    (csd->hiddBitMapAttrBase)
#define __IHidd_UAEGFXBitmap (csd->hiddUAEGFXBitMapAttrBase)
#define __IHidd_GC			(csd->hiddGCAttrBase)
#define __IHidd_Sync	    (csd->hiddSyncAttrBase)
#define __IHidd_PixFmt		(csd->hiddPixFmtAttrBase)
#define __IHidd_Gfx 	    (csd->hiddGfxAttrBase)
#define __IHidd_Attr		(csd->hiddAttrBase)
#define __IHidd_ColorMap	(csd->hiddColorMapAttrBase)

#define HiddBitMapBase		(csd->hiddBitMapBase)
#define HiddColorMapBase	(csd->hiddColorMapBase)
#define HiddGfxBase		(csd->hiddGfxBase)

#include <hidd/graphics_inline.h>

#define UtilityBase ((csd)->cs_UtilityBase)

struct RTGMode
{
	struct Node node;
	ULONG modeid;
	UWORD width, height;
	OOP_Object *pf;
	OOP_Object *sync;
};

struct uaegfx_staticdata
{
    OOP_Class 	    	    *gfxclass;
    OOP_Class 	    	    *bmclass;

	OOP_AttrBase hiddBitMapAttrBase;  
	OOP_AttrBase hiddUAEGFXBitMapAttrBase;
	OOP_AttrBase hiddGCAttrBase;
	OOP_AttrBase hiddSyncAttrBase;
	OOP_AttrBase hiddPixFmtAttrBase;
	OOP_AttrBase hiddGfxAttrBase;
	OOP_AttrBase hiddAttrBase;
	OOP_AttrBase hiddColorMapAttrBase;

	OOP_MethodID hiddBitMapBase;
	OOP_MethodID hiddColorMapBase;
	OOP_MethodID hiddGfxBase;
	
	struct List rtglist;
	struct Library *CardBase;
	struct Library *cs_IntuitionBase;
	struct Library *cs_UtilityBase;
	struct bm_data *disp;
	APTR uaeromvector;
	ULONG rgbformat;
	struct ModeInfo *modeinfo;
	UBYTE *boardinfo;
	UBYTE *bitmapextra;
	UBYTE *vram_start;
	ULONG vram_size;
	struct MemHeader *vmem;
	
	WORD sprite_width, sprite_height;
	BOOL hardwaresprite;
	WORD spritecolors;

	BOOL initialized;
	BOOL superforward; 
	
	UWORD dwidth, dheight;
	ULONG dmodeid;

	struct ViewPort *viewport;
    void (*acb)(void *data, void *bm);
    APTR acbdata;
};

struct UAEGFXclbase
{
    struct Library        library;
    
    struct uaegfx_staticdata csd;
    IPTR                  cs_SegList;
};

#undef CSD
#define CSD(cl)     	(&((struct UAEGFXclbase *)cl->UserData)->csd)

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    struct MinList bitmaps;		/* Currently shown bitmap objects       */
};

#endif

