/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 */

#ifndef VIRTIO_QUEUE_H
#define VIRTIO_QUEUE_H

struct virtio_queue *virtio_alloc_queue(device_t dev, int qid, int size);
void                 virtio_free_queue(struct virtio_queue *vq);

/* Drain used ring; returns number of completions handled. */
ULONG                virtio_process_used(struct virtio_queue *vq);

/*
 * Submit a virtio-blk style request: out-header + (data) + status byte.
 * Returns desc head on success, -1 on failure.
 * The supplied virtio_request structure is copied into the per-slot tracking
 * array; the caller need not keep it alive after submission.
 */
int virtio_submit_blk_request(struct virtio_queue *vq,
                              struct virtio_request *templ);

#endif