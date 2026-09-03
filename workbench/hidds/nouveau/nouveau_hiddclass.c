/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <exec/pm.h>
#include <proto/exec.h>
#include "nouveau_intern.h"
#include "compositor.h"

#include <graphics/displayinfo.h>
#include <graphics/driver.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include <libdrm/arosdrm.h>
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
        nvlog("[Nouveau] Not able to get resources information\n");
        UNLOCK_ENGINE
        return FALSE;
    }
    
    /* Selecting connector */
    for (i = 0; i < drmmode->count_connectors; i++)
    {
        drmModeConnectorPtr connector = drmModeGetConnector(fd, drmmode->connectors[i]);

        if (connector)
        {
            nvlog("[Nouveau] connector %u type %u status %u modes %d\n", connector->connector_id,
                connector->connector_type, connector->connection, connector->count_modes);
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                /* Found connected connector */
                *selectedconnector = connector;
                break;
            }
            
            drmModeFreeConnector(connector);
        }
        else
            nvlog("[Nouveau] connector %u: no information\n", drmmode->connectors[i]);
    }
    
    if (!(*selectedconnector))
    {
        nvlog("[Nouveau] no connected connector (%d connectors, %d crtcs)\n", drmmode->count_connectors, drmmode->count_crtcs);
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
        nvlog("[Nouveau] Not able to get crtc information for crtc_id %d\n", crtc_id);
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
        
    /* One entry per mode plus the terminator: the list is consumed
       via TAG_MORE, so it must end in TAG_DONE. */
    syncs = HIDDNouveauAlloc(sizeof(struct TagItem) * (modescount + 1));
    
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

    syncs[modescount].ti_Tag = TAG_DONE;
    syncs[modescount].ti_Data = 0;

    return syncs;
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
bool nouveau_aros_boot_display;

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
            bug("[Nouveau] BAR%lu (0x%p) has no CPU address, cannot match it\n",
                  (unsigned long)bar, (APTR)(IPTR)start);
            continue;
        }

        ranges[count].dr_Base = cpu;
        ranges[count].dr_Size = len;
        bug("[Nouveau] BAR%lu occupies 0x%p, %lu bytes\n",
              (unsigned long)bar, cpu, len);
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
        nouveau_aros_boot_display = TRUE;
        if (!handover->dho_ExpungeDisplay(handover->dho_Context, handle))
        {
            nvlog("[Nouveau] boot display 0x%p shares this card and is still in"
                " use - not taking the card over\n", handle);
            return FALSE;
        }

        bug("[Nouveau] boot display 0x%p released this card\n", handle);
    }

    bug("[Nouveau] handover: done (evicted %s)\n",
        nouveau_aros_boot_display ? "boot display(s)" : "nothing");
    return TRUE;
}

/*
 * Just before the platform reset performer runs (EFI reset sits at
 * priority -56), shut the driver down the way every other port of this
 * stack does on module unload: stop display work, then tell GSP-RM the
 * driver is going away and let it halt, tearing its protected region
 * down so the next boot's GSP-FMC starts cleanly. A power-off skips all
 * of it: the card needs no unload across a power cycle, and a machine
 * without a power-off mechanism still needs the driver alive to render
 * intuition's final screen.
 */
volatile int nouveau_shutting_down;

static struct CardData *nouveau_shutdown_carddata;

