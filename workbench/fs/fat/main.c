/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

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

struct DosLibrary *DOSBase;
struct Library *UtilityBase;
struct Library *IntuitionBase;

struct Globals global_data;
struct Globals *glob = &global_data;

void handler(void)
{
	struct DosPacket *startuppacket;
	LONG error = ERROR_NO_FREE_STORE;

	glob->ourtask = FindTask(NULL);
	glob->ourport = &((struct Process *)glob->ourtask)->pr_MsgPort;
	WaitPort(glob->ourport);
	startuppacket = GetPacket(glob->ourport);
	glob->devnode = BADDR(startuppacket->dp_Arg3);

	kprintf("\nFATFS: opening libraries.\n");
	kprintf("\tFS task: %lx, port %lx\n");

	if ((DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 37)))
	{
		if ((IntuitionBase = OpenLibrary("intuition.library", 37)))
		{
			if ((UtilityBase = OpenLibrary("utility.library", 37)))
			{
				glob->fssm = BADDR(startuppacket->dp_Arg2);
				if ((glob->mempool = CreatePool(MEMF_PUBLIC, DEF_POOL_SIZE, DEF_POOL_TRESHOLD)))
				{
					ULONG diskchgsig_bit;
					if ((error = InitDiskHandler(glob->fssm, &diskchgsig_bit)) == 0)
					{
						ULONG pktsig = 1 << glob->ourport->mp_SigBit;
						ULONG diskchgsig = 1 << diskchgsig_bit;
						ULONG mask = pktsig | diskchgsig;
						ULONG sigs;

						kprintf("\tInitiated device: ");
						knprints(AROS_BSTR_ADDR(glob->devnode->dol_Name), AROS_BSTR_strlen(glob->devnode->dol_Name));

						glob->devnode->dol_Task = glob->ourport;
						ReturnPacket(startuppacket, DOSTRUE, 0);
						kprintf("Handler init finished.\n");
						glob->sb = NULL;
						glob->sblist = NULL;
						glob->disk_inserted = FALSE;
						glob->disk_inhibited = FALSE;
						glob->quit = FALSE;

						ProcessDiskChange(); /* insert disk */
						while(!glob->quit)
						{
							sigs = Wait(mask);
							if (sigs & diskchgsig)
								ProcessDiskChange();
							if (sigs & pktsig)
								ProcessPackets();
						}
						kprintf("\nHandler shutdown initiated\n");
						error = 0;
						startuppacket = NULL;
						CleanupDiskHandler(diskchgsig_bit);
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

	kprintf("The end.\n");
	if (startuppacket)
		ReturnPacket(startuppacket, DOSFALSE, error);
	return;
}

static struct IntData
{
	struct Interrupt Interrupt;
	struct ExecBase *SysBase;
	struct Task *task;
	ULONG signal;
} DiskChangeIntData;

AROS_UFH3(static BOOL, DiskChangeIntHandler,
    AROS_UFHA(struct IntData *,  MyIntData, A1),
    AROS_UFHA(APTR,              dummy, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6)) {

    AROS_USERFUNC_INIT

	struct ExecBase *SysBase=MyIntData->SysBase;

	Signal(MyIntData->task, MyIntData->signal);
	return 0;

    AROS_USERFUNC_EXIT
}

LONG InitDiskHandler (struct FileSysStartupMsg *fssm, ULONG *diskchgsig_bit)
{
	LONG err;
	ULONG diskchgintbit, flags, unit;
	UBYTE *device;

	unit = fssm->fssm_Unit;
	flags = fssm->fssm_Flags;

        device = AROS_BSTR_ADDR(fssm->fssm_Device);

	if ((diskchgintbit = AllocSignal(-1)) >= 0)
	{
		*diskchgsig_bit = diskchgintbit;
		if ((glob->diskport = CreateMsgPort()))
		{
			if ((glob->diskioreq = CreateIORequest(glob->diskport, sizeof(struct IOExtTD))))
			{
				if (OpenDevice(device, unit, (struct IORequest *)glob->diskioreq, flags) == 0)
				{
					kprintf("\tDevice successfully opened\n");
					if ((glob->diskchgreq = AllocVec(sizeof(struct IOExtTD), MEMF_PUBLIC)))
					{
						CopyMem(glob->diskioreq, glob->diskchgreq, sizeof(struct IOExtTD));

						/* fill interrupt data */
						DiskChangeIntData.SysBase = SysBase;
						DiskChangeIntData.task = glob->ourtask;
						DiskChangeIntData.signal = 1 << diskchgintbit;

						DiskChangeIntData.Interrupt.is_Node.ln_Type = NT_INTERRUPT;
						DiskChangeIntData.Interrupt.is_Node.ln_Pri = 0;
						DiskChangeIntData.Interrupt.is_Node.ln_Name = "FATFS";
						DiskChangeIntData.Interrupt.is_Data = &DiskChangeIntData;
						DiskChangeIntData.Interrupt.is_Code = (void (*)(void))DiskChangeIntHandler;

						/* fill io request data */
						glob->diskchgreq->iotd_Req.io_Command = TD_ADDCHANGEINT;
						glob->diskchgreq->iotd_Req.io_Data = &DiskChangeIntData.Interrupt;
						glob->diskchgreq->iotd_Req.io_Length = sizeof(struct Interrupt);
						glob->diskchgreq->iotd_Req.io_Flags = 0;
						SendIO((struct IORequest*)glob->diskchgreq);
						kprintf("\tDisk change interrupt handler installed\n");
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
		*diskchgsig_bit = 0;
	}
	else
		err = ERROR_NO_FREE_STORE;
	return err;
}

void CleanupDiskHandler(ULONG diskchgsig_bit)
{
	kprintf("\tFreeing handler resources:\n");

	/* remove disk change interrupt */
	glob->diskchgreq->iotd_Req.io_Command = TD_REMCHANGEINT;
	glob->diskchgreq->iotd_Req.io_Data = &DiskChangeIntData.Interrupt;
	glob->diskchgreq->iotd_Req.io_Length = sizeof(struct Interrupt);
	glob->diskchgreq->iotd_Req.io_Flags = 0;
	DoIO((struct IORequest*)glob->diskchgreq);
	kprintf("\tDisk change interrupt handler removed\n");

	CloseDevice((struct IORequest *)glob->diskioreq);
	DeleteIORequest(glob->diskioreq);
	FreeVec(glob->diskchgreq);
	DeleteMsgPort(glob->diskport);
	kprintf("\tDevice closed\n");

	glob->diskioreq = NULL;
	glob->diskchgreq = NULL;
	glob->diskport = NULL;    

	FreeSignal(diskchgsig_bit);
	kprintf("\tDone.\n");
}

