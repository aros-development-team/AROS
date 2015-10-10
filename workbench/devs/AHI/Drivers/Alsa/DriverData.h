#ifndef AHI_Drivers_Alsa_DriverData_h
#define AHI_Drivers_Alsa_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "DriverBase.h"

struct AlsaBase
{
    struct DriverBase driverbase;
    struct DosLibrary*   dosbase;
};

#define DRIVERBASE_SIZEOF (sizeof (struct AlsaBase))

#define DOSBase (*(struct DosLibrary**) &AlsaBase->dosbase)

struct AlsaData
{
    struct DriverData   driverdata;
    BYTE                mastersignal;
    BYTE                slavesignal;
    struct Process*     mastertask;
    struct Process*     slavetask;
    struct AlsaBase*    ahisubbase;
    APTR                mixbuffer;
};


#endif /* AHI_Drivers_Alsa_DriverData_h */
