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

    num_Hidd_Gfx_Headless_Attrs
};

#define aHidd_Gfx_Headless_MaxDepth   (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_MaxDepth)
#define aHidd_Gfx_Headless_FixedDepth (HiddGfxHeadlessAttrBase + aoHidd_Gfx_Headless_FixedDepth)

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
