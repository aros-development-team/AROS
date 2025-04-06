/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Graphics function SetChipRev()
*/
#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>

#include <proto/graphics.h>

/* See rom/graphics/setchiprev.c for documentation */

AROS_LH1(ULONG, SetChipRev,
    AROS_LHA(ULONG, ChipRev, D0),
    struct GfxBase *, GfxBase, 148, Graphics)
{
    AROS_LIBFUNC_INIT

    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD vposr, deniseid1, deniseid2, deniseid3;
    UBYTE chipflags = 0;

    vposr = custom->vposr & 0x7f00;
    if ((vposr & 0x0200) == 0x0200) {
        chipflags = GFXF_AA_ALICE | GFXF_HR_AGNUS | GFXF_AA_LISA | GFXF_AA_MLISA | GFXF_HR_DENISE;
    } else if (vposr >= 0x2000) {
        chipflags = GFXF_HR_AGNUS;
        // ECS Agnus can be combined with different Denise chips.
        // DENISEID register does not exist in original Denise.
        // The hack below is likely intended to handle that case.
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

    return GfxBase->ChipRevBits0;

    AROS_LIBFUNC_EXIT
} /* SetChipRev */
