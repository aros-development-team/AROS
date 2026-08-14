/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"
#include "compositor.h"

#include <graphics/displayinfo.h>
#include <graphics/driver.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include <libdrm/arosdrmmode.h>
#include <uapi/drm/nouveau_drm.h>
#include <drm-compat/drm_compat_pci.h>

#undef HiddAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase
#undef HiddGfxNouveauAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddCompositorAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddAttrBase          (SD(cl)->hiddAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase             (SD(cl)->gfxAttrBase)
#define HiddDisplayAttrBase         (SD(cl)->displayAttrBase)
#define HiddDMEnumAttrBase          (SD(cl)->dmenumAttrBase)
#define HiddGfxNouveauAttrBase      (SD(cl)->gfxNouveauAttrBase)
#define HiddSyncAttrBase            (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddCompositorAttrBase     (SD(cl)->compositorAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

#define MAX_BITMAP_WIDTH    4096
#define MAX_BITMAP_HEIGHT   4096
#define GART_BUFFER_SIZE    (12 * 1024 * 1024)

/* HELPER FUNCTIONS */
VOID HIDDNouveauShowCursor(OOP_Object * gfx, BOOL visible)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, gfx);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    LOCK_ENGINE

    if (visible)
    {
        drmModeSetCursor(nvdev->fd, gfxdata->selectedcrtcid, 
            gfxdata->cursor->handle, 64, 64);
    }
    else
    {
        drmModeSetCursor(nvdev->fd, gfxdata->selectedcrtcid, 
            0, 64, 64);
    }

    UNLOCK_ENGINE
}

static BOOL HIDDNouveauSelectConnectorCrtc(LONG fd, drmModeConnectorPtr * selectedconnector,
    drmModeCrtcPtr * selectedcrtc)
{
    *selectedconnector = NULL;
    *selectedcrtc = NULL;
    drmModeResPtr drmmode = NULL;
    LONG i; ULONG crtc_id;

    LOCK_ENGINE

    /* Get all components information */
    drmmode = drmModeGetResources(fd);
    if (!drmmode)
    {
        D(bug("[Nouveau] Not able to get resources information\n"));
        UNLOCK_ENGINE
        return FALSE;
    }
    
    /* Selecting connector */
    for (i = 0; i < drmmode->count_connectors; i++)
    {
        drmModeConnectorPtr connector = drmModeGetConnector(fd, drmmode->connectors[i]);

        if (connector)
        {
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                /* Found connected connector */
                *selectedconnector = connector;
                break;
            }
            
            drmModeFreeConnector(connector);
        }
    }
    
    if (!(*selectedconnector))
    {
        D(bug("[Nouveau] No connected connector\n"));
        drmModeFreeResources(drmmode);
        UNLOCK_ENGINE
        return FALSE;
    }

    /* Selecting first available CRTC */
    if (drmmode->count_crtcs > 0)
        crtc_id = drmmode->crtcs[0];
    else
        crtc_id = 0;

    *selectedcrtc = drmModeGetCrtc(fd, crtc_id);
    if (!(*selectedcrtc))
    {
        D(bug("[Nouveau] Not able to get crtc information for crtc_id %d\n", crtc_id));
        drmModeFreeConnector(*selectedconnector);
        *selectedconnector = NULL;
        drmModeFreeResources(drmmode);
        UNLOCK_ENGINE
        return FALSE;
    }
    
    drmModeFreeResources(drmmode);
    UNLOCK_ENGINE
    return TRUE;
}    

#include <stdio.h>

/*
 * How suitable a mode is as the automatic choice for its size: the one
 * the display asked for first, then whatever sits closest to 60Hz.
 * Lower is better.
 */
static LONG HIDDNouveauModeRank(drmModeModeInfoPtr mode)
{
    LONG off;

    if (mode->type & DRM_MODE_TYPE_PREFERRED)
        return -1;

    off = (LONG)mode->vrefresh - 60;
    if (off < 0)
        off = -off;

    return off;
}

static struct TagItem * HIDDNouveauCreateSyncTagsFromConnector(OOP_Class * cl, drmModeConnectorPtr connector)
{
    struct TagItem * syncs = NULL;
    ULONG modescount = connector->count_modes;
    ULONG i;
    
