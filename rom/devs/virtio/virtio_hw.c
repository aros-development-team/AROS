/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include <string.h>

#include "virtio_debug.h"
#include "virtio_intern.h"
#include "virtio_hw.h"

/* PCI config space basics */
#define PCI_CAPABILITY_LIST     0x34
#define PCI_STATUS              0x06
#define PCI_STATUS_CAP_LIST     0x10

/*
 * Read a base address out of the PCI device by index, masking off the
 * type bits (we only support memory BARs for modern virtio).
 */
static APTR virtio_get_bar(struct VirtIOBase *VirtIOBase, OOP_Object *pciDev, UBYTE bar)
{
    static const ULONG base_attrs[6] = {
        aoHidd_PCIDevice_Base0,
        aoHidd_PCIDevice_Base1,
        aoHidd_PCIDevice_Base2,
        aoHidd_PCIDevice_Base3,
        aoHidd_PCIDevice_Base4,
        aoHidd_PCIDevice_Base5,
    };
    IPTR base = 0;

    if (bar > 5) return NULL;

    OOP_GetAttr(pciDev, HiddPCIDeviceAttrBase + base_attrs[bar], &base);

    /* Mask off type bits, just in case. */
    return (APTR)(base & ~(IPTR)0xF);
}

BOOL virtio_hw_map(device_t dev)
{
    struct VirtIOBase *VirtIOBase = dev ? dev->dev_VirtIOBase : NULL;
    OOP_Object *pdev = dev->dev_Object;
    UBYTE cap_ptr;
    UBYTE i;

    if (!pdev || !VirtIOBase) return FALSE;

    cap_ptr = HIDD_PCIDevice_ReadConfigByte(pdev, PCI_CAPABILITY_LIST);

    /* Walk the cap chain. */
    for (i = 0; cap_ptr && i < 64; i++) {
        struct virtio_pci_cap cap;
        APTR    bar_base;
        UBYTE   *region;
        UBYTE   ctype;
        UBYTE   cur = cap_ptr;
        UBYTE   id;

        id = HIDD_PCIDevice_ReadConfigByte(pdev, cur + 0);
        cap.cap_next = HIDD_PCIDevice_ReadConfigByte(pdev, cur + 1);

        if (id != VIRTIO_PCI_CAP_VENDOR_CFG) {
            cap_ptr = cap.cap_next;
            continue;
        }

        cap.cap_len  = HIDD_PCIDevice_ReadConfigByte(pdev, cur + 2);
        cap.cfg_type = HIDD_PCIDevice_ReadConfigByte(pdev, cur + 3);
        cap.bar      = HIDD_PCIDevice_ReadConfigByte(pdev, cur + 4);
        cap.offset   = HIDD_PCIDevice_ReadConfigLong(pdev, cur + 8);
        cap.length   = HIDD_PCIDevice_ReadConfigLong(pdev, cur + 12);

        bar_base = virtio_get_bar(VirtIOBase, pdev, cap.bar);
        if (!bar_base) {
            cap_ptr = cap.cap_next;
            continue;
        }
        region = (UBYTE *)bar_base + cap.offset;
        ctype  = cap.cfg_type;

        switch (ctype) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            dev->dev_CommonCfg = (volatile struct virtio_pci_common_cfg *)region;
            D(bug("[VIRTIO:HW] CommonCfg @ %p\n", region);)
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            dev->dev_NotifyBase = region;
            dev->dev_NotifyOffMultiplier = HIDD_PCIDevice_ReadConfigLong(pdev, cur + 16);
            D(bug("[VIRTIO:HW] NotifyBase @ %p (mult %u)\n", region, dev->dev_NotifyOffMultiplier);)
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            dev->dev_ISR = region;
            D(bug("[VIRTIO:HW] ISR @ %p\n", region);)
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            dev->dev_DeviceCfg = region;
            D(bug("[VIRTIO:HW] DeviceCfg @ %p\n", region);)
            break;
        default:
            break;
        }

        cap_ptr = cap.cap_next;
    }

    if (dev->dev_CommonCfg && dev->dev_NotifyBase && dev->dev_ISR && dev->dev_DeviceCfg) {
        dev->dev_IsModern = 1;
        return TRUE;
    }

    D(bug("[VIRTIO:HW] %s: missing one or more required capabilities (legacy device?)\n", __func__);)
    return FALSE;
}

BOOL virtio_hw_reset(device_t dev)
{
    if (!dev->dev_CommonCfg) return FALSE;

    dev->dev_CommonCfg->device_status = 0;
    /* Wait for status to read back as 0 */
    {
        ULONG spin = 1000000;
        while (dev->dev_CommonCfg->device_status != 0 && spin--)
            ;
        if (!spin) return FALSE;
    }

    dev->dev_CommonCfg->device_status = VIRTIO_CONFIG_S_ACKNOWLEDGE;
    dev->dev_CommonCfg->device_status =
        VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER;

    return TRUE;
}

UQUAD virtio_hw_get_device_features(device_t dev)
{
    UQUAD lo, hi;
    if (!dev->dev_CommonCfg) return 0;

    dev->dev_CommonCfg->device_feature_select = 0;
    lo = dev->dev_CommonCfg->device_feature;

    dev->dev_CommonCfg->device_feature_select = 1;
    hi = dev->dev_CommonCfg->device_feature;

    return lo | (hi << 32);
}

