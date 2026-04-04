
#include <config.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "library.h"
#include "DriverData.h"

APTR KernelBase = NULL;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL DriverInit(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;

    RPiHDMIBase->dosbase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);

    if (RPiHDMIBase->dosbase == NULL) {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    KernelBase = OpenResource("kernel.resource");

    if (KernelBase == NULL) {
        Req("Unable to open 'kernel.resource'.\n");
        return FALSE;
    }

    RPiHDMIBase->periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if (RPiHDMIBase->periiobase == 0) {
        Req("No BCM283x peripheral base found.\n");
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;

    CloseLibrary((struct Library *) DOSBase);
}
