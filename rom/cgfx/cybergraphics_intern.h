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

    struct SignalSemaphore pixbuf_sema;
    ULONG *pixel_buf;

    OOP_AttrBase    	     hiddBitMapAttrBase;
    OOP_AttrBase    	     hiddGCAttrBase;
    OOP_AttrBase    	     hiddSyncAttrBase;
    OOP_AttrBase    	     hiddPixFmtAttrBase;
    OOP_AttrBase    	     hiddGfxAttrBase;
};

#define GetCGFXBase(base) ((struct IntCGFXBase *)base)

#define __IHidd_BitMap      CyberGfxBase->hiddBitMapAttrBase
#define __IHidd_GC          CyberGfxBase->hiddGCAttrBase
#define __IHidd_Sync        CyberGfxBase->hiddSyncAttrBase
#define __IHidd_PixFmt      CyberGfxBase->hiddPixFmtAttrBase
#define __IHidd_Gfx         CyberGfxBase->hiddGfxAttrBase

/* Pixelbuffer, the same as in graphics.library */

#define NUMPIX 50000
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

extern APTR driver_AllocCModeListTagList(struct TagItem *taglist, struct IntCGFXBase *CyberGfxBase);
extern VOID driver_FreeCModeList(struct List *modeList, struct IntCGFXBase *CyberGfxBase);
extern ULONG driver_BestCModeIDTagList(struct TagItem *tags, struct IntCGFXBase *CyberGfxBase);
extern ULONG driver_GetCyberIDAttr(ULONG attr, ULONG id, struct IntCGFXBase *CyberGfxBase);
extern BOOL driver_IsCyberModeID(ULONG modeid, struct IntCGFXBase *CyberGfxBase);

extern VOID driver_CVideoCtrlTagList(struct ViewPort *vp, struct TagItem *tags, struct IntCGFXBase *CyberGfxBase);
extern ULONG driver_GetCyberMapAttr(struct BitMap *bitMap, ULONG attribute, struct IntCGFXBase *CyberGfxBase);
extern LONG driver_WriteLUTPixelArray(APTR srcrect, 
	UWORD srcx, UWORD srcy,
	UWORD srcmod, struct RastPort *rp, APTR ctable,
	UWORD destx, UWORD desty,
	UWORD sizex, UWORD sizey,
	UBYTE ctabformat,
	struct IntCGFXBase *CyberGfxBase);

extern ULONG driver_ExtractColor(struct RastPort *RastPort, struct BitMap *SingleMap, ULONG Colour, ULONG sX, ULONG sY, ULONG Width, ULONG Height, struct IntCGFXBase *CyberGfxBase);
extern ULONG driver_MovePixelArray(UWORD SrcX, UWORD SrcY, struct RastPort *RastPort, UWORD DstX, UWORD DstY, UWORD SizeX, UWORD SizeY, struct IntCGFXBase *CyberGfxBase);
extern LONG driver_FillPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, ULONG pixel, struct IntCGFXBase *CyberGfxBase);

extern LONG driver_InvertPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct IntCGFXBase *CyberGfxBase);

extern LONG driver_ReadPixelArray(APTR dst, UWORD destx, UWORD desty
	, UWORD dstmod, struct RastPort *rp, UWORD srcx, UWORD srcy
	, UWORD width, UWORD height, UBYTE dstformat, struct IntCGFXBase *CyberGfxBase);

extern LONG driver_WriteRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, ULONG pixel, struct IntCGFXBase *CyberGfxBase);
	
extern ULONG driver_ReadRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, struct IntCGFXBase *CyberGfxBase);

extern LONG driver_WritePixelArray(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, UBYTE srcformat, struct IntCGFXBase *CyberGfxBase);

extern LONG driver_WritePixelArrayAlpha(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, ULONG globalalpha, struct IntCGFXBase *CyberGfxBase);

extern void driver_BltTemplateAlpha(UBYTE *src, LONG srcx, LONG srcmod
    	, struct RastPort *rp, LONG destx, LONG desty, LONG width, LONG height
	, struct IntCGFXBase *CyberGfxBase);

extern VOID driver_UnLockBitMapTagList(APTR handle, struct TagItem *tags, struct IntCGFXBase *CyberGfxBase);
extern VOID driver_UnLockBitMap(APTR handle, struct IntCGFXBase *CyberGfxBase);
extern APTR driver_LockBitMapTagList(struct BitMap *bm, struct TagItem *tags, struct IntCGFXBase *CyberGfxBase);

extern VOID driver_DoCDrawMethodTagList(struct Hook *hook, struct RastPort *rp, struct TagItem *tags, struct IntCGFXBase *CyberGfxBase);

#endif /* CYBERGRAPHICS_INTERN_H */
