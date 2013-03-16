/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "graphics_intern.h"
#include "rgbconv.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef csd

static int GFX_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    EnterFunc(bug("GfxHIDD_Init()\n"));

    csd->cs_GfxBase = NULL;
    NEWLIST(&csd->pflist);
    InitSemaphore(&csd->sema);
    InitSemaphore(&csd->pfsema);
    InitSemaphore(&csd->rgbconvertfuncs_sem);

    /* X -> BGR032 */
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB24 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_RGB24_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR24 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_BGR24_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB16 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_RGB16_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_RGB16LE_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR16 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_BGR16_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR16_LE - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_BGR16LE_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_ARGB32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_ARGB32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGRA32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_BGRA32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGBA32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_RGBA32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_ABGR32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_ABGR32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_0RGB32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_ARGB32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_BGRA32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB032 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_RGBA32_To_BGR032;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_0BGR32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT] = Convert_ABGR32_To_BGR032;

    /* X -> RGB16_LE */
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB24 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_RGB24_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR24 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_BGR24_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB16 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_RGB16_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_RGB16LE_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR16 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_BGR16_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR16_LE - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_BGR16LE_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_ARGB32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_ARGB32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGRA32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_BGRA32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGBA32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_RGBA32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_ABGR32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_ABGR32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_0RGB32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_ARGB32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_BGRA32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB032 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_RGBA32_To_RGB16LE;
    csd->rgbconvertfuncs[vHidd_StdPixFmt_0BGR32 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT] = Convert_ABGR32_To_RGB16LE;

    /* BGR032 -> X */
    csd->rgbconvertfuncs[vHidd_StdPixFmt_BGR032 - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_ARGB32 - FIRST_RGB_STDPIXFMT] = Convert_BGR032_To_ARGB32;

    /* RGB16_LE -> X */
    csd->rgbconvertfuncs[vHidd_StdPixFmt_RGB16_LE - FIRST_RGB_STDPIXFMT]
        [vHidd_StdPixFmt_ARGB32 - FIRST_RGB_STDPIXFMT] = Convert_RGB16LE_To_ARGB32;

    ReturnInt("GfxHIDD_Init", ULONG, TRUE);
}

ADD2INITLIB(GFX_Init, -2)

