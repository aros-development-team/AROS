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

/*
 * Return the unit with the requested number, or NULL if it is absent.
 * The result is borrowed from libBase->Units. The caller must hold the
 * UnitSemaphore during traversal and for as long as it uses the pointer.
 * This helper does not acquire a lock, allocate a unit or retain it.
 */
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

/*
 * Send an existing unit a synchronous DICMD_TAGLIST request. The global
 * UnitSemaphore protects lookup and lifetime; the unit's MsgSemaphore
 * serializes use of its shared request and reply port. Return
 * ERROR_BAD_NUMBER when the unit does not exist, otherwise NO_ERROR after
 * receiving the reply. An operation error inside the tag list is not
 * propagated by this helper. The caller must keep all referenced tag
 * storage valid until the reply and must not call from a callback that
 * needs the unit process to handle another request. This wait has no
 * timeout or cancellation protocol.
 */
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
/*****************************************************************************

    NAME */
        AROS_LH2(LONG, MountImage,

/*  SYNOPSIS */
        AROS_LHA(ULONG, unit_num, D0),
        AROS_LHA(CONST_STRPTR, filename, A0),

/*  LOCATION */
        struct DiskImageBase *, libBase, 7, DiskImage
        )

/*  FUNCTION
        Remove the unit's current image and, if filename is non-NULL, open a
        replacement using automatic plugin selection. Resolve relative names
        using the calling process's current directory. The previous image is
        removed before the replacement is opened; failure does not restore it.

    INPUTS
        unit_num - Number of an existing diskimage.device unit.
        filename - NUL-terminated filename, or NULL to eject the current
            image. Keep the string readable until the call returns.

    RESULT
        Zero on reported success. ERROR_BAD_NUMBER if the unit does not exist.
        Otherwise, an operation error reported by the unit or image plugin.
        The return value is the error code; no caller IoErr() value is set by
        this wrapper.

    NOTES
        Keep diskimage.device open while calling this interface. The unit must
        already exist; these calls do not create units. Call from task/process
        context where semaphore acquisition and waiting are permitted, never
        from an interrupt or a disk-change callback. The call waits for the
        unit process without a timeout or cancellation mechanism.

        File access and plugin opening run in the unit process and may display
        requesters. The filename is borrowed for the call; image resources are
        retained by the device until removal. An actual image transition can
        update ENV:DiskImage and ENVARC:DiskImage preferences. The disk-change
        notification is processed after replying and may occur after return.
        Use UnitControlA() for explicit plugin, screen and password settings.

    BUGS
        diskimage.conf assigns A0,D0 to unit_num,filename. This implementation
        and the bundled AROS call macros use D0,A0. Generated interfaces must
        resolve this disagreement before being used on a register-sensitive
        target. This synopsis follows the implementation.

        The unit retains the address of this wrapper's temporary error value
        after return. See UnitControlA() for the stale error-pointer defect.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH3(LONG, UnitInfo,

/*  SYNOPSIS */
        AROS_LHA(ULONG, unit_num, D0),
        AROS_LHA(STRPTR *, filename, A0),
        AROS_LHA(BOOL *, writeprotect, A1),

/*  LOCATION */
        struct DiskImageBase *, libBase, 8, DiskImage
        )

/*  FUNCTION
        Obtain the mounted image's basename and the unit's software write-
        protection flag in one synchronous request. Allocate a separate copy
        of the name when an image name is present.

    INPUTS
        unit_num - Number of an existing diskimage.device unit.
        filename - Non-NULL address of writable STRPTR storage. Receives an
            allocated basename, or NULL if no name is present or allocation
            fails. Release a returned string with FreeVec().
        writeprotect - Non-NULL address of writable BOOL storage. Receives
            the unit's software protection flag.

    RESULT
        Zero on reported success; ERROR_BAD_NUMBER if the unit does not exist.
        Outputs are untouched if the unit does not exist. A zero return does
        not distinguish a missing image from a failed name allocation.

    NOTES
        Keep diskimage.device open while calling this interface. The unit must
        already exist; these calls do not create units. Call from task/process
        context where semaphore acquisition and waiting are permitted, never
        from an interrupt or a disk-change callback. The call waits for the
        unit process without a timeout or cancellation mechanism.

        Both output addresses are required. The string is a snapshot owned by
        the caller and remains valid after ejecting the image. It is not the
        full source path. The flag does not describe host-file permissions or
        whether the selected plugin implements writing.

    BUGS
        The name-allocation check tests the output address rather than the
        allocated result. Allocation failure can therefore return NULL with
        a zero error. The wrapper also leaves a temporary error-pointer
        address in the unit; see UnitControlA().

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH2(LONG, WriteProtect,

/*  SYNOPSIS */
        AROS_LHA(ULONG, unit_num, D0),
        AROS_LHA(BOOL, writeprotect, D1),

