/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test for vchiq.library: connect to the VideoCore and open the MMAL
    service, which proves the slots, the doorbells and the firmware's
    OPEN/OPENACK round trip.

    Exit code 0 on success, 20 on a failure.
*/

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/vchiq.h>

#include <stdio.h>

struct Library *VCHIQBase;

/* raspberrypi/userland interface/mmal/vc/mmal_vc_msgs.h */
#define MMAL_WORKER_VER_MAJOR   16
#define MMAL_WORKER_VER_MINIMUM 10

static volatile int callbacks;

/* Runs on the VCHIQ slot handler task, so no stdio here */
static VCHIQ_STATUS_T callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                               VCHIQ_SERVICE_HANDLE_T service, void *bulk)
{
    callbacks++;
    if (reason == VCHIQ_MESSAGE_AVAILABLE)
        VCHIQReleaseMessage(service, header);

    return VCHIQ_SUCCESS;
}

int main(void)
{
    VCHIQ_INSTANCE_T instance;
    VCHIQ_SERVICE_HANDLE_T service;
    VCHIQ_SERVICE_PARAMS_T params = {
        .fourcc      = VCHIQ_MAKE_FOURCC('m', 'm', 'a', 'l'),
        .callback    = callback,
        .version     = MMAL_WORKER_VER_MAJOR,
        .version_min = MMAL_WORKER_VER_MINIMUM
    };
    WORD peer = 0;
    LONG status;
    int rc = RETURN_FAIL;

    VCHIQBase = OpenLibrary("vchiq.library", 1);
    if (!VCHIQBase)
    {
        printf("FAIL: no vchiq.library (not a Pi 1-4, or no VideoCore)\n");
        return RETURN_FAIL;
    }

    status = VCHIQInitialise(&instance);
    printf("VCHIQInitialise: %ld\n", (long)status);
    if (status != VCHIQ_SUCCESS)
        goto out;

    status = VCHIQConnect(instance);
    printf("VCHIQConnect: %ld\n", (long)status);
    if (status != VCHIQ_SUCCESS)
        goto shutdown;

    status = VCHIQOpenService(instance, &params, &service);
    printf("VCHIQOpenService(mmal): %ld, handle 0x%x\n", (long)status,
           (unsigned)service);
    if (status != VCHIQ_SUCCESS)
        goto shutdown;

    status = VCHIQGetPeerVersion(service, &peer);
    printf("VCHIQGetPeerVersion: %ld, peer version %d\n", (long)status, peer);

    status = VCHIQCloseService(service);
    printf("VCHIQCloseService: %ld\n", (long)status);
    if (status == VCHIQ_SUCCESS)
        rc = RETURN_OK;

shutdown:
    VCHIQShutdown(instance);
out:
    CloseLibrary(VCHIQBase);
    printf("%s\n", rc == RETURN_OK ? "PASS" : "FAIL");

    return rc;
}
