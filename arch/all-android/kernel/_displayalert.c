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

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_android.h"
#include "kernel_unix.h"

#define D(x)

struct AlertRequest
{
    ULONG cmd;
    ULONG params;
    ULONG code;
    ULONG text;
};

/* We hold our pointers in global variables because we haven't allocated anything yet */
ssize_t (*host_write)(int fd, void *buf, size_t count);
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
    res = host_write(alertPipe, &req, sizeof(req));

    /* Return zero on pipe error */
    return (res == sizeof(req));
}

/*
 * This attaches to display server pipe.
 * We do it very early, because we want startup errors to be reported via Android alerts rather
 * than just dumped into console.
 * Basically we need host's write() function and pipe fd.
 */
static int Alert_Init(void *libc)
{
    APTR libHandle;
    int *ptr;
    char *err;

    /* write() is located in system's libc */
    host_write = HostIFace->hostlib_GetPointer(libc, "write", &err);
    if (!host_write)
    {
        krnPanic(NULL, "Failed to find \"write\" function\n%s", err);
	return FALSE;
    }

    /* Now pick up display pipe fd from our bootstrap */
    libHandle = HostIFace->hostlib_Open("libAROSBootstrap.so", &err);
    D(bug("[Alert_Init] Bootstrap handle: 0x%p\n", libHandle));
    if (!libHandle)
    {
    	krnPanic(NULL, "Failed to open libAROSBootstrap.so\n%s\n", err);
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

/* kernel.resource's STARTUP set is executed on very early startup */
ADD2SET(Alert_Init, startup, 0);
