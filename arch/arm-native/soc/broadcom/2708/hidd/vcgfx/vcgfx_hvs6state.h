#ifndef _VIDEOCOREGFX_HVS6STATE_H
#define _VIDEOCOREGFX_HVS6STATE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HVS6 display-list ownership state.
*/

#include <exec/types.h>

#include "vcgfx_edid.h"

struct vc4_hvs6_state
{
    BOOL    h6_Active;      /* our list is the one being scanned out   */
    ULONG   h6_Chan;        /* channel showing the framebuffer         */
    ULONG   h6_ListBase;    /* list RAM base, word index in the window */
    ULONG   h6_FWList;      /* firmware's list pointer, for restore    */
    ULONG   h6_List[2];     /* our lists, word offsets from the base   */
    ULONG   h6_Pages;       /* lists authored: 1, or 2 once flipping   */
    BOOL    h6_Overlay;     /* an overlay plane is in both lists       */

    /* Back page, ours to allocate; the firmware's surface is the front */
    APTR    h6_BackRaw;
    ULONG   h6_BackSize;    /* bytes handed to AllocMem                */
    UQUAD   h6_BackPhys;
    ULONG   h6_BackMapped;  /* bytes actually remapped                 */

    /* UPM prefetch slices (PTR0); the framebuffer plane keeps the firmware's */
    ULONG   h6_OvlPTR0;
    ULONG   h6_CurPTR0;

    APTR    h6_CurRaw;
    UQUAD   h6_CurPhys;     /* physical address of vcsd_CurBuf         */
    BOOL    h6_Cursor;      /* a cursor plane is in both lists         */

    /* Last programmed entry values, so a move patches a single word */
    ULONG   h6_CurGeom;     /* w<<16 | h                               */
    ULONG   h6_CurPos;
    ULONG   h6_CurAddr;     /* PTR1                                    */
    ULONG   h6_OvlGeom;
    ULONG   h6_OvlPos;
    ULONG   h6_OvlPitch;

    /* FRCNT at the last overlay write, and whether a wait is pending */
    ULONG   h6_OvlFrame;
    BOOL    h6_OvlLatchDue;

    /* The firmware's boot mode on HDMI0, captured before anything was
     * touched (vcgfx_hvs6_mode.c). Switching back to its size restores
     * these verbatim; h6_ModeOK gates every other mode. */
    BOOL    h6_ModeOK;
    struct vcgfx_timing h6_BootTiming;
    ULONG   h6_BootPV[7];       /* pixelvalve +0x00..+0x18          */
    ULONG   h6_BootHDMI[6];     /* HORZA HORZB VERTA0 VERTB0 VERTA1 VERTB1 */
    ULONG   h6_BootVidCtl;
    ULONG   h6_BootCtrl0;       /* HVS channel 0 CTRL0              */
    ULONG   h6_BootLane[4];     /* PHY CTL_0/1/2/CK                 */
    ULONG   h6_BootVcoDiv;      /* PHY PLL_VCOCLK_DIV, divider only */
    ULONG   h6_BootRmOffset;    /* rate manager offset, 30:0        */

    /* A framebuffer of our own, for modes larger than the boot surface,
     * and the prefetch slice its plane needs. */
    ULONG   h6_FbPTR0;
    APTR    h6_FBRaw;
    ULONG   h6_FBSize;          /* bytes handed to AllocMem         */
    UQUAD   h6_FBPhys;
    ULONG   h6_FBMapped;        /* bytes actually remapped          */

    /* The mode the output runs, and whether the link is scrambled
     * (TMDS from 340 MHz up); the watch task keeps the sink's SCDC
     * set-up alive while it is. */
    struct vcgfx_timing h6_CurTiming;
    BOOL    h6_Scrambled;
    struct Task *h6_Watch;

    /* Vblank from pixelvalve 0 (vcgfx_hvs6.c). h6_VSyncMask is the INTEN
     * bit measured to run at frame rate, 0 = none; h6_VSyncTask the one
     * task sleeping on it, and h6_VSyncStamp dates the last tick so a
     * stopped interrupt is noticed instead of slept on. */
    APTR            h6_VSyncIrq;
    ULONG           h6_VSyncMask;
    volatile ULONG  h6_VSyncCount;
    volatile ULONG  h6_VSyncStamp;
    struct Task * volatile h6_VSyncTask;
    ULONG           h6_VSyncSigMask;
};

#endif /* _VIDEOCOREGFX_HVS6STATE_H */