static AROS_INTH1(HIDDNouveauShutdownHandler, struct Interrupt *, handler)
{
    AROS_INTFUNC_INIT

    UBYTE action = handler->is_Node.ln_Type & SD_ACTION_MASK;

    /* Bitwise: covers cold, warm and the combined SD_ACTION_REBOOT */
    if (action & SD_ACTION_REBOOT)
    {
        nouveau_shutting_down = 1;
        if (nouveau_shutdown_carddata)
        {
            bug("[nouveau] shutting down: draining and freeing channels\n");
            HIDDNouveauAccelShutdown(nouveau_shutdown_carddata);
        }
        nouveau_shutdown();
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

static struct Interrupt nouveau_shutdown_interrupt;

static void HIDDNouveauInstallShutdownHandler(struct CardData *carddata)
{
    nouveau_shutdown_carddata = carddata;
    if (nouveau_shutdown_interrupt.is_Code)
        return;

    nouveau_shutdown_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    nouveau_shutdown_interrupt.is_Node.ln_Pri  = -48;
    nouveau_shutdown_interrupt.is_Node.ln_Name = "nouveau.hidd";
    nouveau_shutdown_interrupt.is_Code         = (VOID_FUNC)HIDDNouveauShutdownHandler;
    nouveau_shutdown_interrupt.is_Data         = &nouveau_shutdown_interrupt;
    AddResetCallback(&nouveau_shutdown_interrupt);
}

/* PUBLIC METHODS */
/* DRM connector type -> vHidd_ConnectorType_* (0 = unknown) */
static ULONG HIDDNouveauConnectorType(uint32_t drmtype)
{
    switch (drmtype)
    {
    case DRM_MODE_CONNECTOR_VGA:         return vHidd_ConnectorType_VGA;
    case DRM_MODE_CONNECTOR_DVII:
    case DRM_MODE_CONNECTOR_DVID:
    case DRM_MODE_CONNECTOR_DVIA:        return vHidd_ConnectorType_DVI;
    case DRM_MODE_CONNECTOR_HDMIA:
    case DRM_MODE_CONNECTOR_HDMIB:       return vHidd_ConnectorType_HDMI;
    case DRM_MODE_CONNECTOR_DisplayPort: return vHidd_ConnectorType_DisplayPort;
    case DRM_MODE_CONNECTOR_eDP:         return vHidd_ConnectorType_eDP;
    case DRM_MODE_CONNECTOR_LVDS:        return vHidd_ConnectorType_LVDS;
    case DRM_MODE_CONNECTOR_Composite:
    case DRM_MODE_CONNECTOR_SVIDEO:
    case DRM_MODE_CONNECTOR_Component:
    case DRM_MODE_CONNECTOR_9PinDIN:
    case DRM_MODE_CONNECTOR_TV:          return vHidd_ConnectorType_TV;
    case DRM_MODE_CONNECTOR_DSI:         return vHidd_ConnectorType_DSI;
    case DRM_MODE_CONNECTOR_VIRTUAL:     return vHidd_ConnectorType_Virtual;
    case DRM_MODE_CONNECTOR_USB:         return vHidd_ConnectorType_USBC;
    default:                             return vHidd_ConnectorType_Unknown;
    }
}

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

    HIDDNouveauInstallShutdownHandler(&(SD(cl)->carddata));

    LOCK_ENGINE

    {
        struct nouveau_drm *nvdrm = NULL;
        int drmfd = drmOpen("nouveau", "");

        if (drmfd < 0 || nouveau_drm_new(drmfd, &nvdrm) || nouveau_device_new(&nvdrm->client, &nvdev))
        {
            nvlog("[Nouveau] Not able to open the drm device\n");
            if (nvdrm)
                nouveau_drm_del(&nvdrm);
            if (drmfd >= 0)
                drmClose(drmfd);
            UNLOCK_ENGINE
            return NULL;
        }
    }

    nouveau_client_new(nvdev, &nvclient);


    /* Select crtc and connector */
    if (!HIDDNouveauSelectConnectorCrtc(NOUVEAU_DEV_FD(nvdev), &selectedconnector, &selectedcrtc))
    {
        nvlog("[Nouveau] Not able to select connector and crtc\n");

        UNLOCK_ENGINE

        return NULL;
    }
    
    selectedcrtcid = selectedcrtc->crtc_id;
    drmModeFreeCrtc(selectedcrtc);

    /* Read connector and build sync tags */
    syncs = HIDDNouveauCreateSyncTagsFromConnector(cl, selectedconnector);
    if (syncs == NULL)
    {
        nvlog("[Nouveau] Not able to read any sync modes\n");
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

        /* Name the adapter after the chip that was actually found */
        {
            char chip[24];
            const char *family;

            switch (nvdev->chipset & 0xff0)
            {
            case 0x050: case 0x080: case 0x090: case 0x0a0:
                family = "Tesla"; break;
            case 0x0c0: case 0x0d0:
                family = "Fermi"; break;
            case 0x0e0: case 0x0f0: case 0x100:
                family = "Kepler"; break;
            case 0x110: case 0x120:
                family = "Maxwell"; break;
            case 0x130:
                family = "Pascal"; break;
            case 0x140:
                family = "Volta"; break;
            case 0x160:
                family = "Turing"; break;
            case 0x170:
                family = "Ampere"; break;
            case 0x180:
                family = "Hopper"; break;
            case 0x190:
                family = "Ada"; break;
            case 0x1a0: case 0x1b0:
                family = "Blackwell"; break;
            default:
                family = NULL; break;
            }

            if (!drmGetChipName(NOUVEAU_DEV_FD(nvdev), chip, sizeof(chip)))
                sprintf(chip, "NV%X", (unsigned)nvdev->chipset);

            if (family)
                snprintf(SD(cl)->hardwarename, sizeof(SD(cl)->hardwarename),
                         "NVIDIA %s (%s) Gfx Adaptor", chip, family);
            else
                snprintf(SD(cl)->hardwarename, sizeof(SD(cl)->hardwarename),
                         "NVIDIA %s Gfx Adaptor", chip);
        }

        struct TagItem mytags[] = {
            { aHidd_Name            , (IPTR)"Nouveau"     },
            { aHidd_HardwareName    , (IPTR)SD(cl)->hardwarename },
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
                    { aHidd_Display_GfxHidd,       (IPTR)o        },
                    { aHidd_Display_ModeTags,      (IPTR)modetags },
                    { aHidd_Display_ConnectorType,
                      HIDDNouveauConnectorType(selectedconnector->connector_type) },
                    { aHidd_Display_ConnectorID,   selectedconnector->connector_type_id },
                    { TAG_DONE,                    0              }
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
            case 0x140:
                carddata->Architecture = NV_VOLTA;
                break;
            case 0x160:
                carddata->Architecture = NV_TURING;
                break;
            case 0x170:
                carddata->Architecture = NV_AMPERE;
                break;
            case 0x180:
                carddata->Architecture = NV_HOPPER;
                break;
            case 0x190:
                carddata->Architecture = NV_ADA;
                break;
            case 0x1a0:
            case 0x1b0:
                carddata->Architecture = NV_BLACKWELL;
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
        
            if (!HIDDNouveauAccelCommonInit(carddata))
                nvlog("[Nouveau] acceleration setup failed, running unaccelerated\n");

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
            if (carddata->channel) switch(carddata->Architecture)
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
            case(NV_VOLTA):
            case(NV_TURING):
            case(NV_AMPERE):
            case(NV_HOPPER):
            case(NV_ADA):
            case(NV_BLACKWELL):
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
        case(NV_VOLTA):
        case(NV_TURING):
        case(NV_AMPERE):
        case(NV_HOPPER):
        case(NV_ADA):
        case(NV_BLACKWELL):
            ret = HIDDNouveauNVC0CopySameFormat(carddata, srcdata, destdata, 
                        msg->srcX, msg->srcY, msg->destX, msg->destY, 
                        msg->width, msg->height, GC_DRMD(msg->gc));
            break;
        }


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

