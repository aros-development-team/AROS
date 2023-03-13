/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved
*/
 

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/kernel.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/utility.h>

#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <hidd/pci.h>

#include <string.h>

#include "nvme_debug.h"
#include "nvme_intern.h"
#include "nvme_queue_io.h"

#include LC_LIBDEFS_FILE

BOOL nvme_initsgl(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write)
{
    bug("[NVME%02ld] %s: SGL support not yet implemented!\n", unit->au_UnitNum, __func__);
    return FALSE;
}
