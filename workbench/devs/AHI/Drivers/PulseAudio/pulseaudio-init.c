
#include <aros/debug.h>
#include <config.h>

#include "library.h"
#include "DriverData.h"

#include "pulseaudio-bridge/pulseaudio.h"

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit(struct DriverBase *AHIsubBase)
{
    struct PulseABase *PulseABase = (struct PulseABase *) AHIsubBase;

    D(bug("[PulseA]: DriverInit()\n"));

    PulseABase->dosbase = (struct DosLibrary *)OpenLibrary(DOSNAME, 37);

    if(PulseABase->dosbase == NULL) {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    if(!PULSEA_Init())
        return FALSE;

    PULSEA_MixerInit(&PulseABase->al_MixerHandle, &PulseABase->al_MixerElem,
                     &PulseABase->al_MinVolume, &PulseABase->al_MaxVolume);

    D(bug("[PulseA]: DriverInit() completed\n"));

    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct PulseABase *PulseABase = (struct PulseABase *) AHIsubBase;

    if(PulseABase->al_MixerHandle)
        PULSEA_MixerCleanup(PulseABase->al_MixerHandle);

    PULSEA_Cleanup();

    CloseLibrary((struct Library *) DOSBase);
}
