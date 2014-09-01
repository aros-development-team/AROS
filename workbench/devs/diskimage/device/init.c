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

#define __NOLIBBASE__
#include "diskimage_device.h"
#include "progress.h"
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/expat.h>

#include "rev/diskimage.device_rev.h"

#define LIBNAME "diskimage.device"
CONST TEXT USED_VAR verstag[] = VERSTAG;

struct Library *SysBase;
struct Library *DOSBase;
struct Library *IntuitionBase;
struct Library *aroscbase;

static void FreeBaseVars (struct DiskImageBase *libBase);
static struct DiskImageUnit *InitUnit (struct DiskImageBase *libBase, ULONG unit_number);
static void FreeUnit (struct DiskImageBase *libBase, struct DiskImageUnit *unit);

/* FOR RTF_AUTOINIT:
    This routine gets called after the device has been allocated.
    The device pointer is in D0.  The AmigaDOS segment list is in a0.
    If it returns the device pointer, then the device will be linked
    into the device list.  If it returns NULL, then the device
    will be unloaded.

   IMPORTANT:
    If you don't use the "RTF_AUTOINIT" feature, there is an additional
    caveat.  If you allocate memory in your Open function, remember that
    allocating memory can cause an Expunge... including an expunge of your
    device.  This must not be fatal.  The easy solution is don't add your
    device to the list until after it is ready for action.

   This call is single-threaded by exec; please read the description for
   "dev_open" below. */

