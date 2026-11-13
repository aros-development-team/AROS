#ifndef CLIB_PICASSO96API_PROTOS_H
#define CLIB_PICASSO96API_PROTOS_H


/*
**	$VER: Picasso96API_protos.h 1.0 (09.04.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#ifndef  LIBRARIES_PICASSO96_H
#include <libraries/Picasso96.h>
#endif

struct BitMap * p96AllocBitMap(ULONG SizeX, ULONG SizeY, ULONG Depth, ULONG Flags,
	struct BitMap * FriendBitMap, RGBFTYPE RGBFormat);
void p96FreeBitMap(struct BitMap * BitMap);
ULONG p96GetBitMapAttr(struct BitMap * BitMap, ULONG Attribute);
LONG p96LockBitMap(struct BitMap * BitMap, UBYTE * Buffer, ULONG Size);
void p96UnlockBitMap(struct BitMap * BitMap, LONG Lock);
ULONG p96BestModeIDTagList(struct TagItem * Tags);
ULONG p96BestModeIDTags(ULONG Tags, ...);
ULONG p96RequestModeIDTagList(struct TagItem * Tags);
ULONG p96RequestModeIDTags(ULONG Tags, ...);
struct List * p96AllocModeListTagList(struct TagItem * Tags);
struct List * p96AllocModeListTags(ULONG Tags, ...);
void p96FreeModeList(struct List * List);
ULONG p96GetModeIDAttr(ULONG Mode, ULONG Attribute);
struct Screen * p96OpenScreenTagList(struct TagItem * Tags);
struct Screen * p96OpenScreenTags(ULONG Tags, ...);
BOOL p96CloseScreen(struct Screen * Screen);
void p96WritePixelArray(struct RenderInfo * ri, UWORD SrcX, UWORD SrcY, struct RastPort * rp,
	UWORD DestX, UWORD DestY, UWORD SizeX, UWORD SizeY);
void p96ReadPixelArray(struct RenderInfo * ri, UWORD DestX, UWORD DestY, struct RastPort * rp,
	UWORD SrcX, UWORD SrcY, UWORD SizeX, UWORD SizeY);
ULONG p96WritePixel(struct RastPort * rp, UWORD x, UWORD y, ULONG color);
ULONG p96ReadPixel(struct RastPort * rp, UWORD x, UWORD y);
void p96RectFill(struct RastPort * rp, UWORD MinX, UWORD MinY, UWORD MaxX, UWORD MaxY,
	ULONG color);
void p96WriteTrueColorData(struct TrueColorInfo * tci, UWORD SrcX, UWORD SrcY,
	struct RastPort * rp, UWORD DestX, UWORD DestY, UWORD SizeX,
	UWORD SizeY);
void p96ReadTrueColorData(struct TrueColorInfo * tci, UWORD DestX, UWORD DestY,
	struct RastPort * rp, UWORD SrcX, UWORD SrcY, UWORD SizeX,
	UWORD SizeY);
struct Window * p96PIP_OpenTagList(struct TagItem * Tags);
struct Window * p96PIP_OpenTags(ULONG Tags, ...);
BOOL p96PIP_Close(struct Window * Window);
LONG p96PIP_SetTagList(struct Window * Window, struct TagItem * Tags);
LONG p96PIP_SetTags(struct Window * Window, ULONG Tags, ...);
LONG p96PIP_GetTagList(struct Window * Window, struct TagItem * Tags);
LONG p96PIP_GetTags(struct Window * Window, ULONG Tags, ...);
struct IntuiMessage * p96PIP_GetIMsg(struct MsgPort * Port);
void p96PIP_ReplyIMsg(struct IntuiMessage * IntuiMessage);
LONG p96GetRTGDataTagList(struct TagItem * Tags);
LONG p96GetRTGDataTags(ULONG Tags, ...);
LONG p96GetBoardDataTagList(ULONG Board, struct TagItem * Tags);
LONG p96GetBoardDataTags(ULONG Board, ULONG Tags, ...);
ULONG p96EncodeColor(RGBFTYPE RGBFormat, ULONG Color);

#endif	/*  CLIB_PICASSO96API_PROTOS_H  */
