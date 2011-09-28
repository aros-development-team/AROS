/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in Android GUI if possible
    Lang: english
*/

#include <aros/atomic.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <signal.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_android.h"

#define D(x)

struct AlertRequest
{
    ULONG cmd;
    ULONG params;
    ULONG code;
    ULONG text;
};

/* This comes from kernel_startup.c */
extern struct HostInterface *HostIFace;

int alertPipe = -1;

int SendAlert(uint32_t code, const char *text)
{
    struct AlertRequest req;
    int res;

    /* Prepare a message to server */    
    req.cmd    = 0x00001000;	/* cmd_Alert				   */
    req.params = 2;		/* Two parameters: code and string address */
    req.code   = code;
    req.text   = (IPTR)text;

    /* Send the packet */
    res = KernelIFace.write(alertPipe, &req, sizeof(req));

    /* Return zero on pipe error */
    return (res == sizeof(req));
}

/* This attaches to display server pipe */
static int Alert_Init(void)
{
    APTR libHandle;
    int *ptr;
    char *err;

    /*
     * We use local variable for the handle because we never expunge,
     * so we will never close it
     */
    libHandle = HostIFace->hostlib_Open("libAROSBootstrap.so", &err);
    D(bug("[Alert_Init] Bootstrap handle: 0x%p\n", libHandle));
    if (!libHandle)
    {
    	bug("Failed to open libAROSBootstrap.so: %s\n", err);
	return FALSE;
    }

    ptr = HostIFace->hostlib_GetPointer(libHandle, "DisplayPipe", NULL);
    D(bug("[Alert_Init] DisplayPipe pointer: %p\n", ptr));

    alertPipe = ptr ? *ptr : -1;

    HostIFace->hostlib_Close(libHandle, NULL);

    if (alertPipe == -1)
    {
    	bug("Failed to estabilish communication with display server!\n");
    	return FALSE;
    }

    return TRUE;
}

/* kernel.resource's INIT set is executed on very early startup */
ADD2INIT(Alert_Init, 0);