/* The ROMTAG Init Function */
#ifdef __AROS__
static AROS_UFH3(struct DiskImageBase *, LibInit,
	AROS_UFHA(struct DiskImageBase *, libBase, D0),
	AROS_UFHA(BPTR, seglist, A0),
	AROS_UFHA(struct Library *, exec_base, A6)
)
{
	AROS_USERFUNC_INIT
#else
static struct DiskImageBase *LibInit (REG(d0, struct DiskImageBase *libBase),
	REG(a0, BPTR seglist), REG(a6, struct Library *exec_base))
{
#endif
	libBase->LibNode.lib_Node.ln_Type = NT_DEVICE;
	libBase->LibNode.lib_Node.ln_Pri  = 0;
	libBase->LibNode.lib_Node.ln_Name = LIBNAME;
	libBase->LibNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->LibNode.lib_Version      = VERSION;
	libBase->LibNode.lib_Revision     = REVISION;
	libBase->LibNode.lib_IdString     = VSTRING;
	libBase->SysBase                  = exec_base;
	SysBase = exec_base;

	/* Save pointer to our loaded code (the SegList) */
	libBase->SegList = seglist;

	if ((DOSBase = libBase->DOSBase = OpenLibrary("dos.library", MIN_OS_VERSION)) &&
		(libBase->UtilityBase = OpenLibrary("utility.library", MIN_OS_VERSION)) &&
#ifdef __AROS__
		(aroscbase = libBase->aroscbase = OpenLibrary("arosc.library", 41)) &&
#endif
		(IntuitionBase = libBase->IntuitionBase = OpenLibrary("intuition.library", MIN_OS_VERSION)))
	{
		if ((libBase->UnitSemaphore = CreateSemaphore()) &&
			(libBase->PluginSemaphore = CreateSemaphore()) &&
			(libBase->DiskChangeSemaphore = CreateSemaphore()) &&
			(libBase->Units = CreateList(TRUE)) &&
			(libBase->Plugins = CreateList(TRUE)) &&
			(libBase->ReloadPluginsHooks = CreateList(TRUE)) &&
			(libBase->DiskChangeHooks = CreateList(TRUE)))
		{
			return libBase;
		}
	}

	FreeBaseVars(libBase);
	DeleteLibrary((struct Library *)libBase);
	return NULL;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

static void FreeBaseVars (struct DiskImageBase *libBase) {
	struct Library *SysBase = libBase->SysBase;
	DeleteList(libBase->DiskChangeHooks);
	DeleteList(libBase->ReloadPluginsHooks);
	DeleteList(libBase->Plugins);
	DeleteList(libBase->Units);
	DeleteSemaphore(libBase->DiskChangeSemaphore);
	DeleteSemaphore(libBase->PluginSemaphore);
	DeleteSemaphore(libBase->UnitSemaphore);
	if (libBase->IntuitionBase) CloseLibrary(libBase->IntuitionBase);
#ifdef __AROS__
	if (libBase->aroscbase) CloseLibrary(libBase->aroscbase);
#endif
	if (libBase->UtilityBase) CloseLibrary(libBase->UtilityBase);
	if (libBase->DOSBase) CloseLibrary(libBase->DOSBase);
}

/* Here begins the system interface commands.  When the user calls
   OpenDevice/CloseDevice/RemDevice, this eventually gets translated
   into a call to the following routines (Open/Close/Expunge).
   Exec has already put our device pointer in a6 for us.

   IMPORTANT:
     These calls are guaranteed to be single-threaded; only one task
     will execute your Open/Close/Expunge at a time.

     For Kickstart V33/34, the single-threading method involves "Forbid".
     There is a good chance this will change.  Anything inside your
     Open/Close/Expunge that causes a direct or indirect Wait() will break
     the Forbid().  If the Forbid() is broken, some other task might
     manage to enter your Open/Close/Expunge code at the same time.
     Take care!

   Since exec has turned off task switching while in these routines
   (via Forbid/Permit), we should not take too long in them. */

/* dev_open() sets the io_Error field on an error. If it was successful,
   we should also set up the io_Unit and ln_Type fields.
   Exec takes care of setting up io_Device. */

/* Open the library */
#ifdef __AROS__
static AROS_LH3(LONG, LibOpen,
	AROS_LHA(struct IORequest *, io, A1),
	AROS_LHA(ULONG, unit_number, D0),
	AROS_LHA(ULONG, flags, D1),
	struct DiskImageBase *, libBase, 1, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
static LONG LibOpen(REG(a1, struct IORequest *io), REG(d0, ULONG unit_number),	REG(d1, ULONG flags),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	struct Library *SysBase = libBase->SysBase;
	struct Library *DOSBase = libBase->DOSBase;
	struct DiskImageUnit *unit;

	/* Subtle point: any AllocMem() call can cause a call to this device's
	   expunge vector.  If lib_OpenCnt is zero, the device might get expunged. */

	ObtainSemaphore(libBase->UnitSemaphore);
	
	io->io_Device = (struct Device *)libBase;
	io->io_Unit = NULL;
	io->io_Error = IOERR_SUCCESS;

#ifndef __AROS__
	if (libBase->LibNode.lib_OpenCnt++ == 0) {
		ObtainSemaphore(libBase->PluginSemaphore);
		InitLocaleInfo(SysBase, &libBase->LocaleInfo, "diskimagedevice.catalog");
		LoadPlugins(libBase);
		ReleaseSemaphore(libBase->PluginSemaphore);
	}
#endif
	
	if (unit_number == ~0) {
		io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

		libBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
		ReleaseSemaphore(libBase->UnitSemaphore);

		return IOERR_SUCCESS;
	}

	unit = (struct DiskImageUnit *)libBase->Units->lh_Head;
	while (unit->Node.ln_Succ) {
		if (unit->UnitNum == unit_number) {
			unit->OpenCnt++;
			
			io->io_Unit = (struct Unit *)unit;
			io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
			
			libBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
			ReleaseSemaphore(libBase->UnitSemaphore);
			
			return IOERR_SUCCESS;
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
			
				return IOERR_SUCCESS;
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

	libBase->LibNode.lib_OpenCnt--; /* End of expunge protection */

	ReleaseSemaphore(libBase->UnitSemaphore);

	return io->io_Error;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

/* There are two different things that might be returned from the dev_close()
   routine.  If the device wishes to be unloaded, then dev_close() must return
   the segment list (as given to dev_init()).  Otherwise dev_close() MUST
   return NULL. */

/* Close the library */
#ifdef __AROS__
static AROS_LH1(BPTR, LibClose,
	AROS_LHA(struct IORequest *, io, A1),
	struct DiskImageBase *, libBase, 2, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR LibClose(REG(a1, struct IORequest *io), REG(a6, struct DiskImageBase *libBase))
{
#endif
	struct Library *SysBase = libBase->SysBase;
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

	/* mark us as having one fewer openers */
	if (--libBase->LibNode.lib_OpenCnt == 0) {
		ObtainSemaphore(libBase->PluginSemaphore);
		FreePlugins(libBase);
		FreeLocaleInfo(SysBase, &libBase->LocaleInfo);
		ReleaseSemaphore(libBase->PluginSemaphore);
	}

	ReleaseSemaphore(libBase->UnitSemaphore);

	return ZERO;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

static struct DiskImageUnit *InitUnit (struct DiskImageBase *libBase, ULONG unit_number) {
	struct DiskImageUnit *unit;
	struct Library *SysBase = libBase->SysBase;

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
	struct Library *SysBase = libBase->SysBase;
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

/* dev_expunge() is called by the memory allocator when the system is low on
   memory.

   There are two different things that might be returned from the dev_expunge()
   routine.  If the device is no longer open then dev_expunge() may return the
   segment list (as given to dev_init()).  Otherwise dev_expunge() may set the
   delayed expunge flag and return NULL.
 
   One other important note: because dev_expunge() is called from the memory
   allocator, it may NEVER Wait() or otherwise take long time to complete. */

/* Expunge the library */
#ifdef __AROS__
static AROS_LH0(BPTR, LibExpunge,
	struct DiskImageBase *, libBase, 3, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR LibExpunge(REG(a6, struct DiskImageBase *libBase))
{
#endif
	struct Library *SysBase = libBase->SysBase;
	BPTR result = 0;

	/* see if anyone has us open */
	if (libBase->LibNode.lib_OpenCnt > 0) {
		/* it is still open.  set the delayed expunge flag */
		libBase->LibNode.lib_Flags |= LIBF_DELEXP;
	} else {
		/* go ahead and get rid of us. */
		result = libBase->SegList;

		/* unlink from device list */
		Remove((struct Node *)libBase); /* Remove first (before FreeMem) */

		/* ...device specific closings here... */
		FreeBaseVars(libBase);

		/* free our memory */
		DeleteLibrary((struct Library *)libBase);
	}

	return result;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(APTR, LibReserved,
	struct DiskImageBase *, libBase, 4, DiskImage
)
{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
}
#else
static APTR LibReserved (void) {
	return NULL;
}
#endif

#ifdef __AROS__
#ifdef ABIV1
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, DiskImage, b)
#else
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, DiskImage)
#endif
#else
#define LIB_ENTRY(a,b) a
#endif

const CONST_APTR LibVectors[] = {
	(APTR)LIB_ENTRY(LibOpen, 1),
	(APTR)LIB_ENTRY(LibClose, 2),
	(APTR)LIB_ENTRY(LibExpunge, 3),
	(APTR)LIB_ENTRY(LibReserved, 4),
	(APTR)LIB_ENTRY(LibBeginIO, 5),
	(APTR)LIB_ENTRY(LibAbortIO, 6),
	(APTR)LIB_ENTRY(MountImage, 7),
	(APTR)LIB_ENTRY(UnitInfo, 8),
	(APTR)LIB_ENTRY(WriteProtect, 9),
	(APTR)LIB_ENTRY(UnitControlA, 10),
	(APTR)LIB_ENTRY(ReloadPlugins, 11),
	(APTR)LIB_ENTRY(DoHookPlugins, 12),
	(APTR)LIB_ENTRY(AddDiskChangeHook, 13),
	(APTR)LIB_ENTRY(AddReloadPluginsHook, 14),
	(APTR)-1
};

const IPTR LibInitTab[] = {
	sizeof(struct DiskImageBase),
	(IPTR)LibVectors,
	(IPTR)NULL,
	(IPTR)LibInit
};

const struct Resident USED_VAR ROMTag = {
    RTC_MATCHWORD,
    (struct Resident *)&ROMTag,
    (APTR)(&ROMTag + 1),
    RTF_AUTOINIT, /* Add RTF_COLDSTART if you want to be resident */
    VERSION,
    NT_DEVICE, /* Make this NT_DEVICE if needed */
    0, /* PRI, usually not needed unless you're resident */
    (STRPTR)LIBNAME,
    (STRPTR)VSTRING,
    (APTR)LibInitTab
};
