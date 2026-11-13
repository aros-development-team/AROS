#ifndef HeadlessGFX_HIDD_H
#define HeadlessGFX_HIDD_H

/*
    Copyright (C) 2021-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headless Gfx Hidd data.
    Lang: English.
*/

#include <exec/interrupts.h>

#define IID_Hidd_Gfx_Headless  "hidd.gfx.headless"
#define CLID_Hidd_Gfx_Headless "hidd.gfx.headless"
#define CLID_Hidd_Display_Headless "hidd.display.headless"

/*
 * Private, creation-time attributes. The monitor loader passes these
 * to AddDisplayDriverA() from its icon's tooltypes; obtain the base
 * with OOP_ObtainAttrBase(IID_Hidd_Gfx_Headless).
 */
enum
{
    aoHidd_Gfx_Headless_MaxDepth,   /* [I.G] (ULONG) Deepest mode to expose: 8, 15, 16 or 24 (default 24) */
    aoHidd_Gfx_Headless_FixedDepth, /* [I.G] (ULONG) Expose only this depth; overrides MaxDepth           */
    aoHidd_Gfx_Headless_Width,      /* [I.G] (ULONG) Display width  (default 1024)                       */
    aoHidd_Gfx_Headless_Height,     /* [I.G] (ULONG) Display height (default 768)                        */
    aoHidd_Gfx_Headless_NominalDepth, /* [I.G] (ULONG) Depth to prefer when opening a display; clamped
                                              to the depths actually exposed (default: the deepest)      */

    num_Hidd_Gfx_Headless_Attrs
};

#define aHidd_Gfx_Headless_MaxDepth   (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_MaxDepth)
#define aHidd_Gfx_Headless_FixedDepth (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_FixedDepth)
#define aHidd_Gfx_Headless_Width      (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_Width)
#define aHidd_Gfx_Headless_Height     (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_Height)
#define aHidd_Gfx_Headless_NominalDepth (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_NominalDepth)

#define IS_HEADLESSGFX_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGfxHeadlessAttrBase) < num_Hidd_Gfx_Headless_Attrs)

struct HeadlessGfxDisplayData
{
};

struct HeadlessGfxHiddData
{
    ULONG maxDepth;
    ULONG fixedDepth;
};

#endif /* HeadlessGFX_HIDD_H */
