#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: View structures
    Lang: english
*/

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_COPPER_H
#   include <graphics/copper.h>
#endif
#ifndef GRAPHICS_DISPLAYINFO_H
#   include <graphics/displayinfo.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_GFXNODE_H
#   include <graphics/gfxnodes.h>
#endif
#ifndef GRAPHICS_MONITOR_H
#   include <graphics/monitor.h>
#endif
#ifndef HARDWARE_CUSTOM_H
#   include <hardware/custom.h>
#endif

/*          *** View ***
 *
 * Describes a physical display. Holds actual copperlists for Amiga(tm) chipset
 * and some metainformation.
 *
 * Since AROS is going to work with multiple physical displays, meaning of this
 * structure is downgraded to a simple list of ViewPorts to display (there's only
 * one View despite there can be sevaral monitors and different ViewPorts may be shown
 * on different displays)
 */

struct View
{
    struct ViewPort * ViewPort;	  /* Pointer to a first ViewPort */
    struct cprlist  * LOFCprList; /* Actual display copperlists (only for Amiga chipset) */
    struct cprlist  * SHFCprList;

    WORD DyOffset;
    WORD DxOffset;

    UWORD Modes;		  /* See below */
};

/*          *** ViewExtra ***
 *
 * Additional data for Amiga(tm) chipset. Not used by other hardware.
 */

struct ViewExtra
{
    struct ExtendedNode n;	  /* Common header */

    struct View        * View;    /* View it relates to */
    struct MonitorSpec * Monitor; /* Monitor used for displaying this View */
    UWORD                TopLine;
};

/*          *** ViewPort ***
 *
 * Describes a displayed bitmap (or logical screen).
 *
 * Copperlists are relevant only to Amiga(tm) chipset, for other hardware they are NULL.
 *
 */

struct ViewPort
{
    struct ViewPort * Next;	 /* Pointer to a next ViewPort in the view (NULL for the last ViewPort) */

    struct ColorMap * ColorMap;  /* Points to a ColorMap */
    struct CopList  * DspIns;	 /* Preliminary partial display copperlist */
    struct CopList  * SprIns;    /* Preliminary partial sprite copperlist */
    struct CopList  * ClrIns;
    struct UCopList * UCopIns;   /* User-defined part of the copperlist */

    WORD  DWidth;		 /* Width of currently displayed part in pixels */
    WORD  DHeight;		 /* Height of currently displayed part in pixels */
    WORD  DxOffset;		 /* Displacement from the (0, 0) of the physical screen to (0, 0) of the raster */
    WORD  DyOffset;
    UWORD Modes;		 /* The same as in View */

    UBYTE SpritePriorities;
    UBYTE ExtendedModes;

    struct RasInfo * RasInfo;	/* Playfield specification */
};

/*          *** ViewPortExtra ***
 *
 * Holds additional information about the ViewPort it is associated with
 */

struct ViewPortExtra
{
    struct ExtendedNode n;	    /* Common header */

    struct ViewPort  * ViewPort;    /* ViewPort it relates to */
    struct Rectangle   DisplayClip; /* Total size of displayable part */

    APTR  VecTable;		    /* Unused by AROS */
    APTR  DriverData[2];	    /* Private storage for display drivers. Do not touch! */
    UWORD Flags;		    /* Flags, see below */
    Point Origin[2];
    ULONG cop1ptr;
    ULONG cop2ptr;
};

/*          *** ColorMap ***
 *
 * This structure is the primary storage for palette data.
 * 
 * Color data itself is stored in two tables: ColorTable and LowColorBits.
 * These fields are actually pointer to arrays of UWORDs. Each UWORD corresponds
 * to one color.
 * Number of UWORDs in these arrays is equal to Count value in this structure.
 * ColorTable stores upper nibbles of RGB values, LowColorBits stores low nibbles.
 *
 * Example:
 *  color number 4, value: 0x00ABCDEF
 *  ColorTable  [4] = 0x0ACE,
 *  LowColorBits[4] = 0x0BDF
 *
 * SpriteBase fields keep bank number, not a color number. On m68k Amiga colors are divided into
 * banks, 16 per each. So bank number is color number divided by 16. Base color is a number which
 * is added to all colors of the sprite in order to look up the actual palette entry.
 * AROS may run on different hardware where sprites may have base colors that do not divide by 16.
 * In order to cover this bank numbers have a form: ((c & 0x0F) << 8 ) | (c >> 4), where c is actual
 * color number (i. e. remainder is stored in a high byte of UWORD).
 * 
 */

struct ColorMap
{
    UBYTE Flags;      				/* see below */
    UBYTE Type;       				/* Colormap type (reflects version), see below */
    UWORD Count;				/* Number of palette entries */
    UWORD *ColorTable;				/* Table of high nibbles of color values (see description above) */

    /* The following fields are present only if Type >= COLORMAP_TYPE_V36 */

    struct ViewPortExtra * cm_vpe;		/* ViewPortExtra, for faster access */

    UWORD *LowColorBits;			/* Table of low nibbles of color values (see above) */
    UBYTE TransparencyPlane;
    UBYTE SpriteResolution;			/* see below */
    UBYTE SpriteResDefault;
    UBYTE AuxFlags;

    struct ViewPort * cm_vp;			/* Points back to a ViewPort this colormap belongs to */

    APTR NormalDisplayInfo;
    APTR CoerceDisplayInfo;

    struct TagItem      * cm_batch_items;
    ULONG                 VPModeID;

    /* The following fields are present only if Type >= COLORMAP_TYPE_V39 */
    
    struct PaletteExtra * PalExtra;		/* Structure controlling palette sharing */

    UWORD SpriteBase_Even;			/* Color bank for even sprites (see above) */
    UWORD SpriteBase_Odd;			/* The same for odd sprites		   */
    UWORD Bp_0_base;
    UWORD Bp_1_base;
};

