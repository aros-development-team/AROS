#ifndef _VIDEOCOREGFX_HVS6STATE_H
#define _VIDEOCOREGFX_HVS6STATE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HVS6 display-list ownership state.
*/

#include <exec/types.h>

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
};

#endif /* _VIDEOCOREGFX_HVS6STATE_H */
