#include <libraries/partition.h>
#include <proto/partition.h>

#include "internal/debug.h"
#include "functions.h"
#include "giggledisk.h"
#include "header.h"
#include "readargs.h"
#include "requester.h"

/*
** Device
*/

struct PartitionHandle *Root;

/****************************************************************************/

/* /// Device_Close()
**
*/

/*****************************************************************************/

void Device_Close(struct MDevice *device)
{
    CloseRootPartition(device->root);
}
/* \\\ */

/* /// GD_OpenDevice() */

/*************************************************************************/

ULONG GD_OpenDevice(void)
{
    requester_args[0] = readargs_array[ARG_DEVICE];
    requester_args[1] = readargs_array[ARG_UNIT];

    device.root = OpenRootPartition((STRPTR)readargs_array[ARG_DEVICE], readargs_array[ARG_UNIT]);
    debug("Device root handle: 0x%p\n", device.root);

    return device.root ? 0 : MSG_ERROR_UnableToOpenDevice;
}
/* \\\ */
