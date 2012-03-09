#ifndef GRAPHICS_MONITOR_H
#define GRAPHICS_MONITOR_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Monitor definitions
    Lang: english
*/

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_GFXNODES_H
#   include <graphics/gfxnodes.h>
#endif

#define DEFAULT_MONITOR_NAME "default.monitor"
#define NTSC_MONITOR_NAME    "ntsc.monitor"
#define PAL_MONITOR_NAME     "pal.monitor"
#define VGA_MONITOR_NAME     "vga.monitor"

struct MonitorSpec
{
    struct ExtendedNode ms_Node;

    UWORD ms_Flags;			/* Flags, see below */

    LONG  ratioh;			/* Fixed point fractions, see the */
    LONG  ratiov;			/* RATIO_FIXEDPART and RATIO_UNITY macros */
    UWORD total_rows;			/* Total number of scanlines per frame */
    UWORD total_colorclocks;		/* Total number of color clocks per line (in 1/280 ns units) */
    UWORD DeniseMaxDisplayColumn;
    UWORD BeamCon0;			/* Value for beamcon0 Amiga(tm) chipset register */
    UWORD min_row;

    struct SpecialMonitor * ms_Special; /* Synchro signal timings description (optional) */

    UWORD    ms_OpenCount;		/* Driver open count */
    LONG  (* ms_transform)();
    LONG  (* ms_translate)();
    LONG  (* ms_scale)();
    UWORD    ms_xoffset;
    UWORD    ms_yoffset;

    struct Rectangle ms_LegalView;	/* Allowed range for view positioning (right-bottom position included) */

    LONG  (* ms_maxoscan)();
    LONG  (* ms_videoscan)();
    UWORD    DeniseMinDisplayColumn;
    ULONG    DisplayCompatible;

    struct List            DisplayInfoDataBase;
    struct SignalSemaphore DisplayInfoDataBaseSemaphore;

    LONG (* ms_MrgCop)();		/* Driver call vectors, unused by AROS */
    LONG (* ms_LoadView)();
    LONG (* ms_KillView)();
};

/* An alias for pointer to associated sync object, AROS-specific */
#define ms_Object DisplayInfoDataBase.lh_Head

/* ms_Flags */
#define MSB_REQUEST_NTSC        0
#define MSF_REQUEST_NTSC    (1<<0)
#define MSB_REQUEST_PAL         1
#define MSF_REQUEST_PAL     (1<<1)
#define MSB_REQUEST_SPECIAL     2
#define MSF_REQUEST_SPECIAL (1<<2)
#define MSB_REQUEST_A2024       3
#define MSF_REQUEST_A2024   (1<<3)
#define MSB_DOUBLE_SPRITES      4
#define MSF_DOUBLE_SPRITES  (1<<4)

#define STANDARD_MONITOR_MASK (MSF_REQUEST_NTSC | MSF_REQUEST_PAL)

#define TO_MONITOR   0
#define FROM_MONITOR 1

#define STANDARD_XOFFSET 9
#define STANDARD_YOFFSET 0

/* Some standard/default constants for Amiga(tm) chipset */
#define STANDARD_NTSC_ROWS    262
#define MIN_NTSC_ROW          21
#define STANDARD_PAL_ROWS     312
#define MIN_PAL_ROW           29
#define STANDARD_NTSC_BEAMCON 0x0000
#define STANDARD_PAL_BEAMCON  0x0020
#define SPECIAL_BEAMCON       (VARVBLANK | VARHSYNC | VARVSYNC | VARBEAM | VSYNCTRUE | LOLDIS | CSBLANK)
#define STANDARD_DENISE_MIN   93
#define STANDARD_DENISE_MAX   455
#define STANDARD_COLORCLOCKS  226
#define STANDARD_VIEW_X       0x81
#define STANDARD_VIEW_Y       0x2C
#define STANDARD_HBSTRT       0x06
#define STANDARD_HBSTOP       0x2C
#define STANDARD_HSSTRT       0x0B
#define STANDARD_HSSTOP       0x1C
#define STANDARD_VBSTRT       0x0122
#define STANDARD_VBSTOP       0x1066
#define STANDARD_VSSTRT       0x02A6
#define STANDARD_VSSTOP       0x03AA

#define VGA_COLORCLOCKS (STANDARD_COLORCLOCKS / 2)
#define VGA_TOTAL_ROWS  (STANDARD_NTSC_ROWS * 2)
#define VGA_DENISE_MIN  59
#define MIN_VGA_ROW     29
#define VGA_HBSTRT      0x08
#define VGA_HBSTOP      0x1E
#define VGA_HSSTRT      0x0E
#define VGA_HSSTOP      0x1C
#define VGA_VBSTRT      0x0000
#define VGA_VBSTOP      0x0CCD
#define VGA_VSSTRT      0x0153
#define VGA_VSSTOP      0x0235

#define BROADCAST_BEAMCON (LOLDIS | CSBLANK)
#define BROADCAST_HBSTRT  0x01
#define BROADCAST_HBSTOP  0x27
#define BROADCAST_HSSTRT  0x06
#define BROADCAST_HSSTOP  0x17
#define BROADCAST_VBSTRT  0x0000
#define BROADCAST_VBSTOP  0x1C40
#define BROADCAST_VSSTRT  0x02A6
#define BROADCAST_VSSTOP  0x054C

#define RATIO_FIXEDPART     4
#define RATIO_UNITY     (1<<4)

struct AnalogSignalInterval
{
    UWORD asi_Start;
    UWORD asi_Stop;
};

struct SpecialMonitor
{
    struct ExtendedNode spm_Node;

    UWORD    spm_Flags;				 /* Reserved, set to 0		             */
    LONG  (* do_monitor)(struct MonitorSpec *); /* Driver call vector - set up a video mode */
    LONG  (* reserved1)();			 /* Private data, do not touch               */
    LONG  (* reserved2)();
    LONG  (* reserved3)();

    struct AnalogSignalInterval hblank;		 /* Signal timings by themselves	      */
    struct AnalogSignalInterval vblank;
    struct AnalogSignalInterval hsync;
    struct AnalogSignalInterval vsync;
};

#endif /* GRAPHICS_MONITOR_H */
