#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <drm-compat/drm_compat_types.h>
#include <drm-compat/drm_compat_pci.h>
#include <drm/drm_drv.h>

#include "drm-aros/drm_aros_pci.h"

APTR NouveauMemPool;

APTR HIDDNouveauAlloc(ULONG size)
{
    return AllocVecPooled(NouveauMemPool, size);
}

VOID HIDDNouveauFree(APTR memory)
{
    FreeVecPooled(NouveauMemPool, memory);
}

int nouveau_drm_probe(struct pci_dev *pdev, const struct pci_device_id *pent);

void main()
{
    NouveauMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);

    OpenLibrary("DEVS:Drivers/pcimock.hidd", 0L);

    drm_aros_pci_init();

    struct pci_dev *pdev = drm_aros_pci_find_supported_video_card();

    nouveau_drm_probe(pdev, NULL);

    bug("FINISHED nouveau_drm_probe\n");

    while(1)
        Delay(50);
}

ADD2LIBS("pci.hidd", 0, static struct Library *, PciHiddBase);
