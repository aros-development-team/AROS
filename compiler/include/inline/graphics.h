#ifndef _INLINE_GRAPHICS_H
#define _INLINE_GRAPHICS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef GRAPHICS_BASE_NAME
#define GRAPHICS_BASE_NAME GfxBase
#endif

#define AddAnimOb(anOb, anKey, rp) \
	LP3NR(0x9c, AddAnimOb, struct AnimOb *, anOb, a0, struct AnimOb **, anKey, a1, struct RastPort *, rp, a2, \
	, GRAPHICS_BASE_NAME)

#define AddBob(bob, rp) \
	LP2NR(0x60, AddBob, struct Bob *, bob, a0, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define AddFont(textFont) \
	LP1NR(0x1e0, AddFont, struct TextFont *, textFont, a1, \
	, GRAPHICS_BASE_NAME)

#define AddVSprite(vSprite, rp) \
	LP2NR(0x66, AddVSprite, struct VSprite *, vSprite, a0, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define AllocBitMap(sizex, sizey, depth, flags, friend_bitmap) \
	LP5(0x396, struct BitMap *, AllocBitMap, unsigned long, sizex, d0, unsigned long, sizey, d1, unsigned long, depth, d2, unsigned long, flags, d3, struct BitMap *, friend_bitmap, a0, \
	, GRAPHICS_BASE_NAME)

#define AllocDBufInfo(vp) \
	LP1(0x3c6, struct DBufInfo *, AllocDBufInfo, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define AllocRaster(width, height) \
	LP2(0x1ec, PLANEPTR, AllocRaster, unsigned long, width, d0, unsigned long, height, d1, \
	, GRAPHICS_BASE_NAME)

#define AllocSpriteDataA(bm, tags) \
	LP2(0x3fc, struct ExtSprite *, AllocSpriteDataA, struct BitMap *, bm, a2, struct TagItem *, tags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AllocSpriteData(a0, tags...) \
	({ULONG _tags[] = { tags }; AllocSpriteDataA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AndRectRegion(region, rectangle) \
	LP2NR(0x1f8, AndRectRegion, struct Region *, region, a0, struct Rectangle *, rectangle, a1, \
	, GRAPHICS_BASE_NAME)

#define AndRegionRegion(srcRegion, destRegion) \
	LP2(0x270, BOOL, AndRegionRegion, struct Region *, srcRegion, a0, struct Region *, destRegion, a1, \
	, GRAPHICS_BASE_NAME)

#define Animate(anKey, rp) \
	LP2NR(0xa2, Animate, struct AnimOb **, anKey, a0, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define AreaDraw(rp, x, y) \
	LP3(0x102, LONG, AreaDraw, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define AreaEllipse(rp, xCenter, yCenter, a, b) \
	LP5(0xba, LONG, AreaEllipse, struct RastPort *, rp, a1, long, xCenter, d0, long, yCenter, d1, long, a, d2, long, b, d3, \
	, GRAPHICS_BASE_NAME)

#define AreaEnd(rp) \
	LP1(0x108, LONG, AreaEnd, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define AreaMove(rp, x, y) \
	LP3(0xfc, LONG, AreaMove, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define AskFont(rp, textAttr) \
	LP2NR(0x1da, AskFont, struct RastPort *, rp, a1, struct TextAttr *, textAttr, a0, \
	, GRAPHICS_BASE_NAME)

#define AskSoftStyle(rp) \
	LP1(0x54, ULONG, AskSoftStyle, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define AttachPalExtra(cm, vp) \
	LP2(0x342, LONG, AttachPalExtra, struct ColorMap *, cm, a0, struct ViewPort *, vp, a1, \
	, GRAPHICS_BASE_NAME)

#define AttemptLockLayerRom(layer) \
	LP1A5(0x28e, BOOL, AttemptLockLayerRom, struct Layer *, layer, d7, \
	, GRAPHICS_BASE_NAME)

#define BestModeIDA(tags) \
	LP1(0x41a, ULONG, BestModeIDA, struct TagItem *, tags, a0, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define BestModeID(tags...) \
	({ULONG _tags[] = { tags }; BestModeIDA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define BitMapScale(bitScaleArgs) \
	LP1NR(0x2a6, BitMapScale, struct BitScaleArgs *, bitScaleArgs, a0, \
	, GRAPHICS_BASE_NAME)

#define BltBitMap(srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, minterm, mask, tempA) \
	LP11(0x1e, LONG, BltBitMap, struct BitMap *, srcBitMap, a0, long, xSrc, d0, long, ySrc, d1, struct BitMap *, destBitMap, a1, long, xDest, d2, long, yDest, d3, long, xSize, d4, long, ySize, d5, unsigned long, minterm, d6, unsigned long, mask, d7, PLANEPTR, tempA, a2, \
	, GRAPHICS_BASE_NAME)

#define BltBitMapRastPort(srcBitMap, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm) \
	LP9NR(0x25e, BltBitMapRastPort, struct BitMap *, srcBitMap, a0, long, xSrc, d0, long, ySrc, d1, struct RastPort *, destRP, a1, long, xDest, d2, long, yDest, d3, long, xSize, d4, long, ySize, d5, unsigned long, minterm, d6, \
	, GRAPHICS_BASE_NAME)

#define BltClear(memBlock, byteCount, flags) \
	LP3NR(0x12c, BltClear, PLANEPTR, memBlock, a1, unsigned long, byteCount, d0, unsigned long, flags, d1, \
	, GRAPHICS_BASE_NAME)

#define BltMaskBitMapRastPort(srcBitMap, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm, bltMask) \
	LP10NR(0x27c, BltMaskBitMapRastPort, struct BitMap *, srcBitMap, a0, long, xSrc, d0, long, ySrc, d1, struct RastPort *, destRP, a1, long, xDest, d2, long, yDest, d3, long, xSize, d4, long, ySize, d5, unsigned long, minterm, d6, PLANEPTR, bltMask, a2, \
	, GRAPHICS_BASE_NAME)

#define BltPattern(rp, mask, xMin, yMin, xMax, yMax, maskBPR) \
	LP7NR(0x138, BltPattern, struct RastPort *, rp, a1, PLANEPTR, mask, a0, long, xMin, d0, long, yMin, d1, long, xMax, d2, long, yMax, d3, unsigned long, maskBPR, d4, \
	, GRAPHICS_BASE_NAME)

#define BltTemplate(source, xSrc, srcMod, destRP, xDest, yDest, xSize, ySize) \
	LP8NR(0x24, BltTemplate, PLANEPTR, source, a0, long, xSrc, d0, long, srcMod, d1, struct RastPort *, destRP, a1, long, xDest, d2, long, yDest, d3, long, xSize, d4, long, ySize, d5, \
	, GRAPHICS_BASE_NAME)

#define CBump(copList) \
	LP1NR(0x16e, CBump, struct UCopList *, copList, a1, \
	, GRAPHICS_BASE_NAME)

#define CMove(copList, destination, data) \
	LP3NR(0x174, CMove, struct UCopList *, copList, a1, APTR, destination, d0, long, data, d1, \
	, GRAPHICS_BASE_NAME)

#define CWait(copList, v, h) \
	LP3NR(0x17a, CWait, struct UCopList *, copList, a1, long, v, d0, long, h, d1, \
	, GRAPHICS_BASE_NAME)

#define CalcIVG(v, vp) \
	LP2(0x33c, UWORD, CalcIVG, struct View *, v, a0, struct ViewPort *, vp, a1, \
	, GRAPHICS_BASE_NAME)

#define ChangeExtSpriteA(vp, oldsprite, newsprite, tags) \
	LP4(0x402, LONG, ChangeExtSpriteA, struct ViewPort *, vp, a0, struct ExtSprite *, oldsprite, a1, struct ExtSprite *, newsprite, a2, struct TagItem *, tags, a3, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ChangeExtSprite(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; ChangeExtSpriteA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ChangeSprite(vp, sprite, newData) \
	LP3NR(0x1a4, ChangeSprite, struct ViewPort *, vp, a0, struct SimpleSprite *, sprite, a1, PLANEPTR, newData, a2, \
	, GRAPHICS_BASE_NAME)

#define ChangeVPBitMap(vp, bm, db) \
	LP3NR(0x3ae, ChangeVPBitMap, struct ViewPort *, vp, a0, struct BitMap *, bm, a1, struct DBufInfo *, db, a2, \
	, GRAPHICS_BASE_NAME)

#define ClearEOL(rp) \
	LP1NR(0x2a, ClearEOL, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define ClearRectRegion(region, rectangle) \
	LP2(0x20a, BOOL, ClearRectRegion, struct Region *, region, a0, struct Rectangle *, rectangle, a1, \
	, GRAPHICS_BASE_NAME)

#define ClearRegion(region) \
	LP1NR(0x210, ClearRegion, struct Region *, region, a0, \
	, GRAPHICS_BASE_NAME)

#define ClearScreen(rp) \
	LP1NR(0x30, ClearScreen, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define ClipBlit(srcRP, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm) \
	LP9NR(0x228, ClipBlit, struct RastPort *, srcRP, a0, long, xSrc, d0, long, ySrc, d1, struct RastPort *, destRP, a1, long, xDest, d2, long, yDest, d3, long, xSize, d4, long, ySize, d5, unsigned long, minterm, d6, \
	, GRAPHICS_BASE_NAME)

#define CloseFont(textFont) \
	LP1NR(0x4e, CloseFont, struct TextFont *, textFont, a1, \
	, GRAPHICS_BASE_NAME)

#define CloseMonitor(monitorSpec) \
	LP1(0x2d0, BOOL, CloseMonitor, struct MonitorSpec *, monitorSpec, a0, \
	, GRAPHICS_BASE_NAME)

#define CoerceMode(vp, monitorid, flags) \
	LP3(0x3a8, ULONG, CoerceMode, struct ViewPort *, vp, a0, unsigned long, monitorid, d0, unsigned long, flags, d1, \
	, GRAPHICS_BASE_NAME)

#define CopySBitMap(layer) \
	LP1NR(0x1c2, CopySBitMap, struct Layer *, layer, a0, \
	, GRAPHICS_BASE_NAME)

#define DisownBlitter() \
	LP0NR(0x1ce, DisownBlitter, \
	, GRAPHICS_BASE_NAME)

#define DisposeRegion(region) \
	LP1NR(0x216, DisposeRegion, struct Region *, region, a0, \
	, GRAPHICS_BASE_NAME)

#define DoCollision(rp) \
	LP1NR(0x6c, DoCollision, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define Draw(rp, x, y) \
	LP3NR(0xf6, Draw, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define DrawEllipse(rp, xCenter, yCenter, a, b) \
	LP5NR(0xb4, DrawEllipse, struct RastPort *, rp, a1, long, xCenter, d0, long, yCenter, d1, long, a, d2, long, b, d3, \
	, GRAPHICS_BASE_NAME)

#define DrawGList(rp, vp) \
	LP2NR(0x72, DrawGList, struct RastPort *, rp, a1, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define EraseRect(rp, xMin, yMin, xMax, yMax) \
	LP5NR(0x32a, EraseRect, struct RastPort *, rp, a1, long, xMin, d0, long, yMin, d1, long, xMax, d2, long, yMax, d3, \
	, GRAPHICS_BASE_NAME)

#define ExtendFont(font, fontTags) \
	LP2(0x330, ULONG, ExtendFont, struct TextFont *, font, a0, struct TagItem *, fontTags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ExtendFontTags(a0, tags...) \
	({ULONG _tags[] = { tags }; ExtendFont((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define FindColor(cm, r, g, b, maxcolor) \
	LP5(0x3f0, LONG, FindColor, struct ColorMap *, cm, a3, unsigned long, r, d1, unsigned long, g, d2, unsigned long, b, d3, long, maxcolor, d4, \
	, GRAPHICS_BASE_NAME)

#define FindDisplayInfo(displayID) \
	LP1(0x2d6, DisplayInfoHandle, FindDisplayInfo, unsigned long, displayID, d0, \
	, GRAPHICS_BASE_NAME)

#define Flood(rp, mode, x, y) \
	LP4(0x14a, BOOL, Flood, struct RastPort *, rp, a1, unsigned long, mode, d2, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define FontExtent(font, fontExtent) \
	LP2NR(0x2fa, FontExtent, struct TextFont *, font, a0, struct TextExtent *, fontExtent, a1, \
	, GRAPHICS_BASE_NAME)

#define FreeBitMap(bm) \
	LP1NR(0x39c, FreeBitMap, struct BitMap *, bm, a0, \
	, GRAPHICS_BASE_NAME)

#define FreeColorMap(colorMap) \
	LP1NR(0x240, FreeColorMap, struct ColorMap *, colorMap, a0, \
	, GRAPHICS_BASE_NAME)

#define FreeCopList(copList) \
	LP1NR(0x222, FreeCopList, struct CopList *, copList, a0, \
	, GRAPHICS_BASE_NAME)

#define FreeCprList(cprList) \
	LP1NR(0x234, FreeCprList, struct cprlist *, cprList, a0, \
	, GRAPHICS_BASE_NAME)

#define FreeDBufInfo(dbi) \
	LP1NR(0x3cc, FreeDBufInfo, struct DBufInfo *, dbi, a1, \
	, GRAPHICS_BASE_NAME)

#define FreeGBuffers(anOb, rp, flag) \
	LP3NR(0x258, FreeGBuffers, struct AnimOb *, anOb, a0, struct RastPort *, rp, a1, long, flag, d0, \
	, GRAPHICS_BASE_NAME)

#define FreeRaster(p, width, height) \
	LP3NR(0x1f2, FreeRaster, PLANEPTR, p, a0, unsigned long, width, d0, unsigned long, height, d1, \
	, GRAPHICS_BASE_NAME)

#define FreeSprite(num) \
	LP1NR(0x19e, FreeSprite, long, num, d0, \
	, GRAPHICS_BASE_NAME)

#define FreeSpriteData(sp) \
	LP1NR(0x408, FreeSpriteData, struct ExtSprite *, sp, a2, \
	, GRAPHICS_BASE_NAME)

#define FreeVPortCopLists(vp) \
	LP1NR(0x21c, FreeVPortCopLists, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define GetAPen(rp) \
	LP1(0x35a, ULONG, GetAPen, struct RastPort *, rp, a0, \
	, GRAPHICS_BASE_NAME)

#define GetBPen(rp) \
	LP1(0x360, ULONG, GetBPen, struct RastPort *, rp, a0, \
	, GRAPHICS_BASE_NAME)

#define GetBitMapAttr(bm, attrnum) \
	LP2(0x3c0, ULONG, GetBitMapAttr, struct BitMap *, bm, a0, unsigned long, attrnum, d1, \
	, GRAPHICS_BASE_NAME)

#define GetColorMap(entries) \
	LP1(0x23a, struct ColorMap *, GetColorMap, long, entries, d0, \
	, GRAPHICS_BASE_NAME)

#define GetDisplayInfoData(handle, buf, size, tagID, displayID) \
	LP5(0x2f4, ULONG, GetDisplayInfoData, DisplayInfoHandle, handle, a0, UBYTE *, buf, a1, unsigned long, size, d0, unsigned long, tagID, d1, unsigned long, displayID, d2, \
	, GRAPHICS_BASE_NAME)

#define GetDrMd(rp) \
	LP1(0x366, ULONG, GetDrMd, struct RastPort *, rp, a0, \
	, GRAPHICS_BASE_NAME)

#define GetExtSpriteA(ss, tags) \
	LP2(0x3a2, LONG, GetExtSpriteA, struct ExtSprite *, ss, a2, struct TagItem *, tags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetExtSprite(a0, tags...) \
	({ULONG _tags[] = { tags }; GetExtSpriteA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GetGBuffers(anOb, rp, flag) \
	LP3(0xa8, BOOL, GetGBuffers, struct AnimOb *, anOb, a0, struct RastPort *, rp, a1, long, flag, d0, \
	, GRAPHICS_BASE_NAME)

#define GetOutlinePen(rp) \
	LP1(0x36c, ULONG, GetOutlinePen, struct RastPort *, rp, a0, \
	, GRAPHICS_BASE_NAME)

#define GetRGB32(cm, firstcolor, ncolors, table) \
	LP4NR(0x384, GetRGB32, struct ColorMap *, cm, a0, unsigned long, firstcolor, d0, unsigned long, ncolors, d1, ULONG *, table, a1, \
	, GRAPHICS_BASE_NAME)

#define GetRGB4(colorMap, entry) \
	LP2(0x246, ULONG, GetRGB4, struct ColorMap *, colorMap, a0, long, entry, d0, \
	, GRAPHICS_BASE_NAME)

#define GetRPAttrsA(rp, tags) \
	LP2NR(0x414, GetRPAttrsA, struct RastPort *, rp, a0, struct TagItem *, tags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetRPAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; GetRPAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GetSprite(sprite, num) \
	LP2(0x198, WORD, GetSprite, struct SimpleSprite *, sprite, a0, long, num, d0, \
	, GRAPHICS_BASE_NAME)

#define GetVPModeID(vp) \
	LP1(0x318, LONG, GetVPModeID, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define GfxAssociate(associateNode, gfxNodePtr) \
	LP2NR(0x2a0, GfxAssociate, APTR, associateNode, a0, APTR, gfxNodePtr, a1, \
	, GRAPHICS_BASE_NAME)

#define GfxFree(gfxNodePtr) \
	LP1NR(0x29a, GfxFree, APTR, gfxNodePtr, a0, \
	, GRAPHICS_BASE_NAME)

#define GfxLookUp(associateNode) \
	LP1(0x2be, APTR, GfxLookUp, APTR, associateNode, a0, \
	, GRAPHICS_BASE_NAME)

#define GfxNew(gfxNodeType) \
	LP1(0x294, APTR, GfxNew, unsigned long, gfxNodeType, d0, \
	, GRAPHICS_BASE_NAME)

#define InitArea(areaInfo, vectorBuffer, maxVectors) \
	LP3NR(0x11a, InitArea, struct AreaInfo *, areaInfo, a0, APTR, vectorBuffer, a1, long, maxVectors, d0, \
	, GRAPHICS_BASE_NAME)

#define InitBitMap(bitMap, depth, width, height) \
	LP4NR(0x186, InitBitMap, struct BitMap *, bitMap, a0, long, depth, d0, long, width, d1, long, height, d2, \
	, GRAPHICS_BASE_NAME)

#define InitGMasks(anOb) \
	LP1NR(0xae, InitGMasks, struct AnimOb *, anOb, a0, \
	, GRAPHICS_BASE_NAME)

#define InitGels(head, tail, gelsInfo) \
	LP3NR(0x78, InitGels, struct VSprite *, head, a0, struct VSprite *, tail, a1, struct GelsInfo *, gelsInfo, a2, \
	, GRAPHICS_BASE_NAME)

#define InitMasks(vSprite) \
	LP1NR(0x7e, InitMasks, struct VSprite *, vSprite, a0, \
	, GRAPHICS_BASE_NAME)

#define InitRastPort(rp) \
	LP1NR(0xc6, InitRastPort, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define InitTmpRas(tmpRas, buffer, size) \
	LP3(0x1d4, struct TmpRas *, InitTmpRas, struct TmpRas *, tmpRas, a0, PLANEPTR, buffer, a1, long, size, d0, \
	, GRAPHICS_BASE_NAME)

#define InitVPort(vp) \
	LP1NR(0xcc, InitVPort, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define InitView(view) \
	LP1NR(0x168, InitView, struct View *, view, a1, \
	, GRAPHICS_BASE_NAME)

#define LoadRGB32(vp, table) \
	LP2NR(0x372, LoadRGB32, struct ViewPort *, vp, a0, ULONG *, table, a1, \
	, GRAPHICS_BASE_NAME)

#define LoadRGB4(vp, colors, count) \
	LP3NR(0xc0, LoadRGB4, struct ViewPort *, vp, a0, UWORD *, colors, a1, long, count, d0, \
	, GRAPHICS_BASE_NAME)

#define LoadView(view) \
	LP1NR(0xde, LoadView, struct View *, view, a1, \
	, GRAPHICS_BASE_NAME)

#define LockLayerRom(layer) \
	LP1NRA5(0x1b0, LockLayerRom, struct Layer *, layer, d7, \
	, GRAPHICS_BASE_NAME)

#define MakeVPort(view, vp) \
	LP2(0xd8, ULONG, MakeVPort, struct View *, view, a0, struct ViewPort *, vp, a1, \
	, GRAPHICS_BASE_NAME)

#define ModeNotAvailable(modeID) \
	LP1(0x31e, LONG, ModeNotAvailable, unsigned long, modeID, d0, \
	, GRAPHICS_BASE_NAME)

#define Move(rp, x, y) \
	LP3NR(0xf0, Move, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define MoveSprite(vp, sprite, x, y) \
	LP4NR(0x1aa, MoveSprite, struct ViewPort *, vp, a0, struct SimpleSprite *, sprite, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define MrgCop(view) \
	LP1(0xd2, ULONG, MrgCop, struct View *, view, a1, \
	, GRAPHICS_BASE_NAME)

#define NewRegion() \
	LP0(0x204, struct Region *, NewRegion, \
	, GRAPHICS_BASE_NAME)

#define NextDisplayInfo(displayID) \
	LP1(0x2dc, ULONG, NextDisplayInfo, unsigned long, displayID, d0, \
	, GRAPHICS_BASE_NAME)

#define ObtainBestPenA(cm, r, g, b, tags) \
	LP5(0x348, LONG, ObtainBestPenA, struct ColorMap *, cm, a0, unsigned long, r, d1, unsigned long, g, d2, unsigned long, b, d3, struct TagItem *, tags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ObtainBestPen(a0, a1, a2, a3, tags...) \
	({ULONG _tags[] = { tags }; ObtainBestPenA((a0), (a1), (a2), (a3), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ObtainPen(cm, n, r, g, b, f) \
	LP6(0x3ba, ULONG, ObtainPen, struct ColorMap *, cm, a0, unsigned long, n, d0, unsigned long, r, d1, unsigned long, g, d2, unsigned long, b, d3, long, f, d4, \
	, GRAPHICS_BASE_NAME)

#define OpenFont(textAttr) \
	LP1(0x48, struct TextFont *, OpenFont, struct TextAttr *, textAttr, a0, \
	, GRAPHICS_BASE_NAME)

#define OpenMonitor(monitorName, displayID) \
	LP2(0x2ca, struct MonitorSpec *, OpenMonitor, STRPTR, monitorName, a1, unsigned long, displayID, d0, \
	, GRAPHICS_BASE_NAME)

#define OrRectRegion(region, rectangle) \
	LP2(0x1fe, BOOL, OrRectRegion, struct Region *, region, a0, struct Rectangle *, rectangle, a1, \
	, GRAPHICS_BASE_NAME)

#define OrRegionRegion(srcRegion, destRegion) \
	LP2(0x264, BOOL, OrRegionRegion, struct Region *, srcRegion, a0, struct Region *, destRegion, a1, \
	, GRAPHICS_BASE_NAME)

#define OwnBlitter() \
	LP0NR(0x1c8, OwnBlitter, \
	, GRAPHICS_BASE_NAME)

#define PolyDraw(rp, count, polyTable) \
	LP3NR(0x150, PolyDraw, struct RastPort *, rp, a1, long, count, d0, WORD *, polyTable, a0, \
	, GRAPHICS_BASE_NAME)

#define QBSBlit(blit) \
	LP1NR(0x126, QBSBlit, struct bltnode *, blit, a1, \
	, GRAPHICS_BASE_NAME)

#define QBlit(blit) \
	LP1NR(0x114, QBlit, struct bltnode *, blit, a1, \
	, GRAPHICS_BASE_NAME)

#define ReadPixel(rp, x, y) \
	LP3(0x13e, ULONG, ReadPixel, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define ReadPixelArray8(rp, xstart, ystart, xstop, ystop, array, temprp) \
	LP7(0x30c, LONG, ReadPixelArray8, struct RastPort *, rp, a0, unsigned long, xstart, d0, unsigned long, ystart, d1, unsigned long, xstop, d2, unsigned long, ystop, d3, UBYTE *, array, a2, struct RastPort *, temprp, a1, \
	, GRAPHICS_BASE_NAME)

#define ReadPixelLine8(rp, xstart, ystart, width, array, tempRP) \
	LP6(0x300, LONG, ReadPixelLine8, struct RastPort *, rp, a0, unsigned long, xstart, d0, unsigned long, ystart, d1, unsigned long, width, d2, UBYTE *, array, a2, struct RastPort *, tempRP, a1, \
	, GRAPHICS_BASE_NAME)

#define RectFill(rp, xMin, yMin, xMax, yMax) \
	LP5NR(0x132, RectFill, struct RastPort *, rp, a1, long, xMin, d0, long, yMin, d1, long, xMax, d2, long, yMax, d3, \
	, GRAPHICS_BASE_NAME)

#define ReleasePen(cm, n) \
	LP2NR(0x3b4, ReleasePen, struct ColorMap *, cm, a0, unsigned long, n, d0, \
	, GRAPHICS_BASE_NAME)

#define RemFont(textFont) \
	LP1NR(0x1e6, RemFont, struct TextFont *, textFont, a1, \
	, GRAPHICS_BASE_NAME)

#define RemIBob(bob, rp, vp) \
	LP3NR(0x84, RemIBob, struct Bob *, bob, a0, struct RastPort *, rp, a1, struct ViewPort *, vp, a2, \
	, GRAPHICS_BASE_NAME)

#define RemVSprite(vSprite) \
	LP1NR(0x8a, RemVSprite, struct VSprite *, vSprite, a0, \
	, GRAPHICS_BASE_NAME)

#define ScalerDiv(factor, numerator, denominator) \
	LP3(0x2ac, UWORD, ScalerDiv, unsigned long, factor, d0, unsigned long, numerator, d1, unsigned long, denominator, d2, \
	, GRAPHICS_BASE_NAME)

#define ScrollRaster(rp, dx, dy, xMin, yMin, xMax, yMax) \
	LP7NR(0x18c, ScrollRaster, struct RastPort *, rp, a1, long, dx, d0, long, dy, d1, long, xMin, d2, long, yMin, d3, long, xMax, d4, long, yMax, d5, \
	, GRAPHICS_BASE_NAME)

#define ScrollRasterBF(rp, dx, dy, xMin, yMin, xMax, yMax) \
	LP7NR(0x3ea, ScrollRasterBF, struct RastPort *, rp, a1, long, dx, d0, long, dy, d1, long, xMin, d2, long, yMin, d3, long, xMax, d4, long, yMax, d5, \
	, GRAPHICS_BASE_NAME)

#define ScrollVPort(vp) \
	LP1NR(0x24c, ScrollVPort, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define SetABPenDrMd(rp, apen, bpen, drawmode) \
	LP4NR(0x37e, SetABPenDrMd, struct RastPort *, rp, a1, unsigned long, apen, d0, unsigned long, bpen, d1, unsigned long, drawmode, d2, \
	, GRAPHICS_BASE_NAME)

#define SetAPen(rp, pen) \
	LP2NR(0x156, SetAPen, struct RastPort *, rp, a1, unsigned long, pen, d0, \
	, GRAPHICS_BASE_NAME)

#define SetBPen(rp, pen) \
	LP2NR(0x15c, SetBPen, struct RastPort *, rp, a1, unsigned long, pen, d0, \
	, GRAPHICS_BASE_NAME)

#define SetChipRev(want) \
	LP1(0x378, ULONG, SetChipRev, unsigned long, want, d0, \
	, GRAPHICS_BASE_NAME)

#define SetCollision(num, routine, gelsInfo) \
	LP3NRFP(0x90, SetCollision, unsigned long, num, d0, __fpt, routine, a0, struct GelsInfo *, gelsInfo, a1, \
	, GRAPHICS_BASE_NAME, void (*__fpt)(struct VSprite *vSprite, APTR))

#define SetDrMd(rp, drawMode) \
	LP2NR(0x162, SetDrMd, struct RastPort *, rp, a1, unsigned long, drawMode, d0, \
	, GRAPHICS_BASE_NAME)

#define SetFont(rp, textFont) \
	LP2(0x42, LONG, SetFont, struct RastPort *, rp, a1, struct TextFont *, textFont, a0, \
	, GRAPHICS_BASE_NAME)

#define SetMaxPen(rp, maxpen) \
	LP2NR(0x3de, SetMaxPen, struct RastPort *, rp, a0, unsigned long, maxpen, d0, \
	, GRAPHICS_BASE_NAME)

#define SetOutlinePen(rp, pen) \
	LP2(0x3d2, ULONG, SetOutlinePen, struct RastPort *, rp, a0, unsigned long, pen, d0, \
	, GRAPHICS_BASE_NAME)

#define SetRGB32(vp, n, r, g, b) \
	LP5NR(0x354, SetRGB32, struct ViewPort *, vp, a0, unsigned long, n, d0, unsigned long, r, d1, unsigned long, g, d2, unsigned long, b, d3, \
	, GRAPHICS_BASE_NAME)

#define SetRGB32CM(cm, n, r, g, b) \
	LP5NR(0x3e4, SetRGB32CM, struct ColorMap *, cm, a0, unsigned long, n, d0, unsigned long, r, d1, unsigned long, g, d2, unsigned long, b, d3, \
	, GRAPHICS_BASE_NAME)

#define SetRGB4(vp, index, red, green, blue) \
	LP5NR(0x120, SetRGB4, struct ViewPort *, vp, a0, long, index, d0, unsigned long, red, d1, unsigned long, green, d2, unsigned long, blue, d3, \
	, GRAPHICS_BASE_NAME)

#define SetRGB4CM(colorMap, index, red, green, blue) \
	LP5NR(0x276, SetRGB4CM, struct ColorMap *, colorMap, a0, long, index, d0, unsigned long, red, d1, unsigned long, green, d2, unsigned long, blue, d3, \
	, GRAPHICS_BASE_NAME)

#define SetRPAttrsA(rp, tags) \
	LP2NR(0x40e, SetRPAttrsA, struct RastPort *, rp, a0, struct TagItem *, tags, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetRPAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetRPAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define SetRast(rp, pen) \
	LP2NR(0xea, SetRast, struct RastPort *, rp, a1, unsigned long, pen, d0, \
	, GRAPHICS_BASE_NAME)

#define SetSoftStyle(rp, style, enable) \
	LP3(0x5a, ULONG, SetSoftStyle, struct RastPort *, rp, a1, unsigned long, style, d0, unsigned long, enable, d1, \
	, GRAPHICS_BASE_NAME)

#define SetWriteMask(rp, msk) \
	LP2(0x3d8, ULONG, SetWriteMask, struct RastPort *, rp, a0, unsigned long, msk, d0, \
	, GRAPHICS_BASE_NAME)

#define SortGList(rp) \
	LP1NR(0x96, SortGList, struct RastPort *, rp, a1, \
	, GRAPHICS_BASE_NAME)

#define StripFont(font) \
	LP1NR(0x336, StripFont, struct TextFont *, font, a0, \
	, GRAPHICS_BASE_NAME)

#define SyncSBitMap(layer) \
	LP1NR(0x1bc, SyncSBitMap, struct Layer *, layer, a0, \
	, GRAPHICS_BASE_NAME)

#define Text(rp, string, count) \
	LP3(0x3c, LONG, Text, struct RastPort *, rp, a1, STRPTR, string, a0, unsigned long, count, d0, \
	, GRAPHICS_BASE_NAME)

#define TextExtent(rp, string, count, textExtent) \
	LP4(0x2b2, WORD, TextExtent, struct RastPort *, rp, a1, STRPTR, string, a0, long, count, d0, struct TextExtent *, textExtent, a2, \
	, GRAPHICS_BASE_NAME)

#define TextFit(rp, string, strLen, textExtent, constrainingExtent, strDirection, constrainingBitWidth, constrainingBitHeight) \
	LP8(0x2b8, ULONG, TextFit, struct RastPort *, rp, a1, STRPTR, string, a0, unsigned long, strLen, d0, struct TextExtent *, textExtent, a2, struct TextExtent *, constrainingExtent, a3, long, strDirection, d1, unsigned long, constrainingBitWidth, d2, unsigned long, constrainingBitHeight, d3, \
	, GRAPHICS_BASE_NAME)

#define TextLength(rp, string, count) \
	LP3(0x36, WORD, TextLength, struct RastPort *, rp, a1, STRPTR, string, a0, unsigned long, count, d0, \
	, GRAPHICS_BASE_NAME)

#define UCopperListInit(uCopList, n) \
	LP2(0x252, struct CopList *, UCopperListInit, struct UCopList *, uCopList, a0, long, n, d0, \
	, GRAPHICS_BASE_NAME)

#define UnlockLayerRom(layer) \
	LP1NRA5(0x1b6, UnlockLayerRom, struct Layer *, layer, d7, \
	, GRAPHICS_BASE_NAME)

#define VBeamPos() \
	LP0(0x180, LONG, VBeamPos, \
	, GRAPHICS_BASE_NAME)

#define VideoControl(colorMap, tagarray) \
	LP2(0x2c4, BOOL, VideoControl, struct ColorMap *, colorMap, a0, struct TagItem *, tagarray, a1, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define VideoControlTags(a0, tags...) \
	({ULONG _tags[] = { tags }; VideoControl((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define WaitBOVP(vp) \
	LP1NR(0x192, WaitBOVP, struct ViewPort *, vp, a0, \
	, GRAPHICS_BASE_NAME)

#define WaitBlit() \
	LP0NR(0xe4, WaitBlit, \
	, GRAPHICS_BASE_NAME)

#define WaitTOF() \
	LP0NR(0x10e, WaitTOF, \
	, GRAPHICS_BASE_NAME)

#define WeighTAMatch(reqTextAttr, targetTextAttr, targetTags) \
	LP3(0x324, WORD, WeighTAMatch, struct TextAttr *, reqTextAttr, a0, struct TextAttr *, targetTextAttr, a1, struct TagItem *, targetTags, a2, \
	, GRAPHICS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define WeighTAMatchTags(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; WeighTAMatch((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define WriteChunkyPixels(rp, xstart, ystart, xstop, ystop, array, bytesperrow) \
	LP7NR(0x420, WriteChunkyPixels, struct RastPort *, rp, a0, unsigned long, xstart, d0, unsigned long, ystart, d1, unsigned long, xstop, d2, unsigned long, ystop, d3, UBYTE *, array, a2, long, bytesperrow, d4, \
	, GRAPHICS_BASE_NAME)

#define WritePixel(rp, x, y) \
	LP3(0x144, LONG, WritePixel, struct RastPort *, rp, a1, long, x, d0, long, y, d1, \
	, GRAPHICS_BASE_NAME)

#define WritePixelArray8(rp, xstart, ystart, xstop, ystop, array, temprp) \
	LP7(0x312, LONG, WritePixelArray8, struct RastPort *, rp, a0, unsigned long, xstart, d0, unsigned long, ystart, d1, unsigned long, xstop, d2, unsigned long, ystop, d3, UBYTE *, array, a2, struct RastPort *, temprp, a1, \
	, GRAPHICS_BASE_NAME)

#define WritePixelLine8(rp, xstart, ystart, width, array, tempRP) \
	LP6(0x306, LONG, WritePixelLine8, struct RastPort *, rp, a0, unsigned long, xstart, d0, unsigned long, ystart, d1, unsigned long, width, d2, UBYTE *, array, a2, struct RastPort *, tempRP, a1, \
	, GRAPHICS_BASE_NAME)

#define XorRectRegion(region, rectangle) \
	LP2(0x22e, BOOL, XorRectRegion, struct Region *, region, a0, struct Rectangle *, rectangle, a1, \
	, GRAPHICS_BASE_NAME)

#define XorRegionRegion(srcRegion, destRegion) \
	LP2(0x26a, BOOL, XorRegionRegion, struct Region *, srcRegion, a0, struct Region *, destRegion, a1, \
	, GRAPHICS_BASE_NAME)

#endif /* _INLINE_GRAPHICS_H */
