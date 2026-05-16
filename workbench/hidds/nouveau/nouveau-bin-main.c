#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <drm-aros/drm_aros_pci.h>
#include <drm-compat/drm_compat_types.h>
#include <drm/drm_drv.h>

#include "libdrm/arosdrmmode.h"
#include "libdrm/nouveau/nouveau_bo.h"
#include "libdrm/nouveau/nouveau_drmif.h"

APTR NouveauMemPool;
struct drm_device *current_drm_device;

APTR HIDDNouveauAlloc(ULONG size)
{
    return AllocVecPooled(NouveauMemPool, size);
}

VOID HIDDNouveauFree(APTR memory)
{
    FreeVecPooled(NouveauMemPool, memory);
}

int nouveau_drm_probe(struct pci_dev *pdev, const struct pci_device_id *pent, struct drm_device **pdrm_dev);

void main()
{
    NouveauMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);

    OpenLibrary("DEVS:Drivers/pcimock.hidd", 0L);

    drm_aros_pci_init();

    struct pci_dev *pdev = drm_aros_pci_find_supported_video_card();

    nouveau_drm_probe(pdev, NULL, &current_drm_device);

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

    // bug("Switching mode to 640x480\n");



    // drmModeSetCrtc(0, 0, 0, 0 , 0, NULL, 0, NULL);

_sleep:
    bug("Sleeping\n");
    while(1)
        Delay(50);
}

ADD2LIBS("pci.hidd", 0, static struct Library *, PciHiddBase);