/*  LOCATION */
        struct DiskImageBase *, libBase, 9, DiskImage
        )

/*  FUNCTION
        Set the unit's software write-protection flag. Nonzero writeprotect
        enables protection; zero disables it. No mounted image is required.

    INPUTS
        unit_num - Number of an existing diskimage.device unit.
        writeprotect - Zero to clear software protection; nonzero to set it.

    RESULT
        Zero after the request is processed; ERROR_BAD_NUMBER if the unit
        does not exist. The previous state is not returned.

    NOTES
        Keep diskimage.device open while calling this interface. The unit must
        already exist; these calls do not create units. Call from task/process
        context where semaphore acquisition and waiting are permitted, never
        from an interrupt or a disk-change callback. The call waits for the
        unit process without a timeout or cancellation mechanism.

        Clearing this flag cannot make a read-only plugin writable and does
        not change the image file's permissions. The ordinary write path also
        rejects a missing plugin write method. This call does not save the
        flag in preferences or issue a disk-change notification.

    BUGS
        The wrapper leaves a temporary error-pointer address in the unit;
        see UnitControlA().

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH2(LONG, UnitControlA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, unit_num, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct DiskImageBase *, libBase, 10, DiskImage
        )

/*  FUNCTION
        Send a tag list to an existing unit and wait for its reply. Process
        tags in order until the list ends or the unit records an error.
        Earlier effects are not rolled back. Unknown tags are ignored.

    INPUTS
        unit_num - Number of an existing diskimage.device unit.
        tags - Tag list terminated by TAG_END, or NULL for a no-op request.
            Standard NextTagItem() control tags are supported. Keep the list
            and referenced input/output storage valid until return, subject
            to the error-pointer limitation below.

    RESULT
        Zero after receiving the unit's reply, even if a requested operation
        failed. ERROR_BAD_NUMBER if no such unit exists; no tags are processed
        in that case. Use DITAG_Error to receive the operation's error code.

    NOTES
        Keep diskimage.device open while calling this interface. The unit must
        already exist; these calls do not create units. Call from task/process
        context where semaphore acquisition and waiting are permitted, never
        from an interrupt or a disk-change callback. The call waits for the
        unit process without a timeout or cancellation mechanism.

        Put error outputs and operation settings before DITAG_Filename.
        Output tags after a failing operation are not processed. Initialise
        outputs before calling. Tag data is neither copied nor validated.
        A NULL output address for a getter is invalid.

        Mounting removes the previous image first and can write preferences.
        Plugin/file operations may block or display requesters. A successful
        reply does not guarantee that subsequent disk-change hooks have run.
        Do not call this interface synchronously from a unit-process hook or
        while holding resources needed by the unit or its callbacks.

    TAGS
        DITAG_Error (LONG *)
            Operation error output, initialised to zero when encountered.
            NULL disables the write. Place first; see BUGS.
        DITAG_ErrorStringLength (ULONG)
            Size of the error-text buffer in bytes, including its terminator.
            Set before DITAG_ErrorString; zero suppresses text writes.
        DITAG_ErrorString (STRPTR)
            Optional writable error-text buffer, or NULL. Initialised to an
            empty string when a nonzero length is available. Text depends on
            the error path and plugin; it is not guaranteed for every error.
        DITAG_Screen (struct Screen *)
            Borrowed requester screen for this request, or NULL. The unit
            clears it after processing the list.
        DITAG_Password (CONST_STRPTR)
            Borrowed NUL-terminated password for image opening, or NULL.
            The unit clears the pointer after processing the list.
        DITAG_CurrentDir (BPTR)
            Borrowed directory lock for subsequent filename operations in
            this request. Defaults to ZERO, not the caller's current directory.
        DITAG_Plugin (CONST_STRPTR)
            Name of a loaded plugin to select for subsequent image opening.
            An unknown name sets ERROR_OBJECT_NOT_FOUND. NULL leaves the
            current selection unchanged; the request starts in automatic mode.
        DITAG_Filename (CONST_STRPTR)
            NUL-terminated filename to mount, or NULL to eject. The current
            image is removed first. A failed replacement leaves the unit empty.
        DITAG_WriteProtect (BOOL)
            Set the software protection flag: zero clears, nonzero sets.
        DITAG_GetImageName (STRPTR *)
            Required writable output address. Receives an allocated basename
            to free with FreeVec(), or NULL. See the allocation defect above.
        DITAG_GetWriteProtect (BOOL *)
            Required writable output address for the software protection flag.
        DITAG_DiskImageType (ULONG *)
            Required writable output address. Receives DITYPE_RAW when image
            data exists, otherwise DITYPE_NONE; this is not a format detector.
        DITAG_SetDeviceType (UBYTE value)
            Assign the geometry device type and save a changed value in both
            ENV:DiskImage and ENVARC:DiskImage preferences.
        DITAG_GetDeviceType (UBYTE *)
            Required writable address for the geometry device type.
        DITAG_SetFlags (UBYTE value)
            Assign the geometry flags and save a changed value in both
            preference locations. This does not set the software protection.
        DITAG_GetFlags (UBYTE *)
            Required writable address for the geometry flags.

    BUGS
        ErrorPtr is not reset at request entry or completion. Omitting
        DITAG_Error can make a later failure write through an earlier caller's
        pointer, including a completed wrapper's stack variable. Always put
        DITAG_Error first, using NULL to disable reporting, but this does not
        repair unsafe requests issued by other callers. The implementation
        needs to clear its borrowed error pointer at request boundaries.

        DITAG_GetImageName checks the output address instead of the allocated
        string. A NULL result may be returned without an allocation error.
        ErrorStringLength persists between requests; supply the real buffer
        length before DITAG_ErrorString whenever requesting error text.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
	{ DITAG_Filename,	(IPTR)NULL },
	{ TAG_END }
};

#ifdef __AROS__
/*****************************************************************************

    NAME */
        AROS_LH0(LONG, ReloadPlugins,

/*  LOCATION */
        struct DiskImageBase *, libBase, 11, DiskImage
        )

