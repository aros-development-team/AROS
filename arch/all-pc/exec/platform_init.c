#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/efi.h>
#include <proto/exec.h>

#include "exec_intern.h"

static int Platform_Init(struct ExecBase *SysBase)
{
    struct EFIBase *EFIBase = OpenResource("efi.resource");

    D(bug("[exec] efi.resource 0x%p\n", EFIBase));

    if (EFIBase)
    {
	/* Pick up EFI runtime services pointer. */
	PD(SysBase).efiRT = EFIBase->Runtime;
    }

    return TRUE;
}

ADD2INITLIB(Platform_Init, 0);
