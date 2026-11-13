#ifndef _VBCCINLINE_PICASSO96API_H
#define _VBCCINLINE_PICASSO96API_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct BitMap * __p96AllocBitMap(__reg("a6") struct Library *, __reg("d0") ULONG SizeX, __reg("d1") ULONG SizeY, __reg("d2") ULONG Depth, __reg("d3") ULONG Flags, __reg("a0") struct BitMap * FriendBitMap, __reg("d7") RGBFTYPE RGBFormat)="\tjsr\t-30(a6)";
#define p96AllocBitMap(SizeX, SizeY, Depth, Flags, FriendBitMap, RGBFormat) __p96AllocBitMap(P96Base, (SizeX), (SizeY), (Depth), (Flags), (FriendBitMap), (RGBFormat))

void __p96FreeBitMap(__reg("a6") struct Library *, __reg("a0") struct BitMap * BitMap)="\tjsr\t-36(a6)";
#define p96FreeBitMap(BitMap) __p96FreeBitMap(P96Base, (BitMap))

ULONG __p96GetBitMapAttr(__reg("a6") struct Library *, __reg("a0") struct BitMap * BitMap, __reg("d0") ULONG Attribute)="\tjsr\t-42(a6)";
#define p96GetBitMapAttr(BitMap, Attribute) __p96GetBitMapAttr(P96Base, (BitMap), (Attribute))

LONG __p96LockBitMap(__reg("a6") struct Library *, __reg("a0") struct BitMap * BitMap, __reg("a1") UBYTE * Buffer, __reg("d0") ULONG Size)="\tjsr\t-48(a6)";
#define p96LockBitMap(BitMap, Buffer, Size) __p96LockBitMap(P96Base, (BitMap), (Buffer), (Size))

void __p96UnlockBitMap(__reg("a6") struct Library *, __reg("a0") struct BitMap * BitMap, __reg("d0") LONG Lock)="\tjsr\t-54(a6)";
#define p96UnlockBitMap(BitMap, Lock) __p96UnlockBitMap(P96Base, (BitMap), (Lock))

ULONG __p96BestModeIDTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-60(a6)";
#define p96BestModeIDTagList(Tags) __p96BestModeIDTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __p96BestModeIDTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-60(a6)\n\tmovea.l\t(a7)+,a0";
#define p96BestModeIDTags(...) __p96BestModeIDTags(P96Base, __VA_ARGS__)
#endif

ULONG __p96RequestModeIDTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-66(a6)";
#define p96RequestModeIDTagList(Tags) __p96RequestModeIDTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __p96RequestModeIDTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-66(a6)\n\tmovea.l\t(a7)+,a0";
#define p96RequestModeIDTags(...) __p96RequestModeIDTags(P96Base, __VA_ARGS__)
#endif

struct List * __p96AllocModeListTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-72(a6)";
#define p96AllocModeListTagList(Tags) __p96AllocModeListTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct List * __p96AllocModeListTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-72(a6)\n\tmovea.l\t(a7)+,a0";
#define p96AllocModeListTags(...) __p96AllocModeListTags(P96Base, __VA_ARGS__)
#endif

void __p96FreeModeList(__reg("a6") struct Library *, __reg("a0") struct List * List)="\tjsr\t-78(a6)";
#define p96FreeModeList(List) __p96FreeModeList(P96Base, (List))

ULONG __p96GetModeIDAttr(__reg("a6") struct Library *, __reg("d0") ULONG Mode, __reg("d1") ULONG Attribute)="\tjsr\t-84(a6)";
#define p96GetModeIDAttr(Mode, Attribute) __p96GetModeIDAttr(P96Base, (Mode), (Attribute))

struct Screen * __p96OpenScreenTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-90(a6)";
#define p96OpenScreenTagList(Tags) __p96OpenScreenTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct Screen * __p96OpenScreenTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-90(a6)\n\tmovea.l\t(a7)+,a0";
#define p96OpenScreenTags(...) __p96OpenScreenTags(P96Base, __VA_ARGS__)
#endif

