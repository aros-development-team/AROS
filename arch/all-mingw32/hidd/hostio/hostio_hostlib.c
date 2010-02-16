#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include LC_LIBDEFS_FILE

#include "hostio.h"

const char *kernel_func_names[] = {
    "CreateFileA",
    "CloseHandle",
    "ReadFile",
    "WriteFile",
    "DeviceIoControl",
    "GetLastError",
    NULL
};

static const char *aros_func_names[] = {
    "KrnAllocIRQ",
    "KrnFreeIRQ",
     NULL
};

void *KernelBase;
void *HostLibBase;

static APTR *load_so(const char *sofile, const char **names, void **handle)
{
    APTR *funcptr;
    ULONG i;

    D(bug("[HostIO] loading functions from %s\n", sofile));

    if ((*handle = HostLib_Open(sofile, NULL)) == NULL) {
        D(kprintf("[HostIO] couldn't open '%s'\n", sofile));
        return NULL;
    }

    funcptr = HostLib_GetInterface(*handle, names, &i);
    if (funcptr) {
        if (!i) {
            D(bug("[HostIO] done\n"));
            return funcptr;
        }
        D(kprintf("[HostIO] couldn't get %lu symbols from '%s'\n", i, sofile));
        HostLib_DropInterface(funcptr);
    }
    HostLib_Close(*handle, NULL);
    return NULL;
}

static int hostio_hostlib_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[HostIO] hostlib init\n"));

    if ((KernelBase = OpenResource("kernel.resource")) == NULL) {
        kprintf("[gdi] couldn't open kernel.resource");
        return FALSE;
    }
    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        kprintf("[gdi] couldn't open hostlib.resource");
        return FALSE;
    }

    LIBBASE->hio_csd.KernelIFace = (struct KernelInterface *)load_so("kernel32.dll", kernel_func_names, &LIBBASE->hio_csd.kernel_handle);
    if (!LIBBASE->hio_csd.KernelIFace)
        return FALSE;
    LIBBASE->hio_csd.AROSIFace = (struct AROSInterface *)load_so("kernel.dll", aros_func_names, &LIBBASE->hio_csd.aros_handle);
    if (LIBBASE->hio_csd.AROSIFace)
        return TRUE;
    HostLib_DropInterface((APTR *)LIBBASE->hio_csd.KernelIFace);
    HostLib_Close(LIBBASE->hio_csd.kernel_handle, NULL);
    return FALSE;
}

static int hostio_hostlib_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[HostIO] hostlib expunge\n"));
    
    return FALSE; /* Temporary shutup, otherwise crash */

    if (LIBBASE->hio_csd.KernelIFace != NULL)
        HostLib_DropInterface((APTR *)LIBBASE->hio_csd.KernelIFace);
    if (LIBBASE->hio_csd.kernel_handle != NULL)
        HostLib_Close(LIBBASE->hio_csd.kernel_handle, NULL);

    if (LIBBASE->hio_csd.AROSIFace != NULL)
        HostLib_DropInterface((APTR *)LIBBASE->hio_csd.AROSIFace);
    if (LIBBASE->hio_csd.aros_handle != NULL)
        HostLib_Close(LIBBASE->hio_csd.aros_handle, NULL);

    return TRUE;
}

ADD2INITLIB(hostio_hostlib_init, 1)
ADD2EXPUNGELIB(hostio_hostlib_expunge, 1)

