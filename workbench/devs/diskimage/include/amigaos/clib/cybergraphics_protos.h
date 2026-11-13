/*
**	$VER: cybergraphics_protos.h 41.18 (21.02.1998)
**
**	C prototypes for cybergraphics.library
**
**	Copyright © 1996-1998 by phase5 digital products
**      All Rights reserved.
**
*/

#ifndef CLIB_CYBERGRAPHICS_H
#define CLIB_CYBERGRAPHICS_H 1

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

struct List *AllocCModeListTagList(struct TagItem *);
struct List *AllocCModeListTags(Tag, ...);
ULONG        BestCModeIDTagList(struct TagItem *);
ULONG        BestCModeIDTags(Tag, ...);
ULONG        CModeRequestTagList(APTR, struct TagItem *);
ULONG        CModeRequestTags(APTR, Tag, ...);
void         CVideoCtrlTagList(struct ViewPort *, struct TagItem *);
void         CVideoCtrlTags(struct ViewPort *, Tag tag1, ...);
void         DoCDrawMethodTagList(struct Hook *, struct RastPort *, struct TagItem *);
void         DoCDrawMethodTags(struct Hook *, struct RastPort *, Tag, ...);
ULONG	     ExtractColor(struct RastPort *,struct BitMap *,ULONG,ULONG,ULONG,ULONG,ULONG);
ULONG        FillPixelArray(struct RastPort *, UWORD, UWORD, UWORD, UWORD, ULONG);
void         FreeCModeList(struct List *);
ULONG        GetCyberIDAttr(ULONG, ULONG);
ULONG        GetCyberMapAttr(struct BitMap *, ULONG);
ULONG        InvertPixelArray(struct RastPort *, UWORD, UWORD, UWORD, UWORD);
BOOL         IsCyberModeID(ULONG);
APTR         LockBitMapTagList(APTR,struct TagItem *);
APTR         LockBitMapTags(APTR, Tag, ...);
ULONG        MovePixelArray(UWORD, UWORD, struct RastPort *, UWORD, UWORD, UWORD,
                            UWORD);
ULONG        ReadPixelArray(APTR, UWORD, UWORD, UWORD, struct RastPort *, UWORD,
                            UWORD, UWORD, UWORD, UBYTE);
ULONG        ReadRGBPixel(struct RastPort *, UWORD, UWORD);
LONG         ScalePixelArray(APTR,UWORD,UWORD,UWORD,struct RastPort *,UWORD,
			     UWORD,UWORD,UWORD,UBYTE);
void         UnLockBitMap(APTR);
ULONG        WritePixelArray(APTR, UWORD, UWORD, UWORD, struct RastPort *, UWORD,
                             UWORD, UWORD, UWORD, UBYTE);
ULONG        WriteLUTPixelArray(APTR, UWORD, UWORD, UWORD, struct RastPort *, APTR,
			        UWORD, UWORD, UWORD, UWORD, UBYTE);
LONG         WriteRGBPixel(struct RastPort *, UWORD, UWORD, ULONG);
void         UnLockBitMapTagList(APTR, struct TagItem *);
void         UnLockBitMapTags(APTR, Tag, ...);

#endif /* !CLIB_CYBERGRAPHICS_H */
