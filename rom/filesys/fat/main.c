/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2020 The AROS Development Team
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

#include <string.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

#undef SysBase
#undef UtilityBase

static LONG InitDiskHandler(struct Globals *glob);
static void CleanupDiskHandler(struct Globals *glob);

static void InitCharsetTables(struct Globals *glob)
{
    int i;

    for (i = 0; i < 65536; i++)
    {
        if (i < 256)
        {
            glob->from_unicode[i] = i;
            glob->to_unicode[i] = i;
        }
        else
            glob->from_unicode[i] = '_';
    }
}

static struct Globals *fat_init(struct Process *proc, struct DosPacket *dp,
    struct ExecBase *SysBase)
{
    struct Globals *glob;

    glob = AllocVec(sizeof(struct Globals), MEMF_ANY | MEMF_CLEAR);
    if (glob)
    {
        glob->gl_SysBase = SysBase;
        if ((glob->gl_DOSBase =
                (struct DosLibrary *)TaggedOpenLibrary(TAGGEDOPEN_DOS)))
        {
            if ((glob->gl_UtilityBase = OpenLibrary("utility.library", 0)))
            {
                NEWLIST(&glob->sblist);
                glob->ourtask = &proc->pr_Task;
                glob->ourport = &proc->pr_MsgPort;

                glob->devnode = BADDR(dp->dp_Arg3);

                D(bug("\nFATFS: opening libraries.\n"));
                D(bug("\tFS task: %lx, port %lx\n", glob->ourtask,
                    glob->ourport));

                glob->notifyport = CreateMsgPort();

                glob->fssm = BADDR(dp->dp_Arg2);

                if ((glob->mempool = CreatePool(MEMF_PUBLIC, DEF_POOL_SIZE,
                    DEF_POOL_THRESHOLD)))
                {
                    LONG error = InitTimer(glob);
                    if (!error)
                    {
                        InitCharsetTables(glob);
                        if ((error = InitDiskHandler(glob)) == 0)
                        {
                            return glob;
                        }

                        CleanupTimer(glob);
                    }
                    DeletePool(glob->mempool);
                }

                CloseLibrary(glob->gl_UtilityBase);
            }

            CloseLibrary((struct Library *)glob->gl_DOSBase);
        }

        FreeVec(glob);
    }

    return NULL;
}

static void fat_exit(struct Globals *glob)
{
    struct ExecBase *SysBase = glob->gl_SysBase;
    CleanupDiskHandler(glob);
    CleanupTimer(glob);
    DeletePool(glob->mempool);

    DeleteMsgPort(glob->notifyport);

    CloseLibrary(glob->gl_UtilityBase);
    CloseLibrary((struct Library *)glob->gl_DOSBase);

    FreeVec(glob);
}

LONG handler(struct ExecBase *SysBase)
{
    struct Globals *glob;
    struct Process *proc;
    struct MsgPort *mp;
    struct DosPacket *dp;

    D(bug("%s: start\n", __func__));

    proc = (struct Process *)FindTask(NULL);
    mp = &proc->pr_MsgPort;
    WaitPort(mp);
    dp = (struct DosPacket *)GetMsg(mp)->mn_Node.ln_Name;

    D(bug("%s: mp=%p, path='%b'\n", __func__, mp, dp->dp_Arg1));

    glob = fat_init(proc, dp, SysBase);

    if (!glob)
    {
        D(bug("%s: %b - error %d\n", __func__, dp->dp_Arg1, RETURN_FAIL));
        dp->dp_Res1 = DOSFALSE;
        dp->dp_Res2 = ERROR_NO_FREE_STORE;
        ReplyPacket(dp, SysBase);
        return RETURN_FAIL;
    }
    else
    {
        ULONG pktsig = 1 << glob->ourport->mp_SigBit;
        ULONG diskchgsig = 1 << glob->diskchgsig_bit;
        ULONG notifysig = 1 << glob->notifyport->mp_SigBit;
        ULONG timersig = 1 << glob->timerport->mp_SigBit;
        ULONG mask = pktsig | diskchgsig | notifysig | timersig;
        ULONG sigs;

        D(bug("\tInitiated device: %s\n",
            AROS_BSTR_ADDR(glob->devnode->dol_Name)));

        glob->devnode->dol_Task = glob->ourport;

        D(bug("[fat] returning startup packet\n"));

        dp->dp_Res1 = DOSTRUE;
        dp->dp_Res2 = 0;
        ReplyPacket(dp, SysBase);

        D(bug("Handler init finished.\n"));

        /* Insert disk */
        ProcessDiskChange(glob);

        while (!glob->quit)
        {
            sigs = Wait(mask);
            if (sigs & diskchgsig)
                ProcessDiskChange(glob);
            if (sigs & pktsig)
                ProcessPackets(glob);
            if (sigs & notifysig)
                ProcessNotify(glob);
            if (sigs & timersig)
                HandleTimer(glob);
        }

        D(bug("\nHandler shutdown initiated\n"));

        dp = NULL;

        if (glob->death_packet != NULL)
            ReplyPacket(glob->death_packet, SysBase);

        fat_exit(glob);
    }
    D(bug("The end.\n"));

    return RETURN_OK;
}