    if (modescount == 0)
        return NULL;
        
    /* Allocate enough structures */
    syncs = HIDDNouveauAlloc(sizeof(struct TagItem) * modescount);
    
    /*
     * The list arrives ordered the way a display driver likes it -
     * biggest first, and within a size the fastest first. Left that
     * way, the mode picked for a screen of a given size is the highest
     * rate the display claims, which is the one it is least likely to
     * hold. Reorder within each size so the mode the display prefers
     * leads, and otherwise the one nearest 60Hz. Everything stays
     * available; only which is chosen by default changes.
     */
    {
        ULONG start;

        for (start = 0; start < modescount; )
        {
            ULONG end, j, best;

            /* Modes of one size are already adjacent */
            for (end = start + 1; end < modescount; end++)
            {
                if ((connector->modes[end].hdisplay !=
                     connector->modes[start].hdisplay) ||
                    (connector->modes[end].vdisplay !=
                     connector->modes[start].vdisplay))
                    break;
            }

            best = start;
            for (j = start + 1; j < end; j++)
            {
                if (HIDDNouveauModeRank(&connector->modes[j]) <
                    HIDDNouveauModeRank(&connector->modes[best]))
                    best = j;
            }

            if (best != start)
            {
                drmModeModeInfo tmp = connector->modes[start];
                connector->modes[start] = connector->modes[best];
                connector->modes[best] = tmp;
            }

            start = end;
        }
    }

    for (i = 0; i < modescount; i++)
    {
        struct TagItem * sync = HIDDNouveauAlloc(sizeof(struct TagItem) * 15);
        LONG j = 0;
        
        drmModeModeInfoPtr mode = &connector->modes[i];

        sync[j].ti_Tag = aHidd_Sync_PixelClock;     sync[j++].ti_Data = mode->clock;

        sync[j].ti_Tag = aHidd_Sync_HDisp;          sync[j++].ti_Data = mode->hdisplay;
        sync[j].ti_Tag = aHidd_Sync_HSyncStart;     sync[j++].ti_Data = mode->hsync_start;
        sync[j].ti_Tag = aHidd_Sync_HSyncEnd;       sync[j++].ti_Data = mode->hsync_end;
        sync[j].ti_Tag = aHidd_Sync_HTotal;         sync[j++].ti_Data = mode->htotal;
        sync[j].ti_Tag = aHidd_Sync_HMin;           sync[j++].ti_Data = mode->hdisplay;
        sync[j].ti_Tag = aHidd_Sync_HMax;           sync[j++].ti_Data = MAX_BITMAP_WIDTH;

        sync[j].ti_Tag = aHidd_Sync_VDisp;          sync[j++].ti_Data = mode->vdisplay;
        sync[j].ti_Tag = aHidd_Sync_VSyncStart;     sync[j++].ti_Data = mode->vsync_start;
        sync[j].ti_Tag = aHidd_Sync_VSyncEnd;       sync[j++].ti_Data = mode->vsync_end;
        sync[j].ti_Tag = aHidd_Sync_VTotal;         sync[j++].ti_Data = mode->vtotal;
        sync[j].ti_Tag = aHidd_Sync_VMin;           sync[j++].ti_Data = mode->vdisplay;
        sync[j].ti_Tag = aHidd_Sync_VMax;           sync[j++].ti_Data = MAX_BITMAP_HEIGHT;
        
        /* Name */
        STRPTR syncname = HIDDNouveauAlloc(32);
        sprintf(syncname, "NV:%dx%d@%d", mode->hdisplay, mode->vdisplay, mode->vrefresh);
        
        sync[j].ti_Tag = aHidd_Sync_Description;   sync[j++].ti_Data = (IPTR)syncname;
        
        sync[j].ti_Tag = TAG_DONE;                 sync[j++].ti_Data = 0UL;
        
        syncs[i].ti_Tag = aHidd_DMEnum_SyncTags;
        syncs[i].ti_Data = (IPTR)sync;
    }
    
    return syncs;
}

