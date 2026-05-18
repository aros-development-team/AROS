#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <drm-aros/drm_aros_pci.h>
#include <drm-compat/drm_compat_types.h>
#include <drm/drm_drv.h>

#include <libdrm/arosdrmmode.h>
#include "../libdrm/nouveau/nouveau_bo.h"
#include "../libdrm/nouveau/nouveau_drmif.h"

APTR NouveauMemPool;

APTR HIDDNouveauAlloc(ULONG size)
{
    return AllocVecPooled(NouveauMemPool, size);
}

VOID HIDDNouveauFree(APTR memory)
{
    FreeVecPooled(NouveauMemPool, memory);
}

int nouveau_init();

static BOOL HIDDNouveauSelectConnectorCrtc(LONG fd, drmModeConnectorPtr * selectedconnector,
    drmModeCrtcPtr * selectedcrtc)
{
    *selectedconnector = NULL;
    *selectedcrtc = NULL;
    drmModeResPtr drmmode = NULL;
    LONG i; ULONG crtc_id;



    /* Get all components information */
    drmmode = drmModeGetResources(fd);
    if (!drmmode)
    {
        D(bug("[Nouveau] Not able to get resources information\n"));

        return FALSE;
    }

    /* Selecting connector */
    // for (i = 0; i < drmmode->count_connectors; i++)
    // {
        drmModeConnectorPtr connector = drmModeGetConnector(fd, 1);

    //     if (connector)
    //     {
    //         if (connector->connection == DRM_MODE_CONNECTED)
    //         {
    //             /* Found connected connector */
    //             *selectedconnector = connector;
    //             break;
    //         }

    //         drmModeFreeConnector(connector);
    //     }
    // }

    // if (!(*selectedconnector))
    // {
    //     D(bug("[Nouveau] No connected connector\n"));
    //     drmModeFreeResources(drmmode);

    //     return FALSE;
    // }

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

        return FALSE;
    }

    drmModeFreeResources(drmmode);

    return TRUE;
}

void main()
{
    NouveauMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);

    OpenLibrary("DEVS:Drivers/pcimock.hidd", 0L);

    nouveau_init();

    bug("FINISHED nouveau_drm_probe\n");


    struct nouveau_device *dev = NULL;
    struct nouveau_bo *bo = NULL;
    int ret;

    bug("---- Opening nouveau device\n");

    ret = nouveau_device_open(&dev, "");
    if (ret)
    {
        bug("Failed nouveau_device_open: %d\n", ret);
        goto _sleep;
    }

    bug("---- Allocating FB bitmap\n");

    ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 640 * 4 * 480, &bo);
    if (ret)
    {
        bug("Failed nouveau_bo_new: %d\n", ret);
        goto _sleep;
    }

    bug("---- Mapping FB bitmap\n");
    ret = nouveau_bo_map(bo, NOUVEAU_BO_RDWR);
    if (ret)
    {
        bug("Failed nouveau_bo_map: %d\n", ret);
        goto _sleep;
    }

    int fbid = 0;
    struct nouveau_device_priv *nvdev = nouveau_device(dev);

    bug("---- Adding FB bitmap\n");
    ret = drmModeAddFB(nvdev->fd, 640, 480,
                    24, 4 * 8,
                    640 * 4, bo->handle, &fbid);
    if (ret)
    {
        bug("Failed drmModeAddFB: %d\n", ret);
        goto _sleep;
    }

    bug("---- Selecting CRTS/Connector\n");

    drmModeCrtcPtr selectedcrtc = NULL;
    drmModeConnectorPtr selectedconnector = NULL;

    HIDDNouveauSelectConnectorCrtc(nvdev->fd, &selectedconnector, &selectedcrtc);

    bug("Switching mode to 640x480\n");
    uint32_t aa[] = {1};
    drmModeSetCrtc(nvdev->fd, 1, fbid, 0, 0, aa, 1, NULL);

    drmModeSetCursor(nvdev->fd, 1, 0, 64, 64);

_sleep:
    bug("Sleeping\n");
    while(1)
        Delay(50);
}

ADD2LIBS("pci.hidd", 0, static struct Library *, PciHiddBase);
