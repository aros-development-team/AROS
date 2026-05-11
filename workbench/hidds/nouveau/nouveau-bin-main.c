#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <drm-compat/drm_compat_types.h>
#include <drm/drm_drv.h>

OOP_AttrBase HiddPCIDeviceAttrBase  = 0;

APTR NouveauMemPool;

APTR HIDDNouveauAlloc(ULONG size)
{
    return AllocVecPooled(NouveauMemPool, size);
}

VOID HIDDNouveauFree(APTR memory)
{
    FreeVecPooled(NouveauMemPool, memory);
}

void main()
{
    NouveauMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);
    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    struct drm_device drm_dev;
    nouveau_drm_device_init(&drm_dev);
}