/*  FUNCTION
        Eject images from every existing unit, free the current plugin list,
        load built-in plugins and plugins from DEVS:DiskImage, then invoke
        registered reload hooks. Images are not remounted.

    INPUTS
        None.

    RESULT
        Zero when the routine returns. Plugin discovery and initialisation
        failures are not propagated through this result. Zero is not proof
        that all expected plugins were loaded.

    NOTES
        Keep diskimage.device open. Call from DOS process context. This is a
        device-wide operation: it affects other users' mounted images and
        may remove saved image selections from ENV: and ENVARC:DiskImage.
        It waits for each unit without a timeout or cancellation mechanism.

        UnitSemaphore is held exclusively throughout. PluginSemaphore is
        held exclusively while freeing/loading plugins and calling reload
        hooks. Do not call from a unit process, plugin enumeration callback,
        or another device callback. Hooks must not synchronously call unit
        control or reload APIs, mutate hook lists, or wait for work that needs
        these locks. Signal an application task and return instead.

        All borrowed plugin pointers and names become invalid at reload.
        Copy information that must outlive a DoHookPlugins() callback.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH1(void, DoHookPlugins,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
        struct DiskImageBase *, libBase, 12, DiskImage
        )

/*  FUNCTION
        Call a hook once for each currently loaded plugin in list order.
        Pass the DiskImagePlugin pointer as object and hook->h_Data as the
        message. Ignore the hook's return value and continue enumeration.

    INPUTS
        hook - Non-NULL, initialised Hook with a valid entry point. The hook
            and its data must remain valid for the entire call.

    RESULT
        No result. An empty plugin list produces no callbacks. Returning a
        nonzero value from the hook does not stop enumeration.

    NOTES
        Keep diskimage.device open. Call from context that permits semaphore
        acquisition. Callbacks execute synchronously in the caller's task
        while PluginSemaphore is held shared. Do not mutate the plugin list,
        reload plugins, alter reload-hook registration, or wait for work that
        requires exclusive access to that semaphore.

        Plugin structures and their strings are borrowed. Copy any metadata
        needed after the callback; a concurrent reload may invalidate it as
        soon as enumeration releases the semaphore. The hook is not retained
        or added to a notification list. No timeout is provided.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH2(void, AddDiskChangeHook,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(BOOL, add_or_remove, D0),

/*  LOCATION */
        struct DiskImageBase *, libBase, 13, DiskImage
        )

