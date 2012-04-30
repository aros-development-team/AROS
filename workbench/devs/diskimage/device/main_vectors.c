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

#include "diskimage_device.h"

static struct DiskImageUnit *FindUnit (struct DiskImageBase *libBase, ULONG unit_num) {
	struct DiskImageUnit *unit;
	unit = (struct DiskImageUnit *)libBase->Units->lh_Head;
	while (unit->Node.ln_Succ) {
		if (unit->UnitNum == unit_num) {
			return unit;
		}
		unit = (struct DiskImageUnit *)unit->Node.ln_Succ;
	}
	return NULL;
}

static LONG Internal_UnitControl (ULONG unit_num, struct TagItem *tags, struct DiskImageBase *libBase) {
	struct Library *SysBase = libBase->SysBase;
	struct DiskImageUnit *unit;
	struct DiskImageMsg *msg;
	struct MsgPort *replyport, *port;
	
	ObtainSemaphoreShared(libBase->UnitSemaphore);
	unit = FindUnit(libBase, unit_num);
	if (!unit) {
		ReleaseSemaphore(libBase->UnitSemaphore);
		return ERROR_BAD_NUMBER;
	}

	ObtainSemaphore(unit->MsgSemaphore);
	replyport = unit->ReplyPort;
	port = unit->MsgPort;
	msg = unit->DiskImageMsg;
	
	replyport->mp_SigTask = FindTask(NULL);
	replyport->mp_Flags = PA_SIGNAL;
	msg->dim_Command = DICMD_TAGLIST;
	msg->dim_Tags = tags;

	PutMsg(port, &msg->dim_Msg);
	WaitPort(replyport);
	GetMsg(replyport);
	
	replyport->mp_Flags = PA_IGNORE;
	replyport->mp_SigTask = NULL;
	
	ReleaseSemaphore(unit->MsgSemaphore);
	ReleaseSemaphore(libBase->UnitSemaphore);

	return NO_ERROR;
}

