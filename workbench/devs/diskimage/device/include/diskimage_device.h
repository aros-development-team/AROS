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

#ifndef DISKIMAGE_DEVICE_H
#define DISKIMAGE_DEVICE_H

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include <devices/diskimage.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <devices/scsidisk.h>
#include <string.h>
#include <SDI_compiler.h>
#include "support.h"
#include "prefs.h"
#include "td64.h"

#define InitMsgPort(port) \
	(port)->mp_Flags = PA_IGNORE; \
	NewList(&(port)->mp_MsgList);

#define GetProcMsgPort(proc) (&proc->pr_MsgPort);

#define dbug(args)

struct DiskImageBase {
	struct Library LibNode;
	UWORD Pad;
	BPTR SegList;
	
	struct SignalSemaphore *UnitSemaphore;
	struct SignalSemaphore *PluginSemaphore;
	struct SignalSemaphore *DiskChangeSemaphore;
	struct List *Units;
	struct List *Plugins;
	struct List *ReloadPluginsHooks;
	struct List *DiskChangeHooks;
	UWORD HeaderTestSize;
	UWORD FooterTestSize;

	struct LocaleInfo LocaleInfo;

	struct Library *SysBase;
	struct Library *DOSBase;
	struct Library *UtilityBase;
	struct Library *IntuitionBase;
};

enum {
	DICMD_DIE = 0,
	DICMD_STARTUP,
	DICMD_TAGLIST
};

struct DiskImageMsg {
	struct Message dim_Msg;
	struct DiskImageUnit *dim_Unit;
	ULONG dim_Command;
	const struct TagItem *dim_Tags;
};

struct DeathMessage {
	struct Message dm_Msg;
	LONG dm_ReturnCode;
	LONG dm_Result2;
};

struct DiskImageUnit {
	struct Node Node;
	UBYTE DeviceType;
	UBYTE Flags;
	ULONG UnitNum;
	ULONG OpenCnt;
	struct DiskImageBase *LibBase;
	struct Process *UnitProc;

	struct SignalSemaphore *IOSemaphore;
	struct MsgPort *IOPort;

	struct SignalSemaphore *MsgSemaphore;
	struct MsgPort *ReplyPort;
	struct MsgPort *MsgPort;
	struct DiskImageMsg *DiskImageMsg;
	struct DeathMessage *DeathMsg;

	STRPTR Name;
	struct DiskImagePlugin *Plugin;
	APTR ImageData;

	BPTR TempDir;
	STRPTR TempName;

	ULONG ChangeCnt;
	struct Interrupt *ObsoleteChangeInt;
	struct List *ChangeInts;

	ULONG WriteProtect;
	struct Screen *Screen;
	CONST_STRPTR Password;

	LONG Error;
	LONG *ErrorPtr;
	STRPTR ErrorString;
	ULONG ErrorStringLength;
	
	PrefsObject *Prefs;
};

/* io.c */
#ifndef __AROS__
LONG LibAbortIO (REG(a1, struct IOStdReq *which_io), REG(a6, struct DiskImageBase *libBase));
void LibBeginIO (REG(a1, struct IOExtTD *iotd), REG(a6, struct DiskImageBase *libBase));
#endif

/* unit.c */
#ifdef __AROS__
AROS_UFP3(LONG, UnitProcEntry,
	AROS_UFPA(STRPTR, argstr, A0),
	AROS_UFPA(ULONG, arglen, D0),
	AROS_UFPA(struct Library *, SysBase, A6)
);
#else
int UnitProcEntry (void);
#endif
int UnitProcMain (struct DiskImageUnit *unit);
LONG DOS2IOErr (APTR Self, LONG error);
void SetDiskImageErrorA (APTR Self, struct DiskImageUnit *unit, LONG error, LONG error_string, CONST_APTR error_args);
VARARGS68K void SetDiskImageError (APTR Self, struct DiskImageUnit *unit, LONG error, LONG error_string, ...);

/* scsicmd.c */
LONG DoSCSICmd (struct IOStdReq *io, struct SCSICmd *scsi);

/* main_vectors.c */
#ifndef __AROS__
LONG MountImage (REG(d0, ULONG unit_num), REG(a0, CONST_STRPTR filename),
	REG(a6, struct DiskImageBase *libBase));
LONG UnitInfo (REG(d0, ULONG unit_num), REG(a0, STRPTR *filename), REG(a1, BOOL *writeprotect),
	REG(a6, struct DiskImageBase *libBase));
LONG WriteProtect (REG(d0, ULONG unit_num), REG(d1, BOOL writeprotect),
	REG(a6, struct DiskImageBase *libBase));
LONG UnitControlA (REG(d0, ULONG unit_num), REG(a0, struct TagItem *tags),
	REG(a6, struct DiskImageBase *libBase));
LONG ReloadPlugins (REG(a6, struct DiskImageBase *libBase));
void DoHookPlugins (REG(a0, struct Hook *hook), REG(a6, struct DiskImageBase *libBase));
void AddDiskChangeHook (REG(a0, struct Hook *hook), REG(d0, BOOL add_or_remove),
	REG(a6, struct DiskImageBase *libBase));
void AddReloadPluginsHook (REG(a0, struct Hook *hook), REG(d0, BOOL add_or_remove),
	REG(a6, struct DiskImageBase *libBase));
#endif

/* plugin_vectors.c */
extern struct DIPluginIFace IPluginIFace;

/* plugins.c */
void LoadPlugins (struct DiskImageBase *libBase);
void FreePlugins (struct DiskImageBase *libBase);
struct DiskImagePlugin *FindPlugin (struct DiskImageUnit *unit, BPTR file, CONST_STRPTR name);
APTR OpenImage (APTR Self, struct DiskImageUnit *unit, BPTR file, CONST_STRPTR name);

/* tempfile.c */
LONG CreateTempFile (APTR Self, struct DiskImageUnit *unit, CONST_STRPTR ext,
	BPTR *tmpdir, CONST_STRPTR *tmpname);
BPTR OpenTempFile (APTR Self, struct DiskImageUnit *unit, ULONG mode);
void RemoveTempFile (APTR Self, struct DiskImageUnit *unit);

/* password.c */
STRPTR RequestPassword (APTR Self, struct DiskImageUnit *unit);

#endif
