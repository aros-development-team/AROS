#ifndef HARDWARE_VIRTIO_H
#define HARDWARE_VIRTIO_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: VirtIO 1.x PCI common register & virtqueue definitions
*/

#include <exec/types.h>

/* PCI vendor ID for VirtIO devices (Red Hat / IBM range) */
#define VIRTIO_PCI_VENDOR_ID            0x1AF4

/* Modern (virtio 1.0+) device IDs are vendor 0x1AF4, device 0x1040..0x107F */
#define VIRTIO_PCI_DEVICEID_MIN         0x1040
#define VIRTIO_PCI_DEVICEID_MAX         0x107F
#define VIRTIO_PCI_DEVICEID_BLK_MODERN  0x1042  /* virtio-blk modern */

/* Legacy / transitional virtio-blk */
#define VIRTIO_PCI_DEVICEID_BLK_LEGACY  0x1001

#define VIRTIO_DEVICE_BLOCK             2

/* PCI Configuration capabilities, see virtio 1.x spec, sec 4.1.4 */
#define VIRTIO_PCI_CAP_VENDOR_CFG       0x09
#define VIRTIO_PCI_CAP_COMMON_CFG       1
#define VIRTIO_PCI_CAP_NOTIFY_CFG       2
#define VIRTIO_PCI_CAP_ISR_CFG          3
#define VIRTIO_PCI_CAP_DEVICE_CFG       4
#define VIRTIO_PCI_CAP_PCI_CFG          5

/* PCI capability layout */
struct virtio_pci_cap {
    UBYTE   cap_vndr;       /* generic PCI field: 0x09 */
    UBYTE   cap_next;       /* generic PCI field: next ptr. */
    UBYTE   cap_len;        /* length of this cap structure */
    UBYTE   cfg_type;       /* identifies the structure */
    UBYTE   bar;            /* which BAR contains the structure */
    UBYTE   padding[3];
    ULONG   offset;         /* offset within bar */
    ULONG   length;         /* length within bar */
};

/* Notify capability extends virtio_pci_cap with a multiplier */
struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    ULONG   notify_off_multiplier;
};

/* Common configuration register layout (modern virtio) */
struct virtio_pci_common_cfg {
    /* About the whole device */
    ULONG   device_feature_select;  /* 0x00 RW */
    ULONG   device_feature;         /* 0x04 RO */
    ULONG   guest_feature_select;   /* 0x08 RW */
    ULONG   guest_feature;          /* 0x0C RW */
    UWORD   msix_config;            /* 0x10 RW */
    UWORD   num_queues;             /* 0x12 RO */
    UBYTE   device_status;          /* 0x14 RW */
    UBYTE   config_generation;      /* 0x15 RO */

    /* About a specific virtqueue */
    UWORD   queue_select;           /* 0x16 RW */
    UWORD   queue_size;             /* 0x18 RW power-of-2 */
    UWORD   queue_msix_vector;      /* 0x1A RW */
    UWORD   queue_enable;           /* 0x1C RW */
    UWORD   queue_notify_off;       /* 0x1E RO */
    UQUAD   queue_desc;             /* 0x20 RW */
    UQUAD   queue_driver;           /* 0x28 RW (avail) */
    UQUAD   queue_device;           /* 0x30 RW (used) */
};

/* Device status bits */
#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FEATURES_OK     8
#define VIRTIO_CONFIG_S_NEEDS_RESET     0x40
#define VIRTIO_CONFIG_S_FAILED          0x80

/* Reserved (transport-level) feature bits */
#define VIRTIO_F_NOTIFY_ON_EMPTY        24
#define VIRTIO_F_ANY_LAYOUT             27
#define VIRTIO_F_RING_INDIRECT_DESC     28
#define VIRTIO_F_RING_EVENT_IDX         29
#define VIRTIO_F_VERSION_1              32
#define VIRTIO_F_ACCESS_PLATFORM        33
#define VIRTIO_F_RING_PACKED            34

/* virtio-blk feature bits (subset) */
#define VIRTIO_BLK_F_SIZE_MAX           1
#define VIRTIO_BLK_F_SEG_MAX            2
#define VIRTIO_BLK_F_GEOMETRY           4
#define VIRTIO_BLK_F_RO                 5
#define VIRTIO_BLK_F_BLK_SIZE           6
#define VIRTIO_BLK_F_FLUSH              9
#define VIRTIO_BLK_F_TOPOLOGY           10
#define VIRTIO_BLK_F_CONFIG_WCE         11
#define VIRTIO_BLK_F_DISCARD            13
#define VIRTIO_BLK_F_WRITE_ZEROES       14

/* Default sector size used by virtio-blk (independent of blk_size feature) */
#define VIRTIO_BLK_SECTOR_SIZE          512

/* virtio-blk command request header (sent on out part of descriptor chain) */
struct virtio_blk_outhdr {
    ULONG   type;
    ULONG   ioprio;
    UQUAD   sector;     /* 512-byte units */
};

/* virtio-blk request types */
#define VIRTIO_BLK_T_IN                 0
#define VIRTIO_BLK_T_OUT                1
#define VIRTIO_BLK_T_FLUSH              4
#define VIRTIO_BLK_T_GET_ID             8
#define VIRTIO_BLK_T_DISCARD            11
#define VIRTIO_BLK_T_WRITE_ZEROES       13

/* virtio-blk status codes (one byte at end of descriptor chain) */
#define VIRTIO_BLK_S_OK                 0
#define VIRTIO_BLK_S_IOERR              1
#define VIRTIO_BLK_S_UNSUPP             2

/* virtio-blk device-config layout (read from device cfg BAR) */
struct virtio_blk_config {
    UQUAD   capacity;       /* in 512-byte sectors */
    ULONG   size_max;
    ULONG   seg_max;
    struct {
        UWORD cylinders;
        UBYTE heads;
        UBYTE sectors;
    } geometry;
    ULONG   blk_size;
    /* topology (optional) */
    UBYTE   physical_block_exp;
    UBYTE   alignment_offset;
    UWORD   min_io_size;
    ULONG   opt_io_size;
    UBYTE   wce;
    UBYTE   unused0;
    UWORD   num_queues;
};

/*
 * Split virtqueue layout (virtio 1.x)
 *
 * Each virtqueue has three contiguous parts:
 *   - descriptor table  (16 bytes per descriptor)
 *   - available ring    (variable)
 *   - used ring         (variable)
 * They must be physically contiguous and aligned per spec.
 */

struct virtq_desc {
    UQUAD   addr;       /* guest physical / DMA address */
    ULONG   len;
    UWORD   flags;
    UWORD   next;
};

#define VIRTQ_DESC_F_NEXT       1   /* Marks a buffer as continuing */
#define VIRTQ_DESC_F_WRITE      2   /* Marks a buffer as device-write-only */
#define VIRTQ_DESC_F_INDIRECT   4

struct virtq_avail {
    UWORD   flags;
    UWORD   idx;
    UWORD   ring[];     /* queue_size entries */
    /* UWORD used_event;  -- if VIRTIO_F_RING_EVENT_IDX */
};

#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

struct virtq_used_elem {
    ULONG   id;         /* index of start of used descriptor chain */
    ULONG   len;        /* total length of the descriptor chain written */
};

struct virtq_used {
    UWORD                   flags;
    UWORD                   idx;
    struct virtq_used_elem  ring[];     /* queue_size entries */
    /* UWORD avail_event;  -- if VIRTIO_F_RING_EVENT_IDX */
};

#define VIRTQ_USED_F_NO_NOTIFY  1

#endif /* HARDWARE_VIRTIO_H */