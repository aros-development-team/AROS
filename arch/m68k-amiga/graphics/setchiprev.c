/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Graphics function SetChipRev()
*/
#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>

#include "graphics_intern.h"

#include <interface/HW.h>
#include <interface/Hidd_AmigaGfx.h>

#include <proto/graphics.h>
#include <proto/oop.h>

#include <string.h>

/* See rom/graphics/setchiprev.c for documentation */

struct GfxHookData
{
    OOP_Object  *amigavideo_driver;
};

AROS_UFH3S(BOOL, enum_cb,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(OOP_Object *, obj, A2),
           AROS_UFHA(APTR, data, A1))
{
    AROS_USERFUNC_INIT
    struct GfxHookData *_data = (struct GfxHookData *)data;
    if (OOP_OCLASS(obj)->ClassNode.ln_Name && (!strcmp(OOP_OCLASS(obj)->ClassNode.ln_Name, "hidd.gfx.amigavideo"))) {
        _data->amigavideo_driver = obj;
        return TRUE;
    }
    else {
        return FALSE;
    }
    AROS_USERFUNC_EXIT
}

AROS_LH1(ULONG, SetChipRev,
    AROS_LHA(ULONG, ChipRev, D0),
    struct GfxBase *, GfxBase, 148, Graphics)
{
    AROS_LIBFUNC_INIT

    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD vposr, deniseid1, deniseid2, deniseid3;
    UBYTE chipflags = 0;
    UBYTE best_possible_flags = GFXF_AA_ALICE | GFXF_HR_AGNUS | GFXF_AA_LISA | GFXF_HR_DENISE | GFXF_AA_MLISA;

    vposr = custom->vposr & 0x7f00;
    if ((vposr & 0x0200) == 0x0200) {
        chipflags = GFXF_AA_ALICE | GFXF_HR_AGNUS | GFXF_AA_LISA | GFXF_AA_MLISA | GFXF_HR_DENISE;
    } else if (vposr >= 0x2000) {
        chipflags = GFXF_HR_AGNUS;
        // ECS Agnus can be combined with different Denise chips.
        // DENISEID register does not exist in original Denise,
        // so one cannot just read it once and trust it.
        Disable();
        deniseid1 = custom->deniseid & 0x00ff;
        custom->deniseid = custom->dmaconr;
        deniseid2 = custom->deniseid & 0x00ff;
        custom->deniseid = custom->dmaconr ^ 0x8000;
        deniseid3 = custom->deniseid & 0x00ff;
        Enable();
        if (deniseid1 == deniseid2 && deniseid2 == deniseid3 && deniseid1 == 0xfc)
            chipflags |= GFXF_HR_DENISE;
    }

    if (ChipRev != SETCHIPREV_BEST) {
        if (ChipRev == SETCHIPREV_A && chipflags >= SETCHIPREV_A)
            chipflags = SETCHIPREV_A;
        else if (ChipRev == SETCHIPREV_ECS && chipflags >= SETCHIPREV_ECS)
            chipflags = SETCHIPREV_ECS;
        else if (ChipRev == SETCHIPREV_AA && chipflags >= SETCHIPREV_AA)
            chipflags = SETCHIPREV_AA;
    }
    GfxBase->ChipRevBits0 = chipflags;

    // Notify the amigavideo driver if AGA is enabled
    if (chipflags == best_possible_flags || chipflags == SETCHIPREV_AA) {
        struct Hook enum_hook =
        {
            .h_Entry = enum_cb
        };
        struct GfxHookData hook_data =
        {
            .amigavideo_driver = NULL
        };
        APTR gfxroot = ((struct GfxBase_intern *)GfxBase)->GfxRoot;
        HW_EnumDrivers(gfxroot, &enum_hook, &hook_data);
        // The amigavideo HIDD should be found on this arch, but check anyway
        if (hook_data.amigavideo_driver) {
            OOP_MethodID HiddAmigaGfxBase = OOP_GetMethodID(IID_Hidd_AmigaGfx, 0);
            HIDD_AMIGAGFX_EnableAGA(hook_data.amigavideo_driver);
        }
    }

    return GfxBase->ChipRevBits0;

    AROS_LIBFUNC_EXIT
} /* SetChipRev */
