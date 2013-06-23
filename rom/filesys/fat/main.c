/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2013 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <aros/asmcall.h>
#include <aros/macros.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "fat_fs.h"
#include "fat_protos.h"
#include "charset.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

struct DosLibrary *DOSBase;
struct Library *UtilityBase;
struct Library *IntuitionBase;

struct Globals global_data;
struct Globals *glob = &global_data;

const TEXT version_string[] = "$VER: fat.handler 41.56 (23.6.2013)";

LONG handler(struct ExecBase *SysBase) {
    struct Message *msg;
    struct DosPacket *startuppacket;
    LONG error = ERROR_NO_FREE_STORE;

    memset(glob, 0, sizeof(struct Globals));
    NEWLIST(&glob->sblist);
    glob->ourtask = FindTask(NULL);
    glob->ourport = &((struct Process *)glob->ourtask)->pr_MsgPort;
    WaitPort(glob->ourport);

    msg = GetMsg(glob->ourport);
    startuppacket = (struct DosPacket *) msg->mn_Node.ln_Name;
    glob->devnode = BADDR(startuppacket->dp_Arg3);

    D(bug("\nFATFS: opening libraries.\n"));
    D(bug("\tFS task: %lx, port %lx\n", glob->ourtask, glob->ourport));

    glob->notifyport = CreateMsgPort();

    if ((DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 37))) {
        if ((IntuitionBase = OpenLibrary("intuition.library", 37))) {
            if ((UtilityBase = OpenLibrary("utility.library", 37))) {
                glob->fssm = BADDR(startuppacket->dp_Arg2);

                if ((glob->mempool = CreatePool(MEMF_PUBLIC, DEF_POOL_SIZE, DEF_POOL_THRESHOLD))) {

		    error = InitTimer();
		    if (!error) {
			InitCharsetTables();
                        if ((error = InitDiskHandler(glob->fssm)) == 0) {
                            ULONG pktsig = 1 << glob->ourport->mp_SigBit;
                            ULONG diskchgsig = 1 << glob->diskchgsig_bit;
                            ULONG notifysig = 1 << glob->notifyport->mp_SigBit;
                            ULONG timersig = 1 << glob->timerport->mp_SigBit;
                            ULONG mask = pktsig | diskchgsig | notifysig | timersig;
                            ULONG sigs;
                            struct MsgPort *rp;

                            D(bug("\tInitiated device: %s\n", AROS_BSTR_ADDR(glob->devnode->dol_Name)));

                            glob->devnode->dol_Task = glob->ourport;

                            D(bug("[fat] returning startup packet\n"));

                            rp = startuppacket->dp_Port;
                            startuppacket->dp_Port = glob->ourport;
                            startuppacket->dp_Res1 = DOSTRUE;
                            startuppacket->dp_Res2 = 0;
                            PutMsg(rp, startuppacket->dp_Link);

                            D(bug("Handler init finished.\n"));

                            ProcessDiskChange(); /* insert disk */

                            while(!glob->quit) {
                                sigs = Wait(mask);
                                if (sigs & diskchgsig)
                                    ProcessDiskChange();
                                if (sigs & pktsig)
                                    ProcessPackets();
                                if (sigs & notifysig)
                                    ProcessNotify();
                                if (sigs & timersig)
                                    HandleTimer();
                            }

                            D(bug("\nHandler shutdown initiated\n"));

                            error = 0;
                            startuppacket = NULL;

                            CleanupDiskHandler();
                        }
                        CleanupTimer();
                    }
                    DeletePool(glob->mempool);
                }
                else
                    error = ERROR_NO_FREE_STORE;
 
                CloseLibrary(UtilityBase);
            }
            CloseLibrary(IntuitionBase);
        }
        CloseLibrary((struct Library*)DOSBase);
    }

    DeleteMsgPort(glob->notifyport);

    if (glob->death_packet != NULL)
        ReplyPacket(glob->death_packet);

    D(bug("The end.\n"));

    if (startuppacket != NULL) {
        D(bug("[fat] returning startup packet\n"));

        startuppacket->dp_Res1 = DOSTRUE;
        startuppacket->dp_Res2 = 0;
        ReplyPacket(startuppacket);
    }

    return RETURN_OK;
}

static struct IntData {
    struct Interrupt Interrupt;
    struct ExecBase *SysBase;
    struct Task *task;
    ULONG signal;
} DiskChangeIntData;

