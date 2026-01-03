#ifndef AHI_Drivers_OSS_DriverData_h
#define AHI_Drivers_OSS_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/oss.h>

#include "DriverBase.h"

struct OSSBase {
    struct DriverBase driverbase;
};

#define DRIVERBASE_SIZEOF (sizeof (struct OSSBase))

struct OSSData {
    struct DriverData   driverdata;
    UBYTE		flags;
    UBYTE		pad1;
    BYTE		mastersignal;
    BYTE		slavesignal;
    struct Process	*mastertask;
    struct Process	*slavetask;
    struct OSSBase	*ahisubbase;
    APTR		mixbuffer;
};


#endif /* AHI_Drivers_OSS_DriverData_h */
