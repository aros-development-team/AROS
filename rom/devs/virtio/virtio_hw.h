/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 */

#ifndef VIRTIO_HW_H
#define VIRTIO_HW_H

/*
 * Walk the device's PCI capability list and map the four "modern" virtio-pci
 * windows (common cfg, notify cfg, ISR cfg, device cfg) into dev->dev_*Cfg.
 * Returns TRUE on success.
 */
BOOL virtio_hw_map(device_t dev);

/* Reset the device, walk through ACK -> DRIVER. */
BOOL virtio_hw_reset(device_t dev);

/* Read 64 bits of device features (using device_feature_select). */
UQUAD virtio_hw_get_device_features(device_t dev);

/* Write the negotiated 64-bit guest features. */
void  virtio_hw_set_guest_features(device_t dev, UQUAD features);

/* Negotiate features. Returns TRUE on success. */
BOOL  virtio_hw_negotiate_features(device_t dev, UQUAD wanted);

/* Configure a single queue at index qid; vq must already be allocated. */
BOOL  virtio_hw_setup_queue(device_t dev, UWORD qid, struct virtio_queue *vq);

/* Switch device to DRIVER_OK. */
BOOL  virtio_hw_driver_ok(device_t dev);

/* Read virtio-blk device cfg into dev cached fields. */
void  virtio_hw_read_blk_config(device_t dev);

#endif