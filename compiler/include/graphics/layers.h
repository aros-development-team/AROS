#ifndef GRAPHICS_LAYERS_H
#define GRAPHICS_LAYERS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layer description
    Lang: english
*/

#include <exec/semaphores.h>
#include <graphics/layersext.h>

struct Layer_Info
{
    struct Layer    * top_layer;
    struct Layer    * check_lp;
    struct ClipRect * obs;
    struct ClipRect * FreeClipRects;

    LONG PrivateReserve1;
    LONG PrivateReserve2;

    struct SignalSemaphore Lock;
    struct MinList         gs_Head;

    WORD   PrivateReserve3;
    VOID * PrivateReserve4;

    UWORD  Flags;
    BYTE   fatten_count;
    BYTE   LockLayersCount;
    WORD   PrivateReserve5;
    VOID * BlankHook;
    VOID * LayerInfo_extra;
};

#define NEWLAYERINFO_CALLED 1

#define LAYERSIMPLE          (1<<0)
#define LAYERSMART           (1<<1)
#define LAYERSUPER           (1<<2)
#define LAYERUPDATING        (1<<4)
#define LAYERBACKDROP        (1<<6)
#define LAYERREFRESH         (1<<7)
#define LAYER_CLIPRECTS_LOST (1<<8)
#define LAYERIREFRESH        (1<<9)
#define LAYERIREFRESH2       (1<<10)

#define LAYERS_BACKFILL   ((struct Hook *)0)
#define LAYERS_NOBACKFILL ((struct Hook *)1)

/* LayerInfo Flag */

#define LIFLG_SUPPORTS_OFFSCREEN_LAYERS     (1 << 8)	/* Same flag as AmigaOS hack PowerWindowsNG */

#endif /* GRAPHICS_LAYERS_H */
