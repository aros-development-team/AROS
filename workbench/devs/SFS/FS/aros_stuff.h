#ifndef _AROS_STUFF_H
#define _AROS_STUFF_H

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <utility/date.h>

struct ASFSHandle {
        void *handle;   /* pointer to FileHandle or Lock */
        ULONG flags;
        struct ASFSDeviceInfo *device;
};

#define AHF_IS_LOCK (1<<0)
#define AHF_IS_FH   (1<<1)

struct ASFSDeviceInfo {
        struct MsgPort *taskmp;
        struct Globals *global;
        struct ASFSHandle rootfh;
};

#endif
