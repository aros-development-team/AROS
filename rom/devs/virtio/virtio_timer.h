/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 */

#ifndef VIRTIO_TIMER_H
#define VIRTIO_TIMER_H

struct IORequest *virtio_OpenTimer(struct VirtIOBase *base);
void              virtio_CloseTimer(struct IORequest *tmr);
ULONG             virtio_WaitTO(struct IORequest *tmr, ULONG secs, ULONG micro, ULONG sigs);

#endif