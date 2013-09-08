/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

// in contrast to init.c we use auto opening for AROS

#include "diskimage_device.h"
#include "progress.h"
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/stdc.h>

#include <expat.h>

#define DEBUG 0
#include <aros/debug.h>

static void FreeBaseVars (struct DiskImageBase *libBase);
static struct DiskImageUnit *InitUnit (struct DiskImageBase *libBase, ULONG unit_number);
static void FreeUnit (struct DiskImageBase *libBase, struct DiskImageUnit *unit);

static int LibInit(struct DiskImageBase *libBase)
{
	D(bug("[diskimage/init] libbase %p\n", libBase));

	libBase->SysBase = (struct Library *)SysBase;

	libBase->DOSBase = (struct Library *)__aros_getbase_DOSBase();
        libBase->UtilityBase = (struct Library *)__aros_getbase_UtilityBase();
        libBase->IntuitionBase = (struct Library *)__aros_getbase_IntuitionBase();
        libBase->StdCBase = (struct Library *)__aros_getbase_StdCBase();

        if ((libBase->UnitSemaphore = CreateSemaphore()) &&
            (libBase->PluginSemaphore = CreateSemaphore()) &&
            (libBase->DiskChangeSemaphore = CreateSemaphore()) &&
            (libBase->Units = CreateList(TRUE)) &&
            (libBase->Plugins = CreateList(TRUE)) &&
            (libBase->ReloadPluginsHooks = CreateList(TRUE)) &&
            (libBase->DiskChangeHooks = CreateList(TRUE)))
        {
            return TRUE;
        }

	FreeBaseVars(libBase);
	return FALSE;
}

static void FreeBaseVars (struct DiskImageBase *libBase) {
	// struct Library *SysBase = libBase->SysBase;

	DeleteList(libBase->DiskChangeHooks);
	DeleteList(libBase->ReloadPluginsHooks);
	DeleteList(libBase->Plugins);
	DeleteList(libBase->Units);
	DeleteSemaphore(libBase->DiskChangeSemaphore);
	DeleteSemaphore(libBase->PluginSemaphore);
	DeleteSemaphore(libBase->UnitSemaphore);
}

static int LibOpen(struct DiskImageBase *libBase, struct IORequest *io, ULONG unit_number, ULONG flags)
{
	D(bug("[diskimage/open] libbase %p opencnt %d IORequest %p unit %d flags %d\n",
		libBase, libBase->LibNode.lib_OpenCnt, io, unit_number, flags));

	struct DiskImageUnit *unit;

	/* Subtle point: any AllocMem() call can cause a call to this device's
	   expunge vector.  If lib_OpenCnt is zero, the device might get expunged. */

	ObtainSemaphore(libBase->UnitSemaphore);
	
	io->io_Device = (struct Device *)libBase;
	io->io_Unit = NULL;
	io->io_Error = IOERR_SUCCESS;

	if (unit_number == ~0) {
		io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

		libBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
		ReleaseSemaphore(libBase->UnitSemaphore);

		return TRUE;
	}

	unit = (struct DiskImageUnit *)libBase->Units->lh_Head;
	while (unit->Node.ln_Succ) {
		if (unit->UnitNum == unit_number) {
			unit->OpenCnt++;
			
			io->io_Unit = (struct Unit *)unit;
			io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
			
			libBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
			ReleaseSemaphore(libBase->UnitSemaphore);
			
			return TRUE;
		}
		unit = (struct DiskImageUnit *)unit->Node.ln_Succ;
	}
	
	unit = InitUnit(libBase, unit_number);
	if (unit) {
		BPTR stdin, stdout;

		stdin = Open("NIL:", MODE_OLDFILE);
		stdout = Open("NIL:", MODE_NEWFILE);

		if (stdin && stdout) {
			unit->UnitProc = CreateNewProcTags(
				NP_Name,					unit->Node.ln_Name,
				NP_StackSize,				32768,
				NP_Input,					stdin,
				NP_Output,					stdout,
				NP_CurrentDir,				ZERO,
				NP_Entry,					UnitProcEntry,
				NP_Priority,				4,
				TAG_END);
		}

		if (unit->UnitProc) {
			struct MsgPort *replyport = unit->ReplyPort;
			struct DiskImageMsg *msg = unit->DiskImageMsg;
			struct MsgPort *port;
				
			replyport->mp_SigTask = FindTask(NULL);
			replyport->mp_Flags = PA_SIGNAL;
			msg->dim_Unit = unit;
			msg->dim_Command = DICMD_STARTUP;
			msg->dim_Tags = NULL;

			port = GetProcMsgPort(unit->UnitProc);
			PutMsg(port, &msg->dim_Msg);
			WaitPort(replyport);
			
			replyport->mp_Flags = PA_IGNORE;
			replyport->mp_SigTask = NULL;

			/* Check that it's not DeathMessage */
			if (GetMsg(replyport) == &msg->dim_Msg) {
				AddTail(libBase->Units, &unit->Node);
				io->io_Unit = (struct Unit *)unit;
				io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

				libBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
				ReleaseSemaphore(libBase->UnitSemaphore);
			
				return TRUE;
			} else {
				unit->UnitProc = NULL;
			}
		} else {
			Close(stdin);
			Close(stdout);
		}

		FreeUnit(libBase, unit);
	}

	/* IMPORTANT: Mark IORequest as "complete" */
	io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

	/* IMPORTANT: trash io_Device on open failure */
	io->io_Device = NULL;
	
	if (io->io_Error == IOERR_SUCCESS) io->io_Error = TDERR_NotSpecified;

	/* End of expunge protection */
	ReleaseSemaphore(libBase->UnitSemaphore);

	return FALSE;
}

