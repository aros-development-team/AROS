/* Automatically generated header! Do not edit! */

#ifndef _INLINE_GFXHIDD_H
#define _INLINE_GFXHIDD_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef GFXHIDD_BASE_NAME
#define GFXHIDD_BASE_NAME GfxHiddBase
#endif /* !GFXHIDD_BASE_NAME */

#define HIDDF_Graphics_TestSpeed(val1, val2, val3) \
	LP3NR(0x1e, HIDDF_Graphics_TestSpeed, ULONG, val1, d2, ULONG, val2, d3, ULONG, val3, d4, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_CreateBitMap(width, height, depth, flags, friendBitMap, tagList) \
	LP6(0x24, APTR, HIDD_Graphics_CreateBitMap, ULONG, width, d2, ULONG, height, d3, ULONG, depth, d4, ULONG, flags, d5, APTR, friendBitMap, a2, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_CreateBitMapTags(a0, a1, a2, a3, a4, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_CreateBitMap((a0), (a1), (a2), (a3), (a4), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_CreateGC(bitMap, tagList) \
	LP2(0x42, APTR, HIDD_Graphics_CreateGC, APTR, bitMap, a2, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_CreateGCTags(a0, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_CreateGC((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_DeleteBitMap(bitMap, tagList) \
	LP2NR(0x2a, HIDD_Graphics_DeleteBitMap, APTR, bitMap, a2, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_DeleteBitMapTags(a0, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_DeleteBitMap((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_DeleteGC(gc, tagList) \
	LP2NR(0x48, HIDD_Graphics_DeleteGC, APTR, gc, a2, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_DeleteGCTags(a0, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_DeleteGC((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_DepthArrangeBitMap(bitMap, mode, otherBitMap, tagList) \
	LP4A4(0x3c, ULONG, HIDD_Graphics_DepthArrangeBitMap, APTR, bitMap, a2, ULONG, mode, d2, APTR, otherBitMap, a3, struct TagItem *, tagList, d7, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_DepthArrangeBitMapTags(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_DepthArrangeBitMap((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_DrawLine(gc, x1, y1, x2, y2) \
	LP5(0x78, BOOL, HIDD_Graphics_DrawLine, APTR, gc, a2, WORD, x1, d2, WORD, y1, d3, WORD, x2, d4, WORD, y2, d5, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_DrawLine_Q(gc, x1, y1, x2, y2) \
	LP5NR(0x7e, HIDD_Graphics_DrawLine_Q, APTR, gc, a2, WORD, x1, d2, WORD, y1, d3, WORD, x2, d4, WORD, y2, d5, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_MoveBitMap(bitMap, horizontal, vertical, tagList) \
	LP4NR(0x36, HIDD_Graphics_MoveBitMap, APTR, bitMap, a2, WORD, horizontal, d2, WORD, vertical, d3, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_MoveBitMapTags(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_MoveBitMap((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_ReadPixel(gc, x, y) \
	LP3(0x5a, ULONG, HIDD_Graphics_ReadPixel, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_ReadPixelDirect(gc, x, y) \
	LP3(0x6c, ULONG, HIDD_Graphics_ReadPixelDirect, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_ReadPixelDirect_Q(gc, x, y) \
	LP3(0x72, ULONG, HIDD_Graphics_ReadPixelDirect_Q, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_ReadPixel_Q(gc, x, y) \
	LP3(0x60, ULONG, HIDD_Graphics_ReadPixel_Q, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_ShowBitMap(bitMap, wait, tagList) \
	LP3(0x30, BOOL, HIDD_Graphics_ShowBitMap, APTR, bitMap, a2, BOOL, wait, d2, struct TagItem *, tagList, a3, \
	, GFXHIDD_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define HIDD_Graphics_ShowBitMapTags(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; HIDD_Graphics_ShowBitMap((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define HIDD_Graphics_WritePixel(gc, x, y) \
	LP3(0x54, BOOL, HIDD_Graphics_WritePixel, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_WritePixelDirect(gc, x, y, val) \
	LP4(0x4e, BOOL, HIDD_Graphics_WritePixelDirect, APTR, gc, a2, WORD, x, d2, WORD, y, d3, ULONG, val, d4, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_WritePixelDirect_Q(gc, x, y, val) \
	LP4NR(0x66, HIDD_Graphics_WritePixelDirect_Q, APTR, gc, a2, WORD, x, d2, WORD, y, d3, ULONG, val, d4, \
	, GFXHIDD_BASE_NAME)

#define HIDD_Graphics_WritePixel_Q(gc, x, y) \
	LP3NR(0x84, HIDD_Graphics_WritePixel_Q, APTR, gc, a2, WORD, x, d2, WORD, y, d3, \
	, GFXHIDD_BASE_NAME)

#endif /* !_INLINE_GFXHIDD_H */
