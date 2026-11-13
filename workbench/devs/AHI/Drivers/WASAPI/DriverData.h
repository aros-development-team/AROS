#ifndef AHI_Drivers_WASAPI_DriverData_h
#define AHI_Drivers_WASAPI_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#define DRIVER_NEEDS_GLOBAL_EXECBASE
#include "DriverBase.h"

struct WASAPIBase {
    struct DriverBase driverbase;
    struct DosLibrary   *dosbase;

    /* Mixer properties */
    APTR    al_MixerHandle;
    APTR    al_MixerVolCtrl;
    LONG    al_MinVolume;
    LONG    al_MaxVolume;
};

#define DRIVERBASE_SIZEOF (sizeof (struct WASAPIBase))

#define DOSBase (*(struct DosLibrary**) &WASAPIBase->dosbase)

struct WASAPIData {
    struct DriverData   driverdata;
    UBYTE               flags;
    UBYTE               pad1;
    BYTE                mastersignal;
    BYTE                slavesignal;
    struct Process     *mastertask;
    struct Process     *slavetask;
    struct WASAPIBase    *ahisubbase;
    APTR                mixbuffer;

    APTR                WASAPIPlaybackCtrl;
    APTR                WASAPIPlaybackVolCtrl;
    APTR                WASAPIPlaybackClient;
};


#endif /* AHI_Drivers_WASAPI_DriverData_h */
