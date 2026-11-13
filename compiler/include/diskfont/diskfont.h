#ifndef DISKFONT_DISKFONT_H
#define DISKFONT_DISKFONT_H

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_NODES_H
#	include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#	include <exec/lists.h>
#endif
#ifndef GRAPHICS_TEXT_H
#	include <graphics/text.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

#define	MAXFONTPATH	256

struct FontContents
{
	char	fc_FileName[MAXFONTPATH];
	UWORD	fc_YSize;
	UBYTE	fc_Style;
	UBYTE	fc_Flags;
};

struct TFontContents
{
	char	tfc_FileName[MAXFONTPATH - 2];
	UWORD	tfc_TagCount;
	UWORD	tfc_YSize;
	UBYTE	tfc_Style;
	UBYTE	tfc_Flags;
};

#define FCH_ID	0x0f00
#define TFCH_ID	0x0f02
#define OFCH_ID	0x0f03

struct FontContentsHeader
{
	UWORD	fch_FileID;
	UWORD	fch_NumEntries;
};

#define DFH_ID	0x0f80
#define MAXFONTNAME	32

struct DiskFontHeader
{
	struct Node dfh_DF;
	UWORD	dfh_FileID;
	UWORD	dfh_Revision;
	BPTR	dfh_Segment;
	char	dfh_Name[MAXFONTNAME];
	struct TextFont dfh_TF;
};

#define dfh_TagList dfh_Segment

#define AFB_MEMORY	0
#define AFF_MEMORY	0x0001
#define AFB_DISK	1
#define AFF_DISK	0x0002
#define AFB_SCALED	2
#define AFF_SCALED	0x0004
#define AFB_BITMAP	3
#define AFF_BITMAP	0x0008

#define AFB_TAGGED	16
#define AFF_TAGGED	0x10000L

struct AvailFonts
{
	UWORD			af_Type;
	struct TextAttr	af_Attr;
};

struct TAvailFonts
{
	UWORD				taf_Type;
	struct TTextAttr	taf_Attr;
};

struct AvailFontsHeader
{
	UWORD	afh_NumEntries;
};

#endif