BOOL __p96CloseScreen(__reg("a6") struct Library *, __reg("a0") struct Screen * Screen)="\tjsr\t-96(a6)";
#define p96CloseScreen(Screen) __p96CloseScreen(P96Base, (Screen))

void __p96WritePixelArray(__reg("a6") struct Library *, __reg("a0") struct RenderInfo * ri, __reg("d0") UWORD SrcX, __reg("d1") UWORD SrcY, __reg("a1") struct RastPort * rp, __reg("d2") UWORD DestX, __reg("d3") UWORD DestY, __reg("d4") UWORD SizeX, __reg("d5") UWORD SizeY)="\tjsr\t-102(a6)";
#define p96WritePixelArray(ri, SrcX, SrcY, rp, DestX, DestY, SizeX, SizeY) __p96WritePixelArray(P96Base, (ri), (SrcX), (SrcY), (rp), (DestX), (DestY), (SizeX), (SizeY))

void __p96ReadPixelArray(__reg("a6") struct Library *, __reg("a0") struct RenderInfo * ri, __reg("d0") UWORD DestX, __reg("d1") UWORD DestY, __reg("a1") struct RastPort * rp, __reg("d2") UWORD SrcX, __reg("d3") UWORD SrcY, __reg("d4") UWORD SizeX, __reg("d5") UWORD SizeY)="\tjsr\t-108(a6)";
#define p96ReadPixelArray(ri, DestX, DestY, rp, SrcX, SrcY, SizeX, SizeY) __p96ReadPixelArray(P96Base, (ri), (DestX), (DestY), (rp), (SrcX), (SrcY), (SizeX), (SizeY))

ULONG __p96WritePixel(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("d0") UWORD x, __reg("d1") UWORD y, __reg("d2") ULONG color)="\tjsr\t-114(a6)";
#define p96WritePixel(rp, x, y, color) __p96WritePixel(P96Base, (rp), (x), (y), (color))

ULONG __p96ReadPixel(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("d0") UWORD x, __reg("d1") UWORD y)="\tjsr\t-120(a6)";
#define p96ReadPixel(rp, x, y) __p96ReadPixel(P96Base, (rp), (x), (y))

void __p96RectFill(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("d0") UWORD MinX, __reg("d1") UWORD MinY, __reg("d2") UWORD MaxX, __reg("d3") UWORD MaxY, __reg("d4") ULONG color)="\tjsr\t-126(a6)";
#define p96RectFill(rp, MinX, MinY, MaxX, MaxY, color) __p96RectFill(P96Base, (rp), (MinX), (MinY), (MaxX), (MaxY), (color))

void __p96WriteTrueColorData(__reg("a6") struct Library *, __reg("a0") struct TrueColorInfo * tci, __reg("d0") UWORD SrcX, __reg("d1") UWORD SrcY, __reg("a1") struct RastPort * rp, __reg("d2") UWORD DestX, __reg("d3") UWORD DestY, __reg("d4") UWORD SizeX, __reg("d5") UWORD SizeY)="\tjsr\t-132(a6)";
#define p96WriteTrueColorData(tci, SrcX, SrcY, rp, DestX, DestY, SizeX, SizeY) __p96WriteTrueColorData(P96Base, (tci), (SrcX), (SrcY), (rp), (DestX), (DestY), (SizeX), (SizeY))

void __p96ReadTrueColorData(__reg("a6") struct Library *, __reg("a0") struct TrueColorInfo * tci, __reg("d0") UWORD DestX, __reg("d1") UWORD DestY, __reg("a1") struct RastPort * rp, __reg("d2") UWORD SrcX, __reg("d3") UWORD SrcY, __reg("d4") UWORD SizeX, __reg("d5") UWORD SizeY)="\tjsr\t-138(a6)";
#define p96ReadTrueColorData(tci, DestX, DestY, rp, SrcX, SrcY, SizeX, SizeY) __p96ReadTrueColorData(P96Base, (tci), (DestX), (DestY), (rp), (SrcX), (SrcY), (SizeX), (SizeY))

