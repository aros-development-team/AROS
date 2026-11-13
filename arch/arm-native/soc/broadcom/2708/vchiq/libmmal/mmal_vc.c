/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libmmal vc_client: one libvcmmal connection, shared by every component
    of the program and opened by the first mmal_vc_init().
*/

#include <proto/exec.h>

#include "libmmal_private.h"

static struct SignalSemaphore vc_lock;
static BOOL vc_lock_ready;
static struct VCMMAL *vc_client;
static ULONG vc_users;

static void vc_lock_init(void)
{
    Forbid();
    if (!vc_lock_ready)
    {
        InitSemaphore(&vc_lock);
        vc_lock_ready = TRUE;
    }
    Permit();
}

MMAL_STATUS_T mmal_vc_init(void)
{
    MMAL_STATUS_T st = MMAL_SUCCESS;

    vc_lock_init();
    ObtainSemaphore(&vc_lock);
    if (!vc_users && !(vc_client = VCMMAL_Open()))
        st = MMAL_ENXIO;
    else
        vc_users++;
    ReleaseSemaphore(&vc_lock);

    return st;
}

MMAL_STATUS_T mmal_vc_init_fd(int dev_vchiq_fd)
{
    return mmal_vc_init();
}

void mmal_vc_deinit(void)
{
    vc_lock_init();
    ObtainSemaphore(&vc_lock);
    if (vc_users && --vc_users == 0)
    {
        VCMMAL_Close(vc_client);
        vc_client = NULL;
    }
    ReleaseSemaphore(&vc_lock);
}

struct VCMMAL *libmmal_vc_client(void)
{
    return vc_client;
}

MMAL_STATUS_T mmal_vc_use(void)
{
    return MMAL_SUCCESS;
}

MMAL_STATUS_T mmal_vc_release(void)
{
    return MMAL_SUCCESS;
}

MMAL_STATUS_T mmal_vc_get_version(uint32_t *major, uint32_t *minor, uint32_t *minimum)
{
    ULONG ma = 0, mi = 0;
    MMAL_STATUS_T st;

    if (mmal_vc_init() != MMAL_SUCCESS)
        return MMAL_ENXIO;
    st = VCMMAL_GetVersion(vc_client, &ma, &mi);
    mmal_vc_deinit();

    *major = ma;
    *minor = mi;
    *minimum = 0;
    return st;
}
