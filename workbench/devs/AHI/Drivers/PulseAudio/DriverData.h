#ifndef AHI_Drivers_PulseA_DriverData_h
#define AHI_Drivers_PulseA_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

struct PulseABase {
    struct DriverBase   driverbase;
    struct DosLibrary   *dosbase;

    /* Mixer properties */
    APTR                al_MixerHandle;
    APTR                al_MixerElem;
    LONG                al_MinVolume;
    LONG                al_MaxVolume;
};

#define DRIVERBASE_SIZEOF (sizeof (struct PulseABase))

#define DOSBase (*(struct DosLibrary**) &PulseABase->dosbase)

struct PulseAData {
    struct DriverData   driverdata;
    UBYTE               flags;
    UBYTE               pad1;
    BYTE                mastersignal;
    BYTE                slavesignal;
    struct Process      *mastertask;
    struct Process      *slavetask;
    struct PulseABase   *ahisubbase;
    APTR                mixbuffer;

    APTR                paudiohandle;
};


#endif /* AHI_Drivers_PulseA_DriverData_h */
