#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
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
#define CMF_CMTRANS                0
#define COLORMAP_TRANSPARENCY  (1<<0)
#define CMF_CPTRANS                1
#define COLORPLANE_TRANPARENCY (1<<1)
#define CMF_BRDRBLNK               2
#define BORDER_BLANKING        (1<<2)
#define CMF_BRDNTRAN               3
#define BORDER_NOTRANPARENCY   (1<<3)
#define VIDEOCONTROL_BATCH     (1<<4)
#define USER_COPPER_CLIP       (1<<5)
#define CMF_BRDRSPRT               6
#define BORDERSPRITES          (1<<6)

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


#endif /* GRAPHICS_VIEW_H */
