#ifndef GRAPHICS_GFXBASE_H
#define GRAPHICS_GFXBASE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: graphics.library
    Lang: english
*/

#ifndef EXEC_INTERRUPTS_H
#   include <exec/interrupts.h>
#endif

#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef GRAPHICS_MONITOR_H
#   include <graphics/monitor.h>
#endif

#define GRAPHICSNAME "graphics.library"

struct GfxBase
{
/* Standard Library Node */
    struct Library LibNode;

    struct View    * ActiView;
    struct copinit * copinit;
    LONG           * cia;
    LONG           * blitter;
    UWORD          * LOFlist;
    UWORD          * SHFlist;
    struct bltnode * blthd;
    struct bltnode * blttl;
    struct bltnode * bsblthd;
    struct bltnode * bsblttl;
    struct Interrupt vbsrv;
    struct Interrupt timsrv;
    struct Interrupt bltsrv;

/* Fonts */
    struct List       TextFonts;
    struct TextFont * DefaultFont;

    UWORD Modes;
    BYTE  VBlank;
    BYTE  Debug;
    WORD  BeamSync;
    WORD  system_bplcon0 ;
    UBYTE SpriteReserved;
    UBYTE bytereserved;
    UWORD Flags;
    WORD  BlitLock;
    WORD  BlitNest;

    struct List   BlitWaitQ;
    struct Task * BlitOwner;
    struct List   TOF_WaitQ;

    UWORD                  DisplayFlags;  /* see below */
    struct SimpleSprite ** SimpleSprites;

    UWORD MaxDisplayRow;
    UWORD MaxDisplayColumn;
    UWORD NormalDisplayRows;
    UWORD NormalDisplayColumns;
    UWORD NormalDPMX;
    UWORD NormalDPMY;

    struct SignalSemaphore * LastChanceMemory;

    UWORD * LCMptr;
    UWORD   MicrosPerLine;
    UWORD   MinDisplayColumn;
    UBYTE   ChipRevBits0;     /* see below */
    UBYTE   MemType;
    UBYTE   crb_reserved[4];
    UWORD   monitor_id;

    ULONG hedley[8];
    ULONG hedley_sprites[8];
    ULONG hedley_sprites1[8];
    WORD  hedley_count;
    UWORD hedley_flags;
    WORD  hedley_tmp;

    LONG * hash_table;
    UWORD  current_tot_rows;
    UWORD  current_tot_cclks;
    UBYTE  hedley_hint;
    UBYTE  hedley_hint2;
    ULONG  nreserved[4];
    LONG * a2024_sync_raster;
    UWORD  control_delta_pal;
    UWORD  control_delta_ntsc;

    struct MonitorSpec     * current_monitor;
    struct List              MonitorList;
    struct MonitorSpec     * default_monitor;
    struct SignalSemaphore * MonitorListSemaphore;

    VOID                   * DisplayInfoDataBase;
    UWORD                    TopLine;
    struct SignalSemaphore * ActiViewCprSemaphore;

/* Library Bases */
    struct Library  *UtilBase;
    struct ExecBase *ExecBase;

    BYTE  * bwshifts;
    UWORD * StrtFetchMasks;
    UWORD * StopFetchMasks;
    UWORD * Overrun;
    WORD  * RealStops;
    UWORD   SpriteWidth;
    UWORD   SpriteFMode;
    BYTE    SoftSprites;
    BYTE    arraywidth;
    UWORD   DefaultSpriteWidth;
    BYTE    SprMoveDisable;
    UBYTE   WantChips;
    UBYTE   BoardMemType;
    UBYTE   Bugs;
    ULONG * gb_LayersBase;
    ULONG   ColorMask;
    APTR    IVector;
    APTR    IData;
    ULONG   SpecialCounter;
    APTR    DBList;
    UWORD   MonitorFlags;
    UBYTE   ScanDoubleSprites;
    UBYTE   BP3Bits;

    struct AnalogSignalInterval MonitorVBlank;
    struct MonitorSpec        * natural_monitor;

    APTR  ProgData;
    UBYTE ExtSprites;
    UBYTE pad3;
    UWORD GfxFlags;
    ULONG VBCounter;

    struct SignalSemaphore * HashTableSemaphore;
    ULONG                  * HWEmul[9];
};
#define ChunkyToPlanarPtr HWEmul[0];

/* DisplayFlags */
#define NTSC             (1<<0)
#define GENLOC           (1<<1)
#define PAL              (1<<2)
#define TODA_SAFE        (1<<3)
#define REALLY_PAL       (1<<4)
#define LPEN_SWAP_FRAMES (1<<5)

/* ChipRevBits */
#define GFXB_BIG_BLITS     0
#define GFXF_BIG_BLITS (1<<0)
#define GFXB_HR_AGNUS      0
#define GFXF_HR_AGNUS  (1<<0)
#define GFXB_HR_DENISE     1
#define GFXF_HR_DENISE (1<<1)
#define GFXB_AA_ALICE      2
#define GFXF_AA_ALICE  (1<<2)
#define GFXB_AA_LISA       3
#define GFXF_AA_LISA   (1<<3)
#define GFXB_AA_MLISA      4
#define GFXF_AA_MLISA  (1<<4)

/* For use in SetChipRev() */
#define SETCHIPREV_A    GFXF_HR_AGNUS
#define SETCHIPREV_ECS  (GFXF_HR_AGNUS | GFXF_HR_DENISE)
#define SETCHIPREV_AA   (SETCHIPREV_ECS | GFXF_AA_ALICE | GFXF_AA_LISA)
#define SETCHIPREV_BEST 0xFFFFFFFF

#define BUS_16  0
#define BUS_32  1
#define NML_CAS 0
#define DBL_CAS 2

#define BANDWIDTH_1X    (BUS_16 | NML_CAS)
#define BANDWIDTH_2XNML BUS_32
#define BANDWIDTH_2XDBL DBL_CAS
#define BANDWIDTH_4X    (BUS_32 | DBL_CAS)

#define BLITMSG_FAULT 4

/* PRIVATE */
#define NEW_DATABASE 1

#endif /* GRAPHICS_GFXBASE_H */
