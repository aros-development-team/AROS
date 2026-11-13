/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.

    Desc: Gfx Hidd color conversion initialization code.
*/
#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "gfx_intern.h"

#include LC_LIBDEFS_FILE

#undef csd

void SetRGBConversionFunctions(HIDDT_RGBConversionFunction rgbconvertfuncs[NUM_RGB_STDPIXFMT][NUM_RGB_STDPIXFMT]);
void SetArchRGBConversionFunctions(HIDDT_RGBConversionFunction rgbconvertfuncs[NUM_RGB_STDPIXFMT][NUM_RGB_STDPIXFMT]);

static int ColorConv_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    EnterFunc(bug("ColorConv_Init()\n"));

    ObtainSemaphore(&csd->rgbconvertfuncs_sem);

    SetRGBConversionFunctions(csd->rgbconvertfuncs);
    SetArchRGBConversionFunctions(csd->rgbconvertfuncs);

    ReleaseSemaphore(&csd->rgbconvertfuncs_sem);

    ReturnInt("ColorConv_Init", ULONG, TRUE);
}

ADD2INITLIB(ColorConv_Init, -1)

