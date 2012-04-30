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
#ifdef __AROS__
	struct Library *aroscbase;
#endif
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
#ifdef __AROS__
AROS_LD1(void, LibBeginIO,
	AROS_LPA(struct IOExtTD *, iotd, A1),
	struct DiskImageBase *, libBase, 5, DiskImage
);
AROS_LD1(LONG, LibAbortIO,
	AROS_LPA(struct IOStdReq *, which_io, A1),
	struct DiskImageBase *, libBase, 6, DiskImage
);
#else
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
#ifdef __AROS__
AROS_LD2(LONG, MountImage,
	AROS_LPA(ULONG, unit_num, D0),
	AROS_LPA(CONST_STRPTR, filename, A0),
	struct DiskImageBase *, libBase, 7, DiskImage
);
AROS_LD3(LONG, UnitInfo,
	AROS_LPA(ULONG, unit_num, D0),
	AROS_LPA(STRPTR *, filename, A0),
	AROS_LPA(BOOL *, writeprotect, A1),
	struct DiskImageBase *, libBase, 8, DiskImage
);
AROS_LD2(LONG, WriteProtect,
	AROS_LPA(ULONG, unit_num, D0),
	AROS_LPA(BOOL, writeprotect, D1),
	struct DiskImageBase *, libBase, 9, DiskImage
);
AROS_LD2(LONG, UnitControlA,
	AROS_LPA(ULONG, unit_num, D0),
	AROS_LPA(struct TagItem *, tags, A0),
	struct DiskImageBase *, libBase, 10, DiskImage
);
AROS_LD0(LONG, ReloadPlugins,
	struct DiskImageBase *, libBase, 11, DiskImage
);
AROS_LD1(void, DoHookPlugins,
	AROS_LPA(struct Hook *, hook, A0),
	struct DiskImageBase *, libBase, 12, DiskImage
);
AROS_LD2(void, AddDiskChangeHook,
	AROS_LPA(struct Hook *, hook, A0),
	AROS_LPA(BOOL, add_or_remove, D0),
	struct DiskImageBase *, libBase, 13, DiskImage
);
AROS_LD2(void, AddReloadPluginsHook,
	AROS_LPA(struct Hook *, hook, A0),
	AROS_LPA(BOOL, add_or_remove, D0),
	struct DiskImageBase *, libBase, 14, DiskImage
);
#else
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
