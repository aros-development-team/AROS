/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function MakeVPort()
*/

#include <aros/debug.h>
#include <graphics/modeid.h>
#include <graphics/view.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "graphics_driver.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, MakeVPort,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A0),
        AROS_LHA(struct ViewPort *, viewport, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 36, Graphics)

/*  FUNCTION
        Prepare a ViewPort to be displayed. Calculate all necessary internal data.
        For Amiga(tm) chipset bitmaps this includes calculating preliminary copperlists.

    INPUTS
        view     - pointer to a View structure
        viewport - pointer to a ViewPort structure
                   the viewport must have a valid pointer to a RasInfo

    RESULT
        error - Result of the operation:
            MVP_OK         - Everything is OK, ViewPort is ready
            MVP_NO_MEM     - There was not enough memory for internal data
            MVP_NO_VPE     - There was no ViewPortExtra for this ViewPort and no memory to
                             allocate a temporary one.
            MVP_NO_DSPINS  - There was not enough memory for Amiga(tm) copperlist.
            MVP_NO_DISPLAY - The BitMap can't be displayed using specified mode (for example,
                             misaligned or wrong depth).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPortExtra *vpe;
    struct HIDD_ViewPortData *vpd;
    struct BitMap *bitmap;
    struct gfxdisplay_data *mdd;
    HIDDT_ModeID modeid;
    ULONG ret = MVP_OK;
    BOOL own_vpe = FALSE;

    if(!viewport || !viewport->RasInfo || !viewport->RasInfo->BitMap)
        return MVP_NO_DISPLAY;

    bitmap = viewport->RasInfo->BitMap;
    modeid = GetVPModeID(viewport);
    mdd = (struct gfxdisplay_data *)GET_VP_DRIVERDATA(viewport);

    if (!IS_HIDD_BM(bitmap) && modeid == INVALID_ID) {
        struct DisplayInfoHandle *dih;

        modeid = ((GfxBase->DisplayFlags & NTSC) ?
                  NTSC_MONITOR_ID : PAL_MONITOR_ID) |
                 (viewport->Modes & (LACE | DOUBLESCAN | SUPERHIRES | PFBA |
                                     EXTRA_HALFBRITE | DUALPF | HAM | HIRES));
        if (viewport->Modes & SUPERHIRES)
            modeid |= HIRES;

        dih = (struct DisplayInfoHandle *)FindDisplayInfo(modeid);
        if (dih)
            mdd = dih->drv;
    }

    /* Attach a temporary ViewPortExtra if needed */
    vpe = (struct ViewPortExtra *)GfxLookUp(viewport);
    D(bug("[MakeVPort] ViewPort 0x%p, ViewPortExtra 0x%p\n", viewport, vpe));

    if(!vpe) {
        vpe = (struct ViewPortExtra *)GfxNew(VIEWPORT_EXTRA_TYPE);
        if(!vpe)
            return MVP_NO_VPE;

        vpe->Flags = VPXF_FREE_ME;
        GfxAssociate(viewport, &vpe->n);
        own_vpe = TRUE;
    }

    /* Now make sure that ViewPortData is created */
    if(!VPE_DATA(vpe))
        vpe->DriverData[0] = AllocMem(sizeof(struct HIDD_ViewPortData), MEMF_PUBLIC | MEMF_CLEAR);

    vpd = VPE_DATA(vpe);
    if(vpd) {
        BOOL current_owned = (vpe->Flags & VPXF_WRAPPED_BITMAP) != 0;

        vpd->vpe = vpe;

        if(IS_HIDD_BM(bitmap)) {
            if(vpd->Bitmap != HIDD_BM_OBJ(bitmap)) {
                if(vpd->PreviousBitmap) {
                    if(current_owned)
                        OOP_DisposeObject(vpd->Bitmap);
                } else {
                    vpd->PreviousBitmap = vpd->Bitmap;
                    if(current_owned)
                        vpd->Flags |= HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
                    else
                        vpd->Flags &= ~HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
                }
                vpd->Bitmap = HIDD_BM_OBJ(bitmap);
            }
            vpe->Flags &= ~VPXF_WRAPPED_BITMAP;
        } else if(current_owned) {
            struct BitMap *wrapped = NULL;

            OOP_GetAttr(vpd->Bitmap, aHidd_PlanarBM_BitMap,
                        (IPTR *)&wrapped);
            if(wrapped != bitmap &&
               !HIDD_PlanarBM_SetBitMap(vpd->Bitmap, bitmap))
                ret = MVP_NO_DISPLAY;
        } else {
            struct TagItem tags[] = {
                { aHidd_PlanarBM_BitMap, (IPTR)bitmap },
                { aHidd_BitMap_Width, bitmap->BytesPerRow << 3 },
                { aHidd_BitMap_Height, bitmap->Rows },
                { aHidd_BitMap_Depth, bitmap->Depth },
                { aHidd_BitMap_Displayable, TRUE },
                { aHidd_BitMap_ModeID, modeid },
                { TAG_DONE, 0 }
            };
            OOP_Object *newbitmap = HIDD_Display_CreateObject(
                mdd->display_obj, PrivGBase(GfxBase)->basebm, tags);

            if(newbitmap) {
                if(vpd->PreviousBitmap) {
                    if(current_owned)
                        OOP_DisposeObject(vpd->Bitmap);
                } else {
                    vpd->PreviousBitmap = vpd->Bitmap;
                    if(current_owned)
                        vpd->Flags |= HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
                    else
                        vpd->Flags &= ~HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
                }

                vpd->Bitmap = newbitmap;
                vpe->Flags |= VPXF_WRAPPED_BITMAP;
            } else {
                ret = MVP_NO_MEM;
            }
        }

        D(bug("[MakeVPort] Bitmap object: 0x%p\n", vpd->Bitmap));

        if(IS_HIDD_BM(bitmap)) {
            /*
             * If we have a colormap attached to a HIDD bitmap, we can verify
             * that bitmap and colormap modes do not differ.
             */
            if(viewport->ColorMap) {
                struct DisplayInfoHandle *dih = viewport->ColorMap->NormalDisplayInfo;

                if(dih) {
                    if(((struct gfxdisplay_data *)HIDD_BM_DRVDATA(viewport->RasInfo->BitMap) != dih->drv) ||
                            (HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap) != dih->id)) {

                        D(bug("[MakeVPort] Bad NormalDisplayInfo\n"));
                        D(bug("[MakeVPort] Driverdata: ViewPort 0x%p, BitMap 0x%p\n", dih->drv, HIDD_BM_DRVDATA(viewport->RasInfo->BitMap)));
                        D(bug("[MakeVPort] HIDD ModeID: ViewPort 0x%p, BitMap 0x%p\n", dih->id, HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap)));
                        ret = MVP_NO_DISPLAY;
                    }
                }

                if(viewport->ColorMap->VPModeID != INVALID_ID) {
                    if(GET_BM_MODEID(viewport->RasInfo->BitMap) != viewport->ColorMap->VPModeID) {
                        D(bug("[MakeVPort] Bad ModeID, ViewPort 0x%08lX, BitMap 0x%08lX\n", viewport->ColorMap->VPModeID,
                              GET_BM_MODEID(viewport->RasInfo->BitMap)));
                        ret = MVP_NO_DISPLAY;
                    }
                }
            }
        }

        /*
         * Ensure that we have a bitmap object.
         * OBTAIN_HIDD_BM() may fail on planar bitmap in low memory situation.
         */
        if(ret == MVP_OK && vpd->Bitmap) {
            /*
             * Store driverdata pointer in private ViewPortExtra field.
             * It is needed because the caller can first free BitMap, then
             * its ViewPort. In this case we won't be able to retrieve
             * driver pointer from the bitmap in FreeVPortCopLists().
             */
            vpe->DriverData[1] = mdd;
            ret = HIDD_Display_MakeViewPort(mdd->display_obj, vpd, view);
        } else if(ret == MVP_OK)
            ret = MVP_NO_MEM;

        if(ret != MVP_OK && vpd->PreviousBitmap) {
            if(vpe->Flags & VPXF_WRAPPED_BITMAP)
                OOP_DisposeObject(vpd->Bitmap);

            vpd->Bitmap = vpd->PreviousBitmap;
            vpd->PreviousBitmap = NULL;
            if(vpd->Flags & HIDD_VPDF_PREVIOUS_BITMAP_OWNED)
                vpe->Flags |= VPXF_WRAPPED_BITMAP;
            else
                vpe->Flags &= ~VPXF_WRAPPED_BITMAP;
            vpd->Flags &= ~HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
        }
    } else
        ret = MVP_NO_MEM;

    if(ret == MVP_OK)
        /* Use ScrollVPort() in order to validate offsets */
        ScrollVPort(viewport);
    else {
        if(own_vpe)
            GfxFree(&vpe->n);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
