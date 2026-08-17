
#include <config.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>

#include "library.h"
#include "DriverData.h"
#include "rpihdmi-soc.h"

APTR KernelBase = NULL;
APTR DMABase = NULL;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL DriverInit(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;

    RPiHDMIBase->dosbase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);

    if (RPiHDMIBase->dosbase == NULL)
    {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    KernelBase = OpenResource("kernel.resource");

    if (KernelBase == NULL) {
        Req("Unable to open 'kernel.resource'.\n");
        return FALSE;
    }

    DMABase = OpenResource("dma.resource");

    if (DMABase == NULL)
    {
        Req("Unable to open 'dma.resource'.\n");
        return FALSE;
    }

    RPiHDMIBase->periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if (RPiHDMIBase->periiobase == 0)
    {
        Req("No BCM283x peripheral base found.\n");
        return FALSE;
    }

    if (RPiHDMIBase->periiobase == BCM2708_DMA_PERIIOBASE_2711)
    {
        Req("Unsupported Raspberry Pi HDMI audio hardware. P4 not yet implemented\n");
        return FALSE;
        //RPiHDMIBase->soc = &rpihdmi_bcm2711_soc;
    }
    else
    {
        RPiHDMIBase->soc = &rpihdmi_bcm283x_soc;
    }

    if (RPiHDMIBase->soc == NULL) {
        Req("Unsupported Raspberry Pi HDMI audio hardware.\n");
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