/* Flags */
#define CMF_CMTRANS                 0
#define COLORMAP_TRANSPARENCY   (1<<0)
#define CMF_CPTRANS                 1
#define COLORPLANE_TRANSPARENCY (1<<1)
#define CMF_BRDRBLNK                2
#define BORDER_BLANKING         (1<<2)
#define CMF_BRDNTRAN                3
#define BORDER_NOTRANSPARENCY   (1<<3)
#define VIDEOCONTROL_BATCH      (1<<4)
#define USER_COPPER_CLIP        (1<<5)
#define CMF_BRDRSPRT                6
#define BORDERSPRITES           (1<<6)

/* Type */
#define COLORMAP_TYPE_V1_2 0
#define COLORMAP_TYPE_V36  1
#define COLORMAP_TYPE_V39  2

/* SpriteResolution */
#define SPRITERESN_ECS     0x00
#define SPRITERESN_140NS   0x01
#define SPRITERESN_70NS    0x02
#define SPRITERESN_35NS    0x03
#define SPRITERESN_DEFAULT 0xFF

/*          *** RasInfo ***
 *
 * Describes playfield(s) (actually bitmaps)
 */

struct RasInfo
{
    struct RasInfo * Next;   /* Pointer to a next playfield (if there's more than one) */
    struct BitMap  * BitMap; /* Actual data to display */

    WORD RxOffset;	     /* Offset of the playfield relative to ViewPort */
    WORD RyOffset;	     /* (So that different playfields may be shifted against each other) */
};

/* Modes for ViewPort and View */
#define GENLOCK_VIDEO   (1<<1)
#define LACE            (1<<2)
#define DOUBLESCAN      (1<<3)
#define SUPERHIRES      (1<<5)
#define PFBA            (1<<6)
#define EXTRA_HALFBRITE (1<<7)
#define GENLOCK_AUDIO   (1<<8)
#define DUALPF          (1<<10)
#define HAM             (1<<11)
#define EXTENDED_MODE   (1<<12)
#define VP_HIDE         (1<<13)
#define SPRITES         (1<<14)
#define HIRES           (1<<15)

/* ViewPortExtra Flags */
#define VPXB_FREE_ME          0  /* Temporary ViewPortExtra allocated during MakeVPort(). ViewPortExtra with this flag */
#define VPXF_FREE_ME      (1<<0) /* will be automatically found and disposed during FreeVPortCopLists(). Private flag in fact, don't set it by hands */
#define VPXB_LAST             1
#define VPXF_LAST         (1<<1)
#define VPXB_STRADDLES256     4
#define VPXF_STRADDLES256 (1<<4)
#define VPXB_STRADDLES512     5
#define VPXF_STRADDLES512 (1<<5)

/* PRIVATE */
#define VPB_TENHZ     4
#define VPF_TENHZ (1<<4)
#define VPB_A2024     6
#define VPF_A2024 (1<<6)

#define EXTEND_VSTRUCT 0x1000

#define CMAB_FULLPALETTE 	0
#define CMAF_FULLPALETTE 	(1<<CMAB_FULLPALETTE)
#define CMAB_NO_INTERMED_UPDATE 1
#define CMAF_NO_INTERMED_UPDATE (1<<CMAB_NO_INTERMED_UPDATE)
#define CMAB_NO_COLOR_LOAD 	2
#define CMAF_NO_COLOR_LOAD 	(1 << CMAB_NO_COLOR_LOAD)
#define CMAB_DUALPF_DISABLE 	3
#define CMAF_DUALPF_DISABLE 	(1 << CMAB_DUALPF_DISABLE)


struct PaletteExtra
{
	struct SignalSemaphore pe_Semaphore;
	UWORD	pe_FirstFree;
	UWORD	pe_NFree;
	UWORD	pe_FirstShared;
	UWORD	pe_NShared;
	UBYTE	*pe_RefCnt;
	UBYTE	*pe_AllocList;
	struct ViewPort *pe_ViewPort;
	UWORD	pe_SharableColors;
};

#define PENF_EXCLUSIVE (1l<<PENB_EXCLUSIVE)
#define PENF_NO_SETCOLOR (1l<<PENB_NO_SETCOLOR)
#define PENB_EXCLUSIVE 0
#define PENB_NO_SETCOLOR 1


#define PEN_EXCLUSIVE PENF_EXCLUSIVE
#define PEN_NO_SETCOLOR PENF_NO_SETCOLOR

#define PRECISION_EXACT	-1
#define PRECISION_IMAGE	0
#define PRECISION_ICON	16
#define PRECISION_GUI	32


#define OBP_Precision 0x84000000
#define OBP_FailIfBad 0x84000001

#define MVP_OK		0
#define MVP_NO_MEM	1
#define MVP_NO_VPE	2
#define MVP_NO_DSPINS	3	
#define MVP_NO_DISPLAY	4	
#define MVP_OFF_BOTTOM	5	

#define MCOP_OK		0	
#define MCOP_NO_MEM	1
#define MCOP_NOP	2

struct DBufInfo {
	APTR	dbi_Link1;
	ULONG	dbi_Count1;
	struct Message dbi_SafeMessage;	
	APTR dbi_UserData1;

	APTR	dbi_Link2;
	ULONG	dbi_Count2;
	struct Message dbi_DispMessage;	
	APTR	dbi_UserData2;	
	ULONG	dbi_MatchLong;
	APTR	dbi_CopPtr1;
	APTR	dbi_CopPtr2;
	APTR	dbi_CopPtr3;
	UWORD	dbi_BeamPos1;
	UWORD	dbi_BeamPos2;
};

#endif /* GRAPHICS_VIEW_H */
