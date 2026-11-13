/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libmmal: the client side of raspberrypi/userland's MMAL API, on
    libvcmmal. Only the VideoCore ("vc.") components exist.
*/

#ifndef LIBMMAL_PRIVATE_H
#define LIBMMAL_PRIVATE_H

#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/vc/mmal_vc_api.h>

/* After the MMAL headers, which then provide the shared names */
#include <hardware/vcmmal.h>

#include <exec/semaphores.h>

#define LIBMMAL_EXTRADATA_MAX   128

struct MMAL_BUFFER_HEADER_PRIVATE_T {
    MMAL_BUFFER_HEADER_T *header;
    volatile LONG refcount;
    void (*pf_release)(MMAL_BUFFER_HEADER_T *header);
    void *owner;
    MMAL_BH_PRE_RELEASE_CB_T pf_pre_release;
    void *pre_release_userdata;
    MMAL_BUFFER_HEADER_TYPE_SPECIFIC_T type;
    /* What libvcmmal sends; it is handed back in the buffer callback */
    struct VCMMALBuffer vb;
};

struct MMAL_QUEUE_T {
    struct SignalSemaphore lock;
    MMAL_BUFFER_HEADER_T *first;
    MMAL_BUFFER_HEADER_T **last;
    unsigned int length;
    struct Task *waiter;
    ULONG waitmask;
};

struct MMAL_PORT_PRIVATE_T {
    struct VCMMALPort *vp;
    MMAL_PORT_BH_CB_T cb;
    BOOL vc_enabled;        /* control port: enabled in the VideoCore for good */
    volatile LONG inflight;
    struct Task *idle_waiter;
    ULONG idle_mask;
    MMAL_ES_FORMAT_T format;
    MMAL_ES_SPECIFIC_FORMAT_T es;
    uint8_t extradata[LIBMMAL_EXTRADATA_MAX];
    char name[48];
};

struct MMAL_COMPONENT_PRIVATE_T {
    struct VCMMALComponent *vc;
    MMAL_POOL_T *event_pool;
};

/* mmal_vc.c */
struct VCMMAL *libmmal_vc_client(void);

/* mmal_core.c: wait for any of sigmask, at most ms milliseconds */
ULONG libmmal_wait(ULONG sigmask, ULONG ms);

#endif /* LIBMMAL_PRIVATE_H */
