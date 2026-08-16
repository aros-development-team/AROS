/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/kernel.h>

#include "nouveau_drv.h"
#include "nouveau_bo.h"
#include "nouveau_gem.h"
#include "nouveau_uvmm.h"
#include "nouveau_sched.h"
#include "nouveau_exec.h"
#include "nouveau_hwmon.h"

#include <core/tegra.h>
#include "nvkm/engine/device/acpi.h"

/*
 * Parts of the driver that do not exist on this platform: the VM_BIND /
 * EXEC uAPI with its scheduler and GPU-VA manager, buffer sharing, hwmon,
 * ACPI and the Tegra platform device. Each returns "not there" so the
 * abi16 channel path, which is what libdrm uses, is all that remains.
 */

int nouveau_sched_create(struct nouveau_sched **psched, struct nouveau_drm *drm,
    struct workqueue_struct *wq, u32 credit_limit)
{
    *psched = NULL;
    return 0;
}

void nouveau_sched_destroy(struct nouveau_sched **psched)
{
    *psched = NULL;
}

int nouveau_uvmm_ioctl_vm_init(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
    return -ENOSYS;
}

int nouveau_uvmm_ioctl_vm_bind(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
    return -ENOSYS;
}

void nouveau_uvmm_fini(struct nouveau_uvmm *uvmm)
{
}

void nouveau_uvmm_bo_map_all(struct nouveau_bo *nvbo, struct nouveau_mem *mem)
{
}

void nouveau_uvmm_bo_unmap_all(struct nouveau_bo *nvbo)
{
}

int nouveau_exec_ioctl_exec(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
    return -ENOSYS;
}

struct sg_table *nouveau_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
    return ERR_PTR(-ENOSYS);
}

struct drm_gem_object *nouveau_gem_prime_import_sg_table(struct drm_device *dev,
    struct dma_buf_attachment *attach, struct sg_table *sg)
{
    return ERR_PTR(-ENOSYS);
}

struct dma_buf *nouveau_gem_prime_export(struct drm_gem_object *gobj, int flags)
{
    return ERR_PTR(-ENOSYS);
}

int nouveau_gem_prime_pin(struct drm_gem_object *obj)
{
    return -ENOSYS;
}

void nouveau_gem_prime_unpin(struct drm_gem_object *obj)
{
}

int nouveau_hwmon_init(struct drm_device *dev)
{
    return 0;
}

void nouveau_hwmon_fini(struct drm_device *dev)
{
}

void nvkm_acpi_init(struct nvkm_device *device)
{
}

void nvkm_acpi_fini(struct nvkm_device *device)
{
}

int nvkm_device_tegra_new(const struct nvkm_device_tegra_func *func, struct platform_device *pdev,
    const char *cfg, const char *dbg, struct nvkm_device **pdevice)
{
    return -ENODEV;
}