void virtio_hw_set_guest_features(device_t dev, UQUAD features)
{
    if (!dev->dev_CommonCfg) return;

    dev->dev_CommonCfg->guest_feature_select = 0;
    dev->dev_CommonCfg->guest_feature        = (ULONG)(features & 0xFFFFFFFFUL);

    dev->dev_CommonCfg->guest_feature_select = 1;
    dev->dev_CommonCfg->guest_feature        = (ULONG)(features >> 32);
}

BOOL virtio_hw_negotiate_features(device_t dev, UQUAD wanted)
{
    UQUAD have;
    UBYTE st;

    if (!dev->dev_CommonCfg) return FALSE;

    have = virtio_hw_get_device_features(dev);
    dev->dev_Features = have & wanted;

    /* VERSION_1 is mandatory for modern devices. */
    if (dev->dev_IsModern) {
        if (!(have & ((UQUAD)1 << VIRTIO_F_VERSION_1))) {
            D(bug("[VIRTIO:HW] %s: device does not advertise VERSION_1\n", __func__);)
            return FALSE;
        }
        dev->dev_Features |= ((UQUAD)1 << VIRTIO_F_VERSION_1);
    }

    virtio_hw_set_guest_features(dev, dev->dev_Features);

    /* Set FEATURES_OK */
    st = dev->dev_CommonCfg->device_status;
    dev->dev_CommonCfg->device_status = st | VIRTIO_CONFIG_S_FEATURES_OK;

    /* Re-read */
    st = dev->dev_CommonCfg->device_status;
    if (!(st & VIRTIO_CONFIG_S_FEATURES_OK)) {
        D(bug("[VIRTIO:HW] %s: device rejected feature negotiation (status %02x)\n", __func__, st);)
        return FALSE;
    }
    return TRUE;
}

BOOL virtio_hw_setup_queue(device_t dev, UWORD qid, struct virtio_queue *vq)
{
    UWORD qsize;
    UWORD notify_off;

    if (!dev->dev_CommonCfg) return FALSE;

    dev->dev_CommonCfg->queue_select = qid;
    qsize = dev->dev_CommonCfg->queue_size;
    if (qsize == 0) {
        D(bug("[VIRTIO:HW] %s: queue %u is not present\n", __func__, qid);)
        return FALSE;
    }
    if (qsize < vq->q_Size) {
        D(bug("[VIRTIO:HW] %s: device caps queue size at %u, requested %u\n",
              __func__, qsize, vq->q_Size);)
        return FALSE;
    }
    /* Tell the device our chosen size */
    dev->dev_CommonCfg->queue_size = vq->q_Size;

    dev->dev_CommonCfg->queue_desc   = vq->q_DescPhys;
    dev->dev_CommonCfg->queue_driver = vq->q_AvailPhys;
    dev->dev_CommonCfg->queue_device = vq->q_UsedPhys;

    notify_off = dev->dev_CommonCfg->queue_notify_off;
    vq->q_Notify = (volatile UWORD *)(dev->dev_NotifyBase +
                                      (ULONG)notify_off * dev->dev_NotifyOffMultiplier);

    /* No MSI-X for first version. */
    dev->dev_CommonCfg->queue_msix_vector = 0xFFFF;

    /* Enable */
    dev->dev_CommonCfg->queue_enable = 1;
    return TRUE;
}

BOOL virtio_hw_driver_ok(device_t dev)
{
    UBYTE st;
    if (!dev->dev_CommonCfg) return FALSE;
    st = dev->dev_CommonCfg->device_status;
    dev->dev_CommonCfg->device_status = st | VIRTIO_CONFIG_S_DRIVER_OK;

    st = dev->dev_CommonCfg->device_status;
    if (st & VIRTIO_CONFIG_S_FAILED) return FALSE;
    return TRUE;
}

void virtio_hw_read_blk_config(device_t dev)
{
    volatile struct virtio_blk_config *cfg;

    if (!dev->dev_DeviceCfg) {
        /* Use defaults */
        dev->dev_Capacity = 0;
        dev->dev_BlkSize  = VIRTIO_BLK_SECTOR_SIZE;
        dev->dev_Heads    = 16;
        dev->dev_Sectors  = 63;
        dev->dev_Cylinders = 0;
        return;
    }

    cfg = (volatile struct virtio_blk_config *)dev->dev_DeviceCfg;
    dev->dev_Capacity = cfg->capacity;     /* in 512-byte sectors */

    if (dev->dev_Features & ((UQUAD)1 << VIRTIO_BLK_F_BLK_SIZE)) {
        dev->dev_BlkSize = cfg->blk_size ? cfg->blk_size : VIRTIO_BLK_SECTOR_SIZE;
    } else {
        dev->dev_BlkSize = VIRTIO_BLK_SECTOR_SIZE;
    }

    if (dev->dev_Features & ((UQUAD)1 << VIRTIO_BLK_F_GEOMETRY)) {
        dev->dev_Cylinders = cfg->geometry.cylinders;
        dev->dev_Heads     = cfg->geometry.heads;
        dev->dev_Sectors   = cfg->geometry.sectors;
    } else {
        dev->dev_Heads     = 16;
        dev->dev_Sectors   = 63;
        dev->dev_Cylinders = 0;
    }

    dev->dev_ReadOnly = (dev->dev_Features & ((UQUAD)1 << VIRTIO_BLK_F_RO)) ? 1 : 0;

    D(bug("[VIRTIO:HW] %s: capacity=%llu sectors, blk_size=%u, geom=%u/%u/%u, RO=%u\n",
          __func__, (unsigned long long)dev->dev_Capacity, dev->dev_BlkSize,
          dev->dev_Cylinders, dev->dev_Heads, dev->dev_Sectors, dev->dev_ReadOnly);)
}