#ifdef __AROS__
AROS_LH2(LONG, MountImage,
	AROS_LHA(ULONG, unit_num, D0),
	AROS_LHA(CONST_STRPTR, filename, A0),
	struct DiskImageBase *, libBase, 7, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG MountImage (REG(d0, ULONG unit_num), REG(a0, CONST_STRPTR filename),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	LONG error = NO_ERROR;
	LONG error2;
	struct TagItem tags[4];
	tags[0].ti_Tag = DITAG_Error;
	tags[0].ti_Data = (IPTR)&error;
	tags[1].ti_Tag = DITAG_CurrentDir;
	tags[1].ti_Data = (IPTR)GetCurrentDir();
	tags[2].ti_Tag = DITAG_Filename;
	tags[2].ti_Data = (IPTR)filename;
	tags[3].ti_Tag = TAG_END;
	error2 = Internal_UnitControl(unit_num, tags, libBase);
	return error ? error : error2;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH3(LONG, UnitInfo,
	AROS_LHA(ULONG, unit_num, D0),
	AROS_LHA(STRPTR *, filename, A0),
	AROS_LHA(BOOL *, writeprotect, A1),
	struct DiskImageBase *, libBase, 8, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG UnitInfo (REG(d0, ULONG unit_num), REG(a0, STRPTR *filename), REG(a1, BOOL *writeprotect),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	LONG error = NO_ERROR;
	LONG error2;
	struct TagItem tags[4];
	tags[0].ti_Tag = DITAG_Error;
	tags[0].ti_Data = (IPTR)&error;
	tags[1].ti_Tag = DITAG_GetImageName;
	tags[1].ti_Data = (IPTR)filename;
	tags[2].ti_Tag = DITAG_GetWriteProtect;
	tags[2].ti_Data = (IPTR)writeprotect;
	tags[3].ti_Tag = TAG_END;
	error2 = Internal_UnitControl(unit_num, tags, libBase);
	return error ? error : error2;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH2(LONG, WriteProtect,
	AROS_LHA(ULONG, unit_num, D0),
	AROS_LHA(BOOL, writeprotect, D1),
	struct DiskImageBase *, libBase, 9, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG WriteProtect (REG(d0, ULONG unit_num), REG(d1, BOOL writeprotect),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	LONG error = NO_ERROR;
	LONG error2;
	struct TagItem tags[3];
	tags[0].ti_Tag = DITAG_Error;
	tags[0].ti_Data = (IPTR)&error;
	tags[1].ti_Tag = DITAG_WriteProtect;
	tags[1].ti_Data = writeprotect;
	tags[2].ti_Tag = TAG_END;
	error2 = Internal_UnitControl(unit_num, tags, libBase);
	return error ? error : error2;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH2(LONG, UnitControlA,
	AROS_LHA(ULONG, unit_num, D0),
	AROS_LHA(struct TagItem *, tags, A0),
	struct DiskImageBase *, libBase, 10, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG UnitControlA (REG(d0, ULONG unit_num), REG(a0, struct TagItem *tags),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	return Internal_UnitControl(unit_num, tags, libBase);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

static const struct TagItem unmount_tags[] = {
	{ DITAG_Filename,	(Tag)NULL },
	{ TAG_END }
};

#ifdef __AROS__
AROS_LH0(LONG, ReloadPlugins,
	struct DiskImageBase *, libBase, 11, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
LONG ReloadPlugins (REG(a6, struct DiskImageBase *libBase)) {
#endif
	struct Library *SysBase = libBase->SysBase;
	struct Library *UtilityBase = libBase->UtilityBase;
	struct MsgPort *replyport, *port;
	struct DiskImageMsg *msg;
	struct DiskImageUnit *unit;
	struct Hook *hook;

	ObtainSemaphore(libBase->UnitSemaphore);

	/* Unmount all units */
	unit = (struct DiskImageUnit *)libBase->Units->lh_Head;
	while (unit->Node.ln_Succ) {
		ObtainSemaphore(unit->MsgSemaphore);
		
		replyport = unit->ReplyPort;
		port = unit->MsgPort;
		msg = unit->DiskImageMsg;
		
		replyport->mp_SigTask = FindTask(NULL);
		replyport->mp_Flags = PA_SIGNAL;
		msg->dim_Command = DICMD_TAGLIST;
		msg->dim_Tags = unmount_tags;
		
		PutMsg(port, &msg->dim_Msg);
		WaitPort(replyport);
		GetMsg(replyport);
		
		ReleaseSemaphore(unit->MsgSemaphore);
		
		unit = (struct DiskImageUnit *)unit->Node.ln_Succ;
	}

	ObtainSemaphore(libBase->PluginSemaphore);
	FreePlugins(libBase);
	LoadPlugins(libBase);
	hook = (struct Hook *)libBase->ReloadPluginsHooks->lh_Head;
	while (hook->h_MinNode.mln_Succ) {
		CallHookPkt(hook, SysBase, hook->h_Data);
		hook = (struct Hook *)hook->h_MinNode.mln_Succ;
	}
	ReleaseSemaphore(libBase->PluginSemaphore);

	ReleaseSemaphore(libBase->UnitSemaphore);

	return NO_ERROR;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH1(void, DoHookPlugins,
	AROS_LHA(struct Hook *, hook, A0),
	struct DiskImageBase *, libBase, 12, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
void DoHookPlugins (REG(a0, struct Hook *hook), REG(a6, struct DiskImageBase *libBase)) {
#endif
	struct Library *SysBase = libBase->SysBase;
	struct Library *UtilityBase = libBase->UtilityBase;
	struct Node *plugin;

	ObtainSemaphoreShared(libBase->PluginSemaphore);
	plugin = libBase->Plugins->lh_Head;
	while (plugin->ln_Succ) {
		CallHookPkt(hook, plugin, hook->h_Data);
		plugin = plugin->ln_Succ;
	}
	ReleaseSemaphore(libBase->PluginSemaphore);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH2(void, AddDiskChangeHook,
	AROS_LHA(struct Hook *, hook, A0),
	AROS_LHA(BOOL, add_or_remove, D0),
	struct DiskImageBase *, libBase, 13, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
void AddDiskChangeHook (REG(a0, struct Hook *hook), REG(d0, BOOL add_or_remove),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	struct Library *SysBase = libBase->SysBase;
	
	ObtainSemaphore(libBase->DiskChangeSemaphore);
	if (add_or_remove) {
		AddTail(libBase->DiskChangeHooks, (struct Node *)hook);
	} else {
		Remove((struct Node *)hook);
	}
	ReleaseSemaphore(libBase->DiskChangeSemaphore);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH2(void, AddReloadPluginsHook,
	AROS_LHA(struct Hook *, hook, A0),
	AROS_LHA(BOOL, add_or_remove, D0),
	struct DiskImageBase *, libBase, 14, DiskImage
)
{
	AROS_LIBFUNC_INIT
#else
void AddReloadPluginsHook (REG(a0, struct Hook *hook), REG(d0, BOOL add_or_remove),
	REG(a6, struct DiskImageBase *libBase))
{
#endif
	struct Library *SysBase = libBase->SysBase;

	ObtainSemaphore(libBase->PluginSemaphore);
	if (add_or_remove) {
		AddTail(libBase->ReloadPluginsHooks, (struct Node *)hook);
	} else {
		Remove((struct Node *)hook);
	}
	ReleaseSemaphore(libBase->PluginSemaphore);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}