/* This function assumes that the mode, crtc and output are already selected */
static BOOL HIDDNouveauShowBitmapForSelectedMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauData * gfxdata = NULL;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    uint32_t output_ids[] = {0};
    uint32_t output_count = 1;
    IPTR e = (IPTR)NULL;
    OOP_Object * gfx = NULL;
    LONG ret;

    LOCK_ENGINE
    LOCK_BITMAP
    
    /* Check if passed bitmap has been registered as framebuffer */
    if (bmdata->fbid == 0)
    {
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        return FALSE;
    }
    
    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    output_ids[0] = ((drmModeConnectorPtr)gfxdata->selectedconnector)->connector_id;
    

    ret = drmModeSetCrtc(nvdev->fd, gfxdata->selectedcrtcid,
            bmdata->fbid, -bmdata->xoffset, -bmdata->yoffset, output_ids,
            output_count, gfxdata->selectedmode);

    UNLOCK_BITMAP
    UNLOCK_ENGINE

    if (ret) return FALSE; else return TRUE;
}

BOOL HIDDNouveauSwitchToVideoMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    OOP_Object * gfx = NULL;
    struct HIDDNouveauData * gfxdata = NULL; 
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;

    LOCK_ENGINE

    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    selectedconnector = (drmModeConnectorPtr)gfxdata->selectedconnector;

    D(bug("[Nouveau] HIDDNouveauSwitchToVideoMode, bm: 0x%x\n", bm));
    
    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Nouveau] Invalid ModeID\n"));
        UNLOCK_ENGINE
        return FALSE;
    }

    /* Get Sync and PixelFormat properties */
    HIDD_DMEnum_GetMode(SD(cl)->dmenum, modeid, &sync, &pf);

    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);    
    
    D(bug("[Nouveau] Sync: %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    pixel, hdisp, hstart, hend, htotal, vdisp, vstart, vend, vtotal));

    D(bug("[Nouveau] Connector %d, CRTC %d\n", 
        selectedconnector->connector_id, gfxdata->selectedcrtcid));

    /* Select mode */
    gfxdata->selectedmode = NULL;
    for (i = 0; i < selectedconnector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &selectedconnector->modes[i];
        
        if ((mode->hdisplay == hdisp) && (mode->vdisplay == vdisp) &&
            (mode->hsync_start == hstart) && (mode->vsync_start == vstart) &&
            (mode->hsync_end == hend) && (mode->vsync_end == vend))
        {
            gfxdata->selectedmode = mode;
            break;
        }
    }
    
    if (!gfxdata->selectedmode)
    {
        D(bug("[Nouveau] Not able to select mode\n"));
        UNLOCK_ENGINE
        return FALSE;
    }

    /* For screen switching the bitmap might have already once been a framebuffer 
       - check bmdata->fbid. Also the bitmap itself needs to know whether it is 
       added as framebuffer so that it can unregister itself in Dispose */

    /* Add as frame buffer */
    if (bmdata->fbid == 0)
    {
	    ret = drmModeAddFB(nvdev->fd, bmdata->drawable.width, bmdata->drawable.height,
	                bmdata->drawable.depth, bmdata->bytesperpixel * 8,
	                bmdata->pitch, bmdata->bo->handle, &bmdata->fbid);
        if (ret)
        {
            D(bug("[Nouveau] Not able to add framebuffer\n"));
            UNLOCK_ENGINE
            return FALSE;
        }
    }


    /* Switch mode */
    if (!HIDDNouveauShowBitmapForSelectedMode(bm))
    {
        D(bug("[Nouveau] Not able to set crtc\n"));
        UNLOCK_ENGINE
        return FALSE;        
    }

    HIDDNouveauShowCursor(gfx, TRUE);

    UNLOCK_ENGINE
    return TRUE;
}

/*
 * Retire whatever the firmware left driving this card.
 *
 * A boot-mode display driver - a GOP or VESA console - keeps writing to
 * the aperture the firmware handed it. That is harmless while the
 * aperture is plain memory, and fatal when it is one of this card's
 * BARs: on the Milk-V Titan the GOP framebuffer *is* BAR1, so once the
 * probe below installs nouveau's own BAR1 page tables, the console's
 * next redraw sweeps the channels' USERD and the fence buffer instead of
 * a screen.
 *
 * So we describe our apertures to graphics.library and let it hand back
 * the boot displays that overlap them, one at a time, for shutdown. A
 * boot driver on a different card overlaps nothing and is left running.
 *
 * Returns FALSE if a display that shares this card could not be shut
 * down. The caller must then abandon the whole probe: the card has not
 * been touched yet, so refusing to load leaves a working firmware
 * console on screen, where carrying on regardless would leave two
 * drivers writing to the same hardware - which is the failure this
 * whole mechanism exists to prevent.
 */
static BOOL HIDDNouveauReleaseBootDisplays(struct pci_dev *pdev,
    struct DisplayHandover *handover)
{
    struct DisplayRange ranges[7];
    ULONG count = 0;
    ULONG bar;
    APTR handle;

    for (bar = 0; bar < 6; bar++)
    {
        resource_size_t start = pci_resource_start(pdev, bar);
        unsigned long len = pci_resource_len(pdev, bar);
        APTR cpu;

        if ((start == 0) || (len == 0))
            continue;

        /*
         * Boot drivers describe their framebuffer by the pointer they
         * write through, so the comparison has to happen in CPU
         * addresses. Translating rather than ioremap()ing keeps this
         * side-effect free - we are only asking where these apertures
         * would be, not asking for them to be mapped.
         */
        cpu = pci_resource_cpu_addr(start);
        if ((cpu == NULL) || (cpu == (APTR)-1))
        {
            D(bug("[Nouveau] BAR%lu (0x%p) has no CPU address, cannot match it\n",
                  (unsigned long)bar, (APTR)(IPTR)start));
            continue;
        }

        ranges[count].dr_Base = cpu;
        ranges[count].dr_Size = len;
        D(bug("[Nouveau] BAR%lu occupies 0x%p, %lu bytes\n",
              (unsigned long)bar, cpu, len));
        count++;
    }

    ranges[count].dr_Base = NULL;
    ranges[count].dr_Size = 0;

    /*
     * With nothing describable to match on, fall back to "conflicts with
     * everything". Losing a boot display we could have kept costs a
     * screen; keeping one we should have lost corrupts this card.
     */
    while ((handle = handover->dho_FindDisplay(handover->dho_Context,
                                               count ? ranges : NULL)))
    {
        if (!handover->dho_ExpungeDisplay(handover->dho_Context, handle))
        {
            bug("[Nouveau] boot display 0x%p shares this card and is still in"
                " use - not taking the card over\n", handle);
            return FALSE;
        }

        D(bug("[Nouveau] boot display 0x%p released this card\n", handle));
    }

    return TRUE;
}

/* PUBLIC METHODS */
OOP_Object * METHOD(Nouveau, Root, New)
{
    drmModeCrtcPtr selectedcrtc = NULL;
    drmModeConnectorPtr selectedconnector = NULL;
    struct nouveau_device *nvdev = NULL;
    struct nouveau_client *nvclient = NULL;
    struct TagItem * syncs = NULL;
    struct CardData * carddata = &(SD(cl)->carddata);
    struct DisplayHandover *handover;
    struct pci_dev *pdev;
    LONG ret;
    ULONG selectedcrtcid;

    pdev = nouveau_init_findcard();
    if (!pdev)
        return NULL;

    /*
     * Between finding the card and touching it: take down anything the
     * firmware left driving it. graphics.library only offers the
     * handover here, in New(), where it holds the display database for
     * us - the interface must not be kept for later.
     */
    handover = (struct DisplayHandover *)GetTagData(DDRVA_Handover, 0, msg->attrList);
    if (handover)
    {
        if (!HIDDNouveauReleaseBootDisplays(pdev, handover))
            return NULL;
    }
    else
        D(bug("[Nouveau] no handover offered, boot drivers left alone\n"));

    if (nouveau_init_probe(pdev) < 0)
        return NULL;

    LOCK_ENGINE

    nouveau_device_open("", &nvdev);

    nouveau_client_new(nvdev, &nvclient);


    /* Select crtc and connector */
    if (!HIDDNouveauSelectConnectorCrtc(nvdev->fd, &selectedconnector, &selectedcrtc))
    {
        D(bug("[Nouveau] Not able to select connector and crtc\n"));

        UNLOCK_ENGINE

        return NULL;
    }
    
    selectedcrtcid = selectedcrtc->crtc_id;
    drmModeFreeCrtc(selectedcrtc);

    /* Read connector and build sync tags */
    syncs = HIDDNouveauCreateSyncTagsFromConnector(cl, selectedconnector);
    if (syncs == NULL)
    {
        D(bug("[Nouveau] Not able to read any sync modes\n"));
        UNLOCK_ENGINE
        return NULL;
    }
    

    /* Call super contructor */
    {
        struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,	8	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	16	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		24	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	4	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	24	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
        };

        struct TagItem pftags_16bpp[] = {
        { aHidd_PixFmt_RedShift,	16	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	21	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	27	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x0000f800 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x000007e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		16	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	2	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	16	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_RGB16_LE }, /* 12 */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
        };

        struct TagItem modetags[] = {
	    { aHidd_DMEnum_PixFmtTags,	(IPTR)pftags_24bpp	},
	    { aHidd_DMEnum_PixFmtTags,	(IPTR)pftags_16bpp	},
        { TAG_MORE, (IPTR)syncs },  /* FIXME: sync tags will leak */
	    { TAG_DONE, 0UL }
        };

        struct TagItem mytags[] = {
            { aHidd_Name            , (IPTR)"Nouveau"     },
            { aHidd_HardwareName    , (IPTR)"Nvidia Gfx Adaptor"   },
            { aHidd_ProducerName    , (IPTR)"Nvidia Corporation"  },
	    { TAG_MORE, (IPTR)msg->attrList }
        };

        struct pRoot_New mymsg;

        mymsg.mID = msg->mID;
        mymsg.attrList = mytags;

        msg = &mymsg;


        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        D(bug("[Nouveau] GFX New\n"));

        if (o)
        {
            struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, o);
            /* Pass local information to class */
            gfxdata->selectedcrtcid = selectedcrtcid;
            gfxdata->selectedmode = NULL;
            gfxdata->selectedconnector = selectedconnector;
            carddata->dev = nvdev;
            carddata->client = nvclient;
            ULONG gartsize = 0;
            UQUAD value;

            /* Create the display object and its mode enumerator */
            {
                struct TagItem displaytags[] =
                {
                    { aHidd_Display_GfxHidd,  (IPTR)o        },
                    { aHidd_Display_ModeTags, (IPTR)modetags },
                    { TAG_DONE,               0              }
                };
                SD(cl)->display = OOP_NewObject(SD(cl)->displayclass, NULL, displaytags);
                if (SD(cl)->display)
                    OOP_GetAttr(SD(cl)->display, aHidd_Display_DMEnumerator, (IPTR *)&SD(cl)->dmenum);
            }

            /* Check chipset Architecture */
            switch (carddata->dev->chipset & 0xff0)
            {
            case 0x000:
                carddata->Architecture = NV_ARCH_04;
                break;
            case 0x010:
                carddata->Architecture = NV_ARCH_10;
                break;
            case 0x020:
                carddata->Architecture = NV_ARCH_20;
                break;
            case 0x030:
                carddata->Architecture = NV_ARCH_30;
                break;
            case 0x040:
            case 0x060:
                carddata->Architecture = NV_ARCH_40;
                break;
            case 0x050:
            case 0x080:
            case 0x090:
            case 0x0a0:
                carddata->Architecture = NV_TESLA;
                break;
            case 0x0c0:
            case 0x0d0:
                carddata->Architecture = NV_FERMI;
                break;
            case 0x0e0:
            case 0x0f0:
            case 0x100:
                carddata->Architecture = NV_KEPLER;
                break;
            case 0x110:
            case 0x120:
                carddata->Architecture = NV_MAXWELL;
                break;
            case 0x130:
                carddata->Architecture = NV_PASCAL;
                break;
            default:
                bug("Unrecognized chipset: 0x%x, exiting.\n", carddata->dev->chipset);
                UNLOCK_ENGINE
                return NULL;
            }
            
            nouveau_getparam(carddata->dev, NOUVEAU_GETPARAM_BUS_TYPE, &value);
            if (value == 2 /* NV_PCIE */)
                carddata->IsPCIE = TRUE;
            else
                carddata->IsPCIE = FALSE;

            /* Allocate buffer object for cursor */
            nouveau_bo_new(carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 64 * 64 * 4, NULL, &gfxdata->cursor);
            /* TODO: Check return, how to handle */

            /* Initialize acceleration objects */
        
            ret = HIDDNouveauAccelCommonInit(carddata);
            if (ret < 0)
            {
                /* TODO: Check ret, how to handle ? */
            }

            /* Allocate GART scratch buffer */
            if (carddata->dev->gart_size > GART_BUFFER_SIZE)
                gartsize = GART_BUFFER_SIZE;
            else
                /* always leave 512kb for other things like the fifos */
                gartsize = carddata->dev->gart_size - 512 * 1024;

            /* This can fail */
            nouveau_bo_new(carddata->dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, gartsize, NULL, &carddata->GART);

            InitSemaphore(&carddata->gartsemaphore);
            
            /* Set initial pattern (else 16-bit ROPs are not working) */
            switch(carddata->Architecture)
            {
            case(NV_ARCH_03):
            case(NV_ARCH_04):
            case(NV_ARCH_10):
            case(NV_ARCH_20):
            case(NV_ARCH_30):
            case(NV_ARCH_40):
                HIDDNouveauNV04SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            case(NV_TESLA):
                HIDDNouveauNV50SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            case(NV_FERMI):
            case(NV_KEPLER):
            case(NV_MAXWELL):
            case(NV_PASCAL):
                HIDDNouveauNVC0SetPattern(carddata, ~0, ~0, ~0, ~0);
                break;
            }

            /* Create compositor object */
            {
                struct TagItem comptags [] =
                {
                    { aHidd_Compositor_DisplayHidd, (IPTR)SD(cl)->display },
                    { TAG_DONE, TAG_DONE }
                };
                gfxdata->compositor = OOP_NewObject(SD(cl)->compositorclass, NULL, comptags);
                SD(cl)->compositor = gfxdata->compositor;
                /* TODO: Check if object was created, how to handle ? */
            }

        }
        UNLOCK_ENGINE

        return o;
    }
    UNLOCK_ENGINE

    return NULL;
}

