#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

struct View
{
    struct ViewPort * ViewPort;
    struct cprlist  * LOFCprList;
    struct cprlist  * SHFCprList;

    WORD DyOffset;
    WORD DxOffset;

    UWORD Modes;
};

struct ViewExtra
{
    struct ExtendedNode n;

    struct View        * View;
    struct MonitorSpec * Monitor;
    UWORD                TopLine;
};

struct ViewPort
{
    struct ViewPort * Next;

    struct ColorMap * ColorMap;
    struct CopList  * DspIns;
    struct CopList  * SprIns;
    struct CopList  * ClrIns;
    struct UCopList * UCopIns;

    WORD  DWidth;
    WORD  DHeight;
    WORD  DxOffset;
    WORD  DyOffset;
    UWORD Modes;

    UBYTE SpritePriorities;
    UBYTE ExtendedModes;

    struct RasInfo * RasInfo;
};

struct ViewPortExtra
{
    struct ExtendedNode n;

    struct ViewPort  * ViewPort;
    struct Rectangle   DisplayClip;

    APTR  VecTable;
    APTR  DriverData[2];
    UWORD Flags;
    Point Origin[2];
    ULONG cop1ptr;
    ULONG cop2ptr;
};

struct ColorMap
{
    UBYTE Flags;      /* see below */
    UBYTE Type;       /* see below */
    UWORD Count;
    APTR  ColorTable;

    struct ViewPortExtra * cm_vpe;

    APTR  LowColorBits;
    UBYTE TransparencyPlane;
    UBYTE SpriteResolution;  /* see below */
    UBYTE SpriteResDefault;
    UBYTE AuxFlags;

    struct ViewPort * cm_vp;

    APTR NormalDisplayInfo;
    APTR CoerceDisplayInfo;

    struct TagItem      * cm_batch_items;
    ULONG                 VPModeID;
    struct PaletteExtra * PalExtra;

    UWORD SpriteBase_Even;
    UWORD SpriteBase_Odd;
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

struct RasInfo
{
    struct RasInfo * Next;
    struct BitMap  * BitMap;

    WORD RxOffset;
    WORD RyOffset;
};

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

/* PRIVATE */
#define VPXB_FREE_ME          0
#define VPXF_FREE_ME      (1<<0)
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
