#ifndef _VIDEOCOREGFX_HVS5STATE_H
#define _VIDEOCOREGFX_HVS5STATE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 mode setting state (vcgfx_hvs5_mode.c).
*/

#include <exec/types.h>

#include "vcgfx_edid.h"

/* The firmware's boot mode on HDMI0, kept to put back verbatim, and the
 * mode running now. m5_ModeOK gates every mode of our own. */
struct vc4_hvs5_mode
{
    BOOL    m5_ModeOK;
    BOOL    m5_Ours;            /* a mode of ours is set               */
    ULONG   m5_PixSrc;          /* Hz into PV2's clock divider         */
    struct vcgfx_timing m5_BootTiming;
    struct vcgfx_timing m5_CurTiming;
    ULONG   m5_BootPV[7];       /* PV2 +0x00..+0x18                    */
    ULONG   m5_BootHDMI[6];     /* HORZA HORZB VERTA0 VERTB0 VERTA1 VERTB1 */
    ULONG   m5_BootVidCtl;
    ULONG   m5_BootCtrl;        /* HVS DISPCTRL of the HDMI0 channel   */
    ULONG   m5_BootPHY[6];      /* lanes, band, refclk, div, frac, rm  */
    ULONG   m5_BootHSM;
    ULONG   m5_BootPixDiv;
    ULONG   m5_BootCSC[6];
    ULONG   m5_BootAVI[9];
    BOOL    m5_BootAVIOn;
};

#endif /* _VIDEOCOREGFX_HVS5STATE_H */