static AROS_INTH1(DiskChangeIntHandler, struct IntData *,  MyIntData)
{
    AROS_INTFUNC_INIT

    struct ExecBase *SysBase = MyIntData->SysBase;

    Signal(MyIntData->task, MyIntData->signal);
    return 0;

    AROS_INTFUNC_EXIT
}

LONG InitDiskHandler (struct FileSysStartupMsg *fssm) {
    LONG err;
    ULONG diskchgintbit, flags;
    IPTR unit;
    UBYTE *device;

    unit = fssm->fssm_Unit;
    flags = fssm->fssm_Flags;

    device = AROS_BSTR_ADDR(fssm->fssm_Device);

    if ((diskchgintbit = AllocSignal(-1)) >= 0) {
        glob->diskchgsig_bit = diskchgintbit;

        if ((glob->diskport = CreateMsgPort())) {

            if ((glob->diskioreq = CreateIORequest(glob->diskport, sizeof(struct IOExtTD)))) {

                if (OpenDevice(device, unit, (struct IORequest *)glob->diskioreq, flags) == 0) {
                    D(bug("\tDevice successfully opened\n"));
                    Probe_64bit_support();

                    if ((glob->diskchgreq = AllocVec(sizeof(struct IOExtTD), MEMF_PUBLIC))) {
                        CopyMem(glob->diskioreq, glob->diskchgreq, sizeof(struct IOExtTD));

                        /* fill interrupt data */
                        DiskChangeIntData.SysBase = SysBase;
                        DiskChangeIntData.task = glob->ourtask;
                        DiskChangeIntData.signal = 1 << diskchgintbit;

                        DiskChangeIntData.Interrupt.is_Node.ln_Type = NT_INTERRUPT;
                        DiskChangeIntData.Interrupt.is_Node.ln_Pri = 0;
                        DiskChangeIntData.Interrupt.is_Node.ln_Name = "FATFS";
                        DiskChangeIntData.Interrupt.is_Data = &DiskChangeIntData;
                        DiskChangeIntData.Interrupt.is_Code = (VOID_FUNC)AROS_ASMSYMNAME(DiskChangeIntHandler);

                        /* fill io request data */
                        glob->diskchgreq->iotd_Req.io_Command = TD_ADDCHANGEINT;
                        glob->diskchgreq->iotd_Req.io_Data = &DiskChangeIntData.Interrupt;
                        glob->diskchgreq->iotd_Req.io_Length = sizeof(struct Interrupt);
                        glob->diskchgreq->iotd_Req.io_Flags = 0;

                        SendIO((struct IORequest*)glob->diskchgreq);

                        D(bug("\tDisk change interrupt handler installed\n"));

                        return 0;
                    }
                    else
                        err = ERROR_NO_FREE_STORE;

                    CloseDevice((struct IORequest *)glob->diskioreq);
                }
                else
                    err = ERROR_DEVICE_NOT_MOUNTED;

                DeleteIORequest(glob->diskioreq);
                glob->diskioreq = NULL;
            }
            else
                err = ERROR_NO_FREE_STORE;

            DeleteMsgPort(glob->diskport);
            glob->diskport = NULL;
        }
        else
            err = ERROR_NO_FREE_STORE;

        FreeSignal(diskchgintbit);

        glob->diskchgsig_bit = 0;
    }
    else
        err = ERROR_NO_FREE_STORE;

    return err;
}

void CleanupDiskHandler(void) {
    D(bug("\tFreeing handler resources:\n"));

    /* remove disk change interrupt */
    glob->diskchgreq->iotd_Req.io_Command = TD_REMCHANGEINT;
    glob->diskchgreq->iotd_Req.io_Data = &DiskChangeIntData.Interrupt;
    glob->diskchgreq->iotd_Req.io_Length = sizeof(struct Interrupt);
    glob->diskchgreq->iotd_Req.io_Flags = 0;

    DoIO((struct IORequest*)glob->diskchgreq);
    D(bug("\tDisk change interrupt handler removed\n"));

    CloseDevice((struct IORequest *)glob->diskioreq);
    DeleteIORequest(glob->diskioreq);
    FreeVec(glob->diskchgreq);
    DeleteMsgPort(glob->diskport);
    D(bug("\tDevice closed\n"));

    glob->diskioreq = NULL;
    glob->diskchgreq = NULL;
    glob->diskport = NULL;    

    FreeSignal(glob->diskchgsig_bit);

    D(bug("\tDone.\n"));
}