/* FIXME: IMPLEMENT DISPOSE - calling nouveau_close(), freeing cursor bo, gart bo, 
    selectedconnector, gfxdata->compositor, HIDDNouveauAccelFree */

/* FIXME: IMPLEMENT DISPOSE BITMAP - REMOVE FROM FB IF MARKED AS SUCH */

VOID METHOD(NouveauDisplay, Root, Get)
{
    ULONG idx;

    Hidd_Display_Switch(msg->attrID, idx)
    {
    case aoHidd_Display_SpriteTypes:
        *msg->storage = vHidd_SpriteType_DirectColor;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(NouveauDisplay, Hidd_Display, CreateObject)
{
    OOP_Object      *object = NULL;

    if (msg->cl == SD(cl)->basebm)
    {
        struct pHidd_Display_CreateObject mymsg;
        HIDDT_ModeID modeid;
        HIDDT_StdPixFmt stdpf;

        struct TagItem mytags [] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_Align */
            { aHidd_BitMap_Nouveau_CompositorHidd, (IPTR)SD(cl)->compositor },
            { TAG_MORE, (IPTR)msg->attrList }
        };

        /* Check if user provided valid ModeID */
        /* Check for framebuffer - not needed as Nouveau is a NoFramebuffer driver */
        /* Check for displayable - not needed - displayable has ModeID and we don't
           distinguish between on-screen and off-screen bitmaps */
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (vHidd_ModeID_Invalid != modeid) 
        {
            /* User supplied a valid modeid. We can use our bitmap class */
            mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
            mytags[0].ti_Data	= (IPTR)SD(cl)->bmclass;
        } 

        /* Check if bitmap is a planar bitmap */
        stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
        if (vHidd_StdPixFmt_Plane == stdpf)
        {
            mytags[1].ti_Tag    = aHidd_BitMap_Align;
            mytags[1].ti_Data   = 32;
        }
        
        /* We init a new message struct */
        mymsg.mID	= msg->mID;
        mymsg.cl	= msg->cl;
        mymsg.attrList	= mytags;

        /* Pass the new message to the superclass */
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    }
    else if (SD(cl)->basegallium && (msg->cl == SD(cl)->basegallium))
    {
        /* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_Nouveau, msg->attrList);
    }
    else if (SD(cl)->basei2c && (msg->cl == SD(cl)->basei2c))
    {
        /* Expose the i2c bus object .. */
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}

VOID METHOD(Nouveau, Hidd_Gfx, CopyBox)
{
    OOP_Class * srcclass = OOP_OCLASS(msg->src);
    OOP_Class * destclass = OOP_OCLASS(msg->dest);

    if (IS_NOUVEAU_BM_CLASS(srcclass) && IS_NOUVEAU_BM_CLASS(destclass))
    {
        /* FIXME: add checks for pixel format, etc */
        struct HIDDNouveauBitMapData * srcdata = OOP_INST_DATA(srcclass, msg->src);
        struct HIDDNouveauBitMapData * destdata = OOP_INST_DATA(destclass, msg->dest);
        struct CardData * carddata = &(SD(cl)->carddata);
        BOOL ret = FALSE;
        
        D(bug("[Nouveau] CopyBox %p -> %p\n", msg->src, msg->dest));

        LOCK_ENGINE

        LOCK_MULTI_BITMAP
        LOCK_BITMAP_BM(srcdata)
        LOCK_BITMAP_BM(destdata)
        UNLOCK_MULTI_BITMAP
        
        switch(carddata->Architecture)
        {
        case(NV_ARCH_03):
        case(NV_ARCH_04):
        case(NV_ARCH_10):
        case(NV_ARCH_20):
        case(NV_ARCH_30):
        case(NV_ARCH_40):
            ret = HIDDNouveauNV04CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        case(NV_TESLA):
            ret = HIDDNouveauNV50CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        case(NV_FERMI):
        case(NV_KEPLER):
        case(NV_MAXWELL):
        case(NV_PASCAL):
            ret = HIDDNouveauNVC0CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        }

nouveau_bo_wait(destdata->bo, NOUVEAU_BO_RD, carddata->client);

        UNLOCK_BITMAP_BM(destdata);
        UNLOCK_BITMAP_BM(srcdata);

        UNLOCK_ENGINE

        if (ret)
            return;
        
        /* If operation failed, fallback to default method */
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    
}

VOID METHOD(Nouveau, Root, Get)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_NoFrameBuffer:
            *msg->storage = (IPTR)TRUE;
            return;
        case aoHidd_Gfx_SupportsHWCursor:
            *msg->storage = (IPTR)TRUE;
            return;
        case aoHidd_Gfx_DisplayDefault:
            *msg->storage = (IPTR)SD(cl)->display;
            return;
        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR)"Nouveau";
            return;
        case aoHidd_Gfx_MemoryAttribs:
            {
                struct TagItem *matstate = (struct TagItem *)msg->storage;
                if (matstate)
                {
                    struct TagItem *matag;
                    while ((matag = NextTagItem(&matstate)))
                    {
                        switch(matag->ti_Tag)
                        {
                            case tHidd_Gfx_MemTotal:
                                {
                                    UQUAD value;
                                    nouveau_getparam(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_VRAM_SIZE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemAddressableTotal:
                                {
                                    UQUAD value;
                                    nouveau_getparam(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_GART_SIZE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemFree:
                                {
                                    UQUAD value;
                                    nouveau_getparam(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_VRAM_FREE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                            case tHidd_Gfx_MemAddressableFree:
                                {
                                    UQUAD value;
                                    nouveau_getparam(SD(cl)->carddata.dev, NOUVEAU_GETPARAM_GART_FREE, &value);
                                    matag->ti_Data = (IPTR)value;
                                }
                                break;
                        }
                    }
                }
            }
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

ULONG METHOD(NouveauDisplay, Hidd_Display, ShowViewPorts)
{
    struct pHidd_Compositor_BitMapStackChanged bscmsg =
    {
        .mID  = OOP_GetMethodID(IID_Hidd_Compositor, moHidd_Compositor_BitMapStackChanged),
        .data  = msg->Data
    };

    D(bug("[Nouveau] ShowViewPorts enter TopLevelBM %x\n", (msg->Data ? (msg->Data->Bitmap) : NULL)));

    OOP_DoMethod(SD(cl)->compositor, (OOP_Msg)&bscmsg);

    return TRUE; /* Indicate driver supports this method */
}

#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#endif

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorShape)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    if (msg->shape == NULL)
    {
        /* Hide cursor */
        HIDDNouveauShowCursor(gfx, FALSE);
    }
    else
    {
        IPTR width, height;
        ULONG i;
        ULONG x, y;
        ULONG curimage[64 * 64];
        struct CardData * carddata = &(SD(cl)->carddata);
        
        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);


        if (width > 64) width = 64;
        if (height > 64) height = 64;

        LOCK_ENGINE

        /* Map the cursor buffer */
        nouveau_bo_map(gfxdata->cursor, NOUVEAU_BO_WR, carddata->client);

        /* Clear the matrix */
        for (i = 0; i < 64 * 64; i++)
            ((ULONG*)gfxdata->cursor->map)[i] = 0;

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimage, 64 * 4, 0, 0, 
            width, height, Machine_ARGB32);
        
        if (carddata->Architecture < NV_TESLA)
        {
            ULONG offset, pixel, blue, green, red, alpha;

            /* The image needs to be premultiplied */
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                {
                    offset = y * 64 + x;
                    pixel = curimage[offset];
                    blue  = (pixel & 0x000000FF);
                    green = (pixel & 0x0000FF00) >> 8;
                    red   = (pixel & 0x00FF0000) >> 16;
                    alpha = (pixel & 0xFF000000) >> 24;
                    
                    blue    = (blue * alpha) / 255;
                    green   = (green * alpha) / 255;
                    red     = (red * alpha) / 255;

                    curimage[offset]    = (alpha << 24) | (red << 16) | (green << 8) | blue;
                }
        }

        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                ULONG offset = y * 64 + x;
                writel(curimage[offset], ((ULONG *)gfxdata->cursor->map) + (offset));
            }

        /* Show updated cursor */
        HIDDNouveauShowCursor(gfx, TRUE);

        UNLOCK_ENGINE
    }

    return TRUE;
}

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorPos)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    LOCK_ENGINE
    drmModeMoveCursor(nvdev->fd, gfxdata->selectedcrtcid, msg->x, msg->y);
    UNLOCK_ENGINE

    return TRUE;
}

VOID METHOD(NouveauDisplay, Hidd_Display, SetCursorVisible)
{
    OOP_Object *gfx = NULL;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    HIDDNouveauShowCursor(gfx, msg->visible);
}

static struct HIDD_ModeProperties modeprops = 
{
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE
};

ULONG METHOD(NouveauDisplay, Hidd_Display, ModeProperties)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}