static AROS_INTH1(DiskChangeIntHandler, struct IntData *, MyIntData)
{
    AROS_INTFUNC_INIT

    struct ExecBase *SysBase = MyIntData->SysBase;

    Signal(MyIntData->task, MyIntData->signal);
    return 0;

    AROS_INTFUNC_EXIT
}

static LONG InitDiskHandler(struct Globals *glob)
{
    struct ExecBase *SysBase = glob->gl_SysBase;
    struct FileSysStartupMsg *fssm = glob->fssm;
    LONG err;
    ULONG diskchgintbit, flags;
    IPTR unit;
    UBYTE *device;

    unit = fssm->fssm_Unit;
    flags = fssm->fssm_Flags;

    device = AROS_BSTR_ADDR(fssm->fssm_Device);

    if ((diskchgintbit = AllocSignal(-1)) >= 0)
    {
        glob->diskchgsig_bit = diskchgintbit;

        if ((glob->diskport = CreateMsgPort()))
        {

            if ((glob->diskioreq = CreateIORequest(glob->diskport,
                sizeof(struct IOExtTD))))
            {
                if (OpenDevice(device, unit,
                    (struct IORequest *)glob->diskioreq, flags) == 0)
                {
                    D(bug("\tDevice successfully opened\n"));
                    Probe64BitSupport(glob);

                    if ((glob->diskchgreq =
                        AllocVec(sizeof(struct IOExtTD), MEMF_PUBLIC)))
                    {
                        CopyMem(glob->diskioreq, glob->diskchgreq,
                            sizeof(struct IOExtTD));

                        /* Fill interrupt data */
                        glob->DiskChangeIntData.SysBase = SysBase;
                        glob->DiskChangeIntData.task = glob->ourtask;
                        glob->DiskChangeIntData.signal = 1 << diskchgintbit;

                        glob->DiskChangeIntData.Interrupt.is_Node.ln_Type =
                            NT_INTERRUPT;
                        glob->DiskChangeIntData.Interrupt.is_Node.ln_Pri =
                            0;
                        glob->DiskChangeIntData.Interrupt.is_Node.ln_Name =
                            "FATFS";
                        glob->DiskChangeIntData.Interrupt.is_Data =
                            &glob->DiskChangeIntData;
                        glob->DiskChangeIntData.Interrupt.is_Code =
                            (VOID_FUNC)
                            AROS_ASMSYMNAME(DiskChangeIntHandler);

                        /* Fill I/O request data */
                        glob->diskchgreq->iotd_Req.io_Command =
                            TD_ADDCHANGEINT;
                        glob->diskchgreq->iotd_Req.io_Data =
                            &glob->DiskChangeIntData.Interrupt;
                        glob->diskchgreq->iotd_Req.io_Length =
                            sizeof(struct Interrupt);
                        glob->diskchgreq->iotd_Req.io_Flags = 0;

                        SendIO((struct IORequest *)glob->diskchgreq);

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

static void CleanupDiskHandler(struct Globals *glob)
{
    struct ExecBase *SysBase = glob->gl_SysBase;

    D(bug("\tFreeing handler resources:\n"));

    /* Remove disk change interrupt */
    glob->diskchgreq->iotd_Req.io_Command = TD_REMCHANGEINT;
    glob->diskchgreq->iotd_Req.io_Data = &glob->DiskChangeIntData.Interrupt;
    glob->diskchgreq->iotd_Req.io_Length = sizeof(struct Interrupt);
    glob->diskchgreq->iotd_Req.io_Flags = 0;

    DoIO((struct IORequest *)glob->diskchgreq);
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
