/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

#include <proto/exec.h>

#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <dos/dos.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include "os.h"
#include "afshandler.h"
#include "volumes.h"

#include LC_LIBDEFS_FILE

extern void AFS_work();

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR afsbase)
{
	struct Task *task;
	APTR stack;

	afsbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library",39);
	if (afsbase->dosbase != NULL)
	{
		afsbase->intuitionbase = (struct IntuitionBase *)OpenLibrary("intuition.library",39);
		if (afsbase->intuitionbase != NULL)
		{
			NEWLIST(&afsbase->device_list);
			NEWLIST(&afsbase->port.mp_MsgList);
			afsbase->port.mp_Node.ln_Type = NT_MSGPORT;
			afsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
			NEWLIST(&afsbase->rport.mp_MsgList);
			afsbase->rport.mp_Node.ln_Type = NT_MSGPORT;
			afsbase->rport.mp_Flags = PA_SIGNAL;
			afsbase->rport.mp_SigBit = SIGB_SINGLE;
			task = (struct Task *)AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
			if (task != NULL)
			{
				afsbase->port.mp_SigTask = task;
				afsbase->port.mp_Flags = PA_IGNORE;

				/* Open timer */
				afsbase->timer_request = (struct timerequest *)
					CreateIORequest(&afsbase->port,
						sizeof(struct timerequest));
				if (afsbase->timer_request != NULL)
				{
					if (OpenDevice("timer.device", UNIT_VBLANK,
						(APTR)afsbase->timer_request, 0) == 0)
					{
						/* Create handler task */
						NEWLIST(&task->tc_MemEntry);
						task->tc_Node.ln_Type = NT_TASK;
						task->tc_Node.ln_Name = "afs.handler task";
						stack = AllocMem(AROS_STACKSIZE, MEMF_PUBLIC);
						if (stack != NULL)
						{
					    		struct TagItem tags[] =
							{
							    {TASKTAG_ARG1, (IPTR)afsbase},
							    {TAG_DONE	    	    	}
							};

							task->tc_SPLower = stack;
							task->tc_SPUpper = (BYTE *)stack+AROS_STACKSIZE;
    	    			    	    	    #if AROS_STACK_GROWS_DOWNWARDS
							task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET;
    	    	    	    			    #else
							task->tc_SPReg = (BYTE *)task->tc_SPLower+SP_OFFSET;
	   		    	    	    	    #endif

							if (NewAddTask(task,AFS_work,NULL,tags) != NULL)
								return TRUE;
							FreeMem(stack, AROS_STACKSIZE);
						}
						CloseDevice((struct IORequest *)afsbase->timer_request);
					}
					DeleteIORequest((struct IORequest *)afsbase->timer_request);
				}
				FreeMem(task, sizeof(struct Task));
			}
			CloseLibrary((struct Library *)afsbase->intuitionbase);
		}
		CloseLibrary((struct Library *)afsbase->dosbase);
	}
	return FALSE;
}

#include "baseredef.h"

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR afsbase,
    struct IOFileSys *iofs,
    ULONG unitnum,
    ULONG flags
)
{
#if 0
	struct Volume *volume;
#endif

	afsbase->rport.mp_SigTask=FindTask(NULL);
#if 0
	volume = initVolume
		(
			afsbase,
			iofs->IOFS.io_Device,
			iofs->io_Union.io_OpenDevice.io_DeviceName,
			iofs->io_Union.io_OpenDevice.io_Unit,
			(struct DosEnvec *)iofs->io_Union.io_OpenDevice.io_Environ,
			&iofs->io_DosError
		);
	if (volume != NULL)
	{
		AddTail(&afsbase->device_list, &volume->ln);
		iofs->IOFS.io_Unit = (struct Unit *)(&volume->ah);
		iofs->IOFS.io_Device = &afsbase->device;
		afsbase->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
		iofs->IOFS.io_Error = 0;

		return;
	}
#else
	iofs->IOFS.io_Command = -1;
	PutMsg(&afsbase->port, &iofs->IOFS.io_Message);
	WaitPort(&afsbase->rport);
	(void)GetMsg(&afsbase->rport);
	if (iofs->IOFS.io_Unit != NULL)
	{
		AddTail(&afsbase->device_list, &(((struct AfsHandle *)iofs->IOFS.io_Unit)->volume->ln));
		iofs->IOFS.io_Device = &afsbase->device;
		iofs->IOFS.io_Error = 0;
		return TRUE;
	}
#endif
	iofs->IOFS.io_Error = IOERR_OPENFAIL;
	return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR afsbase)
{
	CloseDevice((struct IORequest *)afsbase->timer_request);
	DeleteIORequest((struct IORequest *)afsbase->timer_request);
	RemTask(afsbase->port.mp_SigTask);
	FreeMem(((struct Task *)afsbase->port.mp_SigTask)->tc_SPLower,AROS_STACKSIZE);
	FreeMem(afsbase->port.mp_SigTask, sizeof(struct Task));
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)DOSBase);
	return TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR afsbase,
    struct IOFileSys *iofs
)
{
	struct Volume *volume;

	afsbase->rport.mp_SigTask = FindTask(NULL);
/*	iofs->IOFS.io_Command = -2;
	PutMsg(&afsbase->port, &iofs->IOFS.io_Message);
	WaitPort(&afsbase->rport);
	(void)GetMsg(&afsbase->rport);
	if (iofs->io_DosError)
		return 0;				// there is still something to do on this volume
*/
	volume = ((struct AfsHandle *)iofs->IOFS.io_Unit)->volume;
	if (!volume->locklist)
	{
		Remove(&volume->ln);
		uninitVolume(afsbase, volume);
		return TRUE;
	}
	else
	{
		iofs->IOFS.io_Error = ERROR_OBJECT_IN_USE;
		return FALSE;
	}
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct AFSBase *, afsbase, 5, Afs)
{
	AROS_LIBFUNC_INIT
	/* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	/* Nothing is done quick */
    iofs->IOFS.io_Flags &= ~IOF_QUICK;
	/* So let the device task do it */
    PutMsg(&afsbase->port, &iofs->IOFS.io_Message);
	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct AFSBase *, afsbase, 6, Afs)
{
	AROS_LIBFUNC_INIT
	return 0;
	AROS_LIBFUNC_EXIT
}