/*  FUNCTION
        Add a hook to the device-wide disk-change notification list, or remove
        a hook previously added to that list. Appended hooks run in list order.

    INPUTS
        hook - Non-NULL, initialised Hook. For addition, its list node must
            not already belong to any list. For removal, it must currently
            belong to this notification list.
        add_or_remove - Nonzero to add; zero to remove.

    RESULT
        No result. There is no duplicate-registration or membership check.

    NOTES
        Keep the device open and the hook, entry code and h_Data alive until
        removal returns. The device borrows the hook; it neither copies nor
        frees it. Remove it before freeing storage or unloading callback code.
        A Hook has one list node and cannot be registered in both hook lists.
        Call registration/removal from task context; semaphore waits have no
        timeout. Do not change either hook list from inside a callback.


        Notifications execute in the affected unit process with the disk-
        change semaphore held shared. The object is a borrowed ULONG pointer
        to the unit number; the message is hook->h_Data. Copy the number during
        the callback if needed. The return value is ignored. Notifications
        follow message replies, and multiple image operations can be combined
        into one notification. They are not a per-request completion service.
        Different unit processes may invoke the same hook concurrently.

        Do not perform synchronous unit control or reload from the hook:
        waiting for the current unit to process another message deadlocks it.
        Signal the application task and return promptly. External removal
        acquires the semaphore exclusively and waits for active callbacks.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH2(void, AddReloadPluginsHook,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(BOOL, add_or_remove, D0),

/*  LOCATION */
        struct DiskImageBase *, libBase, 14, DiskImage
        )

/*  FUNCTION
        Add a hook to the device-wide plugin-reload notification list, or
        remove a hook previously added to that list. Appended hooks run in
        list order after ReloadPlugins() has rebuilt the plugin list.

    INPUTS
        hook - Non-NULL, initialised Hook. For addition, its list node must
            not already belong to any list. For removal, it must currently
            belong to this notification list.
        add_or_remove - Nonzero to add; zero to remove.

    RESULT
        No result. There is no duplicate-registration or membership check.

    NOTES
        Keep the device open and the hook, entry code and h_Data alive until
        removal returns. The device borrows the hook; it neither copies nor
        frees it. Remove it before freeing storage or unloading callback code.
        A Hook has one list node and cannot be registered in both hook lists.
        Call registration/removal from task context; semaphore waits have no
        timeout. Do not change either hook list from inside a callback.


        Notifications execute in the task calling ReloadPlugins(). The object
        is SysBase and the message is hook->h_Data; the return value is ignored.
        Both the unit and plugin semaphores are held exclusively. Do not call
        synchronous unit control, recursively reload plugins, or wait for work
        that requires those locks. Signal the application task and return.
        External removal waits for any active reload to release its semaphore.

    SEE ALSO
        UnitControlA(), MountImage(), UnitInfo(), WriteProtect(), ReloadPlugins(), DoHookPlugins(), AddDiskChangeHook(), AddReloadPluginsHook()

*****************************************************************************/
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
