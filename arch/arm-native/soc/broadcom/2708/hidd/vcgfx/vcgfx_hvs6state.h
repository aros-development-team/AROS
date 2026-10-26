#ifndef _VIDEOCOREGFX_HVS6STATE_H
#define _VIDEOCOREGFX_HVS6STATE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HVS6 display-list ownership state.
*/

#include <exec/types.h>
#include <oop/oop.h>

#include "vcgfx_edid.h"
#include "vcgfx_vblwait.h"

struct vc4_hvs6_state
{
    BOOL    h6_Active;      /* our list is the one being scanned out   */
    ULONG   h6_Chan;        /* channel showing the framebuffer         */
    ULONG   h6_ListBase;    /* list RAM base, word index in the window */
    ULONG   h6_FWList;      /* firmware's list pointer, for restore    */
    ULONG   h6_List;        /* our list, word offset from the base     */

    /* UPM prefetch slices (PTR0); the framebuffer plane keeps the firmware's */
    ULONG   h6_CurPTR0;

    APTR    h6_CurRaw;
    UQUAD   h6_CurPhys;     /* physical address of vcsd_CurBuf         */
    BOOL    h6_Cursor;      /* a cursor plane is in the list           */

    /* Last programmed entry values, so a move patches a single word */
    ULONG   h6_CurGeom;     /* w<<16 | h                               */
    ULONG   h6_CurPos;
    ULONG   h6_CurAddr;     /* PTR1                                    */

    /* FRCNT at the last list write, and whether a wait is pending */
    ULONG   h6_ArmFrame;
    BOOL    h6_LatchDue;

    /* Screens and overlay planes: three lists so one is always free to
     * write; UPM between h6_UpmFloor and h6_UpmTop (bytes) for them. */
    ULONG   h6_PlList[3];       /* word offsets from h6_ListBase, 0 = none */
    ULONG   h6_PlLast;          /* the one last written to DISPLIST */
    ULONG   h6_UpmTop;
    ULONG   h6_UpmFloor;
    BOOL    h6_ScaleOK;         /* a scaled plane has been seen to run */
    BOOL    h6_ScaleOff;        /* one did not: screens stay 1:1 */

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
     * bit measured to run at frame rate, 0 = none; h6_VSyncWait the
     * tasks sleeping on it, and h6_VSyncStamp dates the last tick so a
     * stopped interrupt is noticed instead of slept on. */
    APTR            h6_VSyncIrq;
    ULONG           h6_VSyncMask;
    volatile ULONG  h6_VSyncCount;
    volatile ULONG  h6_VSyncStamp;
    struct vcgfx_vblwait h6_VSyncWait;

    /* The firmware's colour path, put back with the boot mode: CSC_CTL,
     * the six coefficient words and CHANNEL_CTL, the output crossbar,
     * and the AVI infoframe's packet RAM slot. */
    ULONG   h6_BootCSC[8];
    ULONG   h6_BootXbar;
    ULONG   h6_BootAVI[9];
    BOOL    h6_BootAVIOn;

    BOOL    h6_CscLoaded;       /* YCbCr matrix slots 0 and 1 set */
};

#endif /* _VIDEOCOREGFX_HVS6STATE_H */