struct Window * __p96PIP_OpenTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-144(a6)";
#define p96PIP_OpenTagList(Tags) __p96PIP_OpenTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct Window * __p96PIP_OpenTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-144(a6)\n\tmovea.l\t(a7)+,a0";
#define p96PIP_OpenTags(...) __p96PIP_OpenTags(P96Base, __VA_ARGS__)
#endif

BOOL __p96PIP_Close(__reg("a6") struct Library *, __reg("a0") struct Window * Window)="\tjsr\t-150(a6)";
#define p96PIP_Close(Window) __p96PIP_Close(P96Base, (Window))

LONG __p96PIP_SetTagList(__reg("a6") struct Library *, __reg("a0") struct Window * Window, __reg("a1") struct TagItem * Tags)="\tjsr\t-156(a6)";
#define p96PIP_SetTagList(Window, Tags) __p96PIP_SetTagList(P96Base, (Window), (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __p96PIP_SetTags(__reg("a6") struct Library *, __reg("a0") struct Window * Window, ULONG Tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-156(a6)\n\tmovea.l\t(a7)+,a1";
#define p96PIP_SetTags(Window, ...) __p96PIP_SetTags(P96Base, (Window), __VA_ARGS__)
#endif

LONG __p96PIP_GetTagList(__reg("a6") struct Library *, __reg("a0") struct Window * Window, __reg("a1") struct TagItem * Tags)="\tjsr\t-162(a6)";
#define p96PIP_GetTagList(Window, Tags) __p96PIP_GetTagList(P96Base, (Window), (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __p96PIP_GetTags(__reg("a6") struct Library *, __reg("a0") struct Window * Window, ULONG Tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-162(a6)\n\tmovea.l\t(a7)+,a1";
#define p96PIP_GetTags(Window, ...) __p96PIP_GetTags(P96Base, (Window), __VA_ARGS__)
#endif

struct IntuiMessage * __p96PIP_GetIMsg(__reg("a6") struct Library *, __reg("a0") struct MsgPort * Port)="\tjsr\t-168(a6)";
#define p96PIP_GetIMsg(Port) __p96PIP_GetIMsg(P96Base, (Port))

void __p96PIP_ReplyIMsg(__reg("a6") struct Library *, __reg("a1") struct IntuiMessage * IntuiMessage)="\tjsr\t-174(a6)";
#define p96PIP_ReplyIMsg(IntuiMessage) __p96PIP_ReplyIMsg(P96Base, (IntuiMessage))

LONG __p96GetRTGDataTagList(__reg("a6") struct Library *, __reg("a0") struct TagItem * Tags)="\tjsr\t-180(a6)";
#define p96GetRTGDataTagList(Tags) __p96GetRTGDataTagList(P96Base, (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __p96GetRTGDataTags(__reg("a6") struct Library *, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-180(a6)\n\tmovea.l\t(a7)+,a0";
#define p96GetRTGDataTags(...) __p96GetRTGDataTags(P96Base, __VA_ARGS__)
#endif

LONG __p96GetBoardDataTagList(__reg("a6") struct Library *, __reg("d0") ULONG Board, __reg("a0") struct TagItem * Tags)="\tjsr\t-186(a6)";
#define p96GetBoardDataTagList(Board, Tags) __p96GetBoardDataTagList(P96Base, (Board), (Tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __p96GetBoardDataTags(__reg("a6") struct Library *, __reg("d0") ULONG Board, ULONG Tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-186(a6)\n\tmovea.l\t(a7)+,a0";
#define p96GetBoardDataTags(Board, ...) __p96GetBoardDataTags(P96Base, (Board), __VA_ARGS__)
#endif

ULONG __p96EncodeColor(__reg("a6") struct Library *, __reg("d0") RGBFTYPE RGBFormat, __reg("d1") ULONG Color)="\tjsr\t-192(a6)";
#define p96EncodeColor(RGBFormat, Color) __p96EncodeColor(P96Base, (RGBFormat), (Color))

#endif /*  _VBCCINLINE_PICASSO96API_H  */
