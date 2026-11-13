/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vchiq.library - the VCHIQ message queue to the VideoCore firmware.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "interface/vchiq_arm/vchiq_core.h"
#include "interface/vchiq_arm/vchiq_arm.h"
#include "vchiq_aros.h"

#include LC_LIBDEFS_FILE

int vchiq_init(void);

static int vchiq_lib_init(struct VCHIQBase *VCHIQBase)
{
    int err;

    D(bug("[VCHIQ] %s()\n", __func__));

    if ((err = vchiq_aros_platform_init()) != 0)
    {
        D(bug("[VCHIQ] no VideoCore to talk to (%d)\n", err));
        return FALSE;
    }

    vchiq_core_initialize();

    /*
     * Stay loaded on failure: the slot threads, the doorbell handler and
     * maybe the firmware already use our code and memory. Clients then
     * get VCHIQ_ERROR from VCHIQInitialise().
     */
    if ((err = vchiq_init()) != 0)
        bug("[VCHIQ] initialisation failed (%d)\n", err);

    return TRUE;
}

/* The firmware keeps the slot memory for good */
static int vchiq_lib_expunge(struct VCHIQBase *VCHIQBase)
{
    return FALSE;
}

ADD2INITLIB(vchiq_lib_init, 0)
ADD2EXPUNGELIB(vchiq_lib_expunge, 0)

/* Messages are a single buffer here */
static ssize_t vchiq_copy_flat(void *context, void *dest, size_t offset,
                               size_t maxsize)
{
    memcpy(dest, (const char *)context + offset, maxsize);
    return maxsize;
}

AROS_LH1(LONG, VCHIQInitialise,
         AROS_LHA(VCHIQ_INSTANCE_T *, pinstance, A0),
         struct VCHIQBase *, VCHIQBase, 5, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_initialise(pinstance);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, VCHIQShutdown,
         AROS_LHA(VCHIQ_INSTANCE_T, instance, A0),
         struct VCHIQBase *, VCHIQBase, 6, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_shutdown(instance);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, VCHIQConnect,
         AROS_LHA(VCHIQ_INSTANCE_T, instance, A0),
         struct VCHIQBase *, VCHIQBase, 7, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_connect(instance);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, VCHIQOpenService,
         AROS_LHA(VCHIQ_INSTANCE_T, instance, A0),
         AROS_LHA(const VCHIQ_SERVICE_PARAMS_T *, params, A1),
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T *, phandle, A2),
         struct VCHIQBase *, VCHIQBase, 8, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_open_service(instance, params, phandle);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, VCHIQCloseService,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         struct VCHIQBase *, VCHIQBase, 9, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_close_service(handle);

    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, VCHIQQueueMessage,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         AROS_LHA(const void *, data, A0),
         AROS_LHA(ULONG, size, D1),
         struct VCHIQBase *, VCHIQBase, 10, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_queue_message(handle, vchiq_copy_flat, (void *)data, size);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, VCHIQReleaseMessage,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         AROS_LHA(VCHIQ_HEADER_T *, header, A0),
         struct VCHIQBase *, VCHIQBase, 11, Vchiq)
{
    AROS_LIBFUNC_INIT

    vchiq_release_message(handle, header);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(LONG, VCHIQBulkTransmit,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         AROS_LHA(const void *, data, A0),
         AROS_LHA(ULONG, size, D1),
         AROS_LHA(APTR, userdata, A1),
         AROS_LHA(ULONG, mode, D2),
         struct VCHIQBase *, VCHIQBase, 12, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_bulk_transmit(handle, (void *)data, size, userdata, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH5(LONG, VCHIQBulkReceive,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         AROS_LHA(APTR, data, A0),
         AROS_LHA(ULONG, size, D1),
         AROS_LHA(APTR, userdata, A1),
         AROS_LHA(ULONG, mode, D2),
         struct VCHIQBase *, VCHIQBase, 13, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_bulk_receive(handle, data, size, userdata, mode);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, VCHIQUseService,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         struct VCHIQBase *, VCHIQBase, 14, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_use_service(handle);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, VCHIQReleaseService,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         struct VCHIQBase *, VCHIQBase, 15, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_release_service(handle);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, VCHIQGetPeerVersion,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         AROS_LHA(WORD *, version, A0),
         struct VCHIQBase *, VCHIQBase, 16, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_get_peer_version(handle, version);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(APTR, VCHIQGetServiceUserdata,
         AROS_LHA(VCHIQ_SERVICE_HANDLE_T, handle, D0),
         struct VCHIQBase *, VCHIQBase, 17, Vchiq)
{
    AROS_LIBFUNC_INIT

    return vchiq_get_service_userdata(handle);

    AROS_LIBFUNC_EXIT
}
