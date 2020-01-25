#ifndef AHI_Drivers_Alsa_DriverData_h
#define AHI_Drivers_Alsa_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

struct AlsaBase
{
    struct DriverBase driverbase;
    struct DosLibrary*   dosbase;

    /* Mixer properties */
    APTR    al_MixerHandle;
    APTR    al_MixerElem;
    LONG    al_MinVolume;
    LONG    al_MaxVolume;
};

#define DRIVERBASE_SIZEOF (sizeof (struct AlsaBase))

#define DOSBase (*(struct DosLibrary**) &AlsaBase->dosbase)

struct AlsaData
{
    struct DriverData   driverdata;
    UBYTE               flags;
    UBYTE               pad1;
    BYTE                mastersignal;
    BYTE                slavesignal;
    struct Process*     mastertask;
    struct Process*     slavetask;
    struct AlsaBase*    ahisubbase;
    APTR                mixbuffer;

    APTR                alsahandle;
};


#endif /* AHI_Drivers_Alsa_DriverData_h */