static int LibClose(struct DiskImageBase *libBase, struct IORequest *io)
{
	D(bug("[diskimage/close] libbase %p IORequest %p unit %d\n", libBase, io, io->io_Unit));

	struct DiskImageUnit *unit = (struct DiskImageUnit *)io->io_Unit;

	ObtainSemaphore(libBase->UnitSemaphore);
	
	/* IMPORTANT: make sure the IORequest is not used again
	   with a -1 in io_Device, any BeginIO() attempt will
	   immediatly halt (which is better than a subtle corruption
	   that will lead to hard-to-trace crashes!!! */
	io->io_Unit = (struct Unit *)-1;
	io->io_Device = (struct Device *)-1;

	/* see if the unit is still in use */
	if(unit && --unit->OpenCnt == 0) {
		struct MsgPort *replyport = unit->ReplyPort;
		struct DiskImageMsg *msg = unit->DiskImageMsg;
	
		Remove(&unit->Node);

		replyport->mp_SigTask = FindTask(NULL);
		replyport->mp_Flags = PA_SIGNAL;
		msg->dim_Command = DICMD_DIE;
		msg->dim_Tags = NULL;

		PutMsg(unit->MsgPort, &msg->dim_Msg);
		WaitPort(replyport);
		GetMsg(replyport);

		replyport->mp_Flags = PA_IGNORE;
		replyport->mp_SigTask = NULL;

		FreeUnit(libBase, unit);
	}

	if (libBase->LibNode.lib_OpenCnt == 1) {
		ObtainSemaphore(libBase->PluginSemaphore);
		FreePlugins(libBase);
		FreeLocaleInfo(SysBase, &libBase->LocaleInfo);
		ReleaseSemaphore(libBase->PluginSemaphore);
	}

	ReleaseSemaphore(libBase->UnitSemaphore);

	return TRUE;
}

static struct DiskImageUnit *InitUnit (struct DiskImageBase *libBase, ULONG unit_number) {
	struct DiskImageUnit *unit;
	// struct Library *SysBase = libBase->SysBase;

	unit = AllocVec(sizeof(*unit), MEMF_CLEAR);
	if (!unit) {
		return NULL;
	}

	unit->OpenCnt = 1;
	unit->UnitNum = unit_number;
	unit->LibBase = libBase;

	if ((unit->Node.ln_Name = ASPrintf("diskimage.device unit %ld", unit_number)) &&
		(unit->IOSemaphore = CreateSemaphore()) &&
		(unit->MsgSemaphore = CreateSemaphore()) &&
		(unit->ReplyPort = CreatePortNoSignal()) &&
		(unit->DiskImageMsg = (struct DiskImageMsg *)CreateMsg(sizeof(*unit->DiskImageMsg))) &&
		(unit->DeathMsg = (struct DeathMessage *)CreateMsg(sizeof(*unit->DeathMsg))) &&
		(unit->ChangeInts = CreateList(TRUE)))
	{
		unit->DiskImageMsg->dim_Msg.mn_Node.ln_Name = unit->Node.ln_Name;
		unit->DeathMsg->dm_Msg.mn_Node.ln_Name = unit->Node.ln_Name;
		unit->DiskImageMsg->dim_Msg.mn_ReplyPort = unit->ReplyPort;
		unit->DeathMsg->dm_Msg.mn_ReplyPort = unit->ReplyPort;
		return unit;
	}
	
	FreeUnit(libBase, unit);
	return NULL;
}

static void FreeUnit (struct DiskImageBase *libBase, struct DiskImageUnit *unit) {
	// struct Library *SysBase = libBase->SysBase;
	if (unit) {
		DeleteList(unit->ChangeInts);
		DeleteMsg(&unit->DeathMsg->dm_Msg);
		DeleteMsg(&unit->DiskImageMsg->dim_Msg);
		DeletePortNoSignal(unit->ReplyPort);
		DeleteSemaphore(unit->MsgSemaphore);
		DeleteSemaphore(unit->IOSemaphore);
		FreeVec(unit->Node.ln_Name);
		FreeVec(unit);
	}
}

static int LibExpunge(struct DiskImageBase *libBase)
{
	D(bug("[diskimage/expunge] libbase %p opencnt %d\n", libBase, libBase->LibNode.lib_OpenCnt));

	FreeBaseVars(libBase);
	return TRUE;
}


ADD2INITLIB(LibInit, 0)
ADD2EXPUNGELIB(LibExpunge, 0)
ADD2OPENDEV(LibOpen, 0)
ADD2CLOSEDEV(LibClose, 0)