/*
 * What size of screen to open when nothing asks for a particular one.
 *
 * The monitor says which timing it was built for, and that is the one
 * it displays without complaint; anything else it merely tolerates, if
 * at all. Take its dimensions from that mode rather than from a fixed
 * pair, so the default screen is the one the display actually wants.
 */
VOID METHOD(NouveauDisplay, Hidd_Display, NominalDimensions)
{
    OOP_Object *gfx = NULL;
    ULONG width = 1024, height = 768;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    if (gfx)
    {
        struct HIDDNouveauData *gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);
        drmModeConnectorPtr connector = (drmModeConnectorPtr)gfxdata->selectedconnector;

        if (connector && connector->count_modes > 0)
        {
            drmModeModeInfoPtr mode = &connector->modes[0];
            int i;

            for (i = 0; i < connector->count_modes; i++)
            {
                if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED)
                {
                    mode = &connector->modes[i];
                    break;
                }
            }

            /* Nothing marked preferred: the list is ordered best first */
            width = mode->hdisplay;
            height = mode->vdisplay;

            D(bug("[Nouveau] Nominal dimensions %ux%u from '%s'\n",
                  width, height, mode->name));
        }
    }

    if (msg->width)
        *(msg->width) = width;
    if (msg->height)
        *(msg->height) = height;
    if (msg->depth)
        *(msg->depth) = 24;
}
