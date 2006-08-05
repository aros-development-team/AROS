#ifndef CGXVIDEO_INTERN_H
#define CGXVIDEO_INTERN_H

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

struct IntCGXVBase
{
    struct Library libnode;
};


extern APTR driver_AllocCModeListTagList(struct TagItem *taglist, struct GfxBase *GfxBase);
extern VOID driver_FreeCModeList(struct List *modeList, struct GfxBase *GfxBase);
extern ULONG driver_BestCModeIDTagList(struct TagItem *tags, struct GfxBase *GfxBase);
extern ULONG driver_GetCyberIDAttr(ULONG attr, ULONG id, struct GfxBase *GfxBase);
extern BOOL driver_IsCyberModeID(ULONG modeid, struct GfxBase *GfxBase);

#define GetCGXVBase(base) ((struct IntCGXVBase *)base)


extern VOID driver_CVideoCtrlTagList(struct ViewPort *vp, struct TagItem *tags, struct Library *CGXVideoBase);
extern ULONG driver_GetCyberMapAttr(struct BitMap *bitMap, ULONG attribute, struct Library *CGXVideoBase);
extern LONG driver_WriteLUTPixelArray(APTR srcrect, 
	UWORD srcx, UWORD srcy,
	UWORD srcmod, struct RastPort *rp, APTR ctable,
	UWORD destx, UWORD desty,
	UWORD sizex, UWORD sizey,
	UBYTE ctabformat,
	struct Library *CGXVideoBase);

extern ULONG driver_ExtractColor(struct RastPort *RastPort, struct BitMap *SingleMap, ULONG Colour, ULONG sX, ULONG sY, ULONG Width, ULONG Height, struct Library *CGXVideoBase);
extern ULONG driver_MovePixelArray(UWORD SrcX, UWORD SrcY, struct RastPort *RastPort, UWORD DstX, UWORD DstY, UWORD SizeX, UWORD SizeY, struct Library *CGXVideoBase);
extern LONG driver_FillPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, ULONG pixel, struct Library *CGXVideoBase);

extern LONG driver_InvertPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct Library *CGXVideoBase);

extern LONG driver_ReadPixelArray(APTR dst, UWORD destx, UWORD desty
	, UWORD dstmod, struct RastPort *rp, UWORD srcx, UWORD srcy
	, UWORD width, UWORD height, UBYTE dstformat, struct Library *CGXVideoBase);

extern LONG driver_WriteRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, ULONG pixel, struct Library *CGXVideoBase);
	
extern ULONG driver_ReadRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, struct Library *CGXVideoBase);

extern LONG driver_WritePixelArray(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, UBYTE srcformat, struct Library *CGXVideoBase);

extern LONG driver_WritePixelArrayAlpha(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, ULONG globalalpha, struct Library *CGXVideoBase);

extern void driver_BltTemplateAlpha(UBYTE *src, LONG srcx, LONG srcmod
    	, struct RastPort *rp, LONG destx, LONG desty, LONG width, LONG height
	, struct Library *CGXVideoBase);

extern VOID driver_UnLockBitMapTagList(APTR handle, struct TagItem *tags, struct Library *CGXVideoBase);
extern VOID driver_UnLockBitMap(APTR handle, struct Library *CGXVideoBase);
extern APTR driver_LockBitMapTagList(struct BitMap *bm, struct TagItem *tags, struct Library *CGXVideoBase);


#endif /* CGXVIDEO_INTERN_H */
