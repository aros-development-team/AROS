/*
	Copyright (C) 1997 AROS - The Amiga Replacement OS
	$Id$

	Desc: Start up the ol' Dos boot process.
	Lang: english 
*/

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <libraries/expansionbase.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

extern void boot();

AROS_UFH3(void, intBoot,
    AROS_UFHA(APTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *,SysBase, A6)
)
{
	struct Library *DOSBase = NULL;
	BPTR lock;

	DOSBase = OpenLibrary("dos.library", 0);
	if( DOSBase == NULL)
		Alert(AT_DeadEnd| AG_OpenLib | AN_DOSLib | AO_DOSLib);

	/* We have to do all the locking in this because we don't
	   have a Process in DOSBoot yet, and we need a process
	   to create locks etc.
	*/

	lock = Lock("SYS:", SHARED_LOCK);

	if( lock )
		CurrentDir(lock);
	else
		Alert( AT_DeadEnd | AG_BadParm | AN_DOSLib );

	lock = Lock("SYS:c", SHARED_LOCK);
	if( lock )
		AssignLock("C", lock);

	lock = Lock("SYS:s", SHARED_LOCK);
	if( lock )
		AssignLock("S", lock);

	lock = Lock("SYS:libs", SHARED_LOCK);
	if( lock )
		AssignLock("Libs", lock);

	lock = Lock("SYS:devs", SHARED_LOCK);
	if( lock )
		AssignLock("Devs", lock);
	
	/* Late binding ENVARC: assign, only if used */
	AssignLate("ENVARC:", "SYS:Prefs/env-archive");

	/* We now call the system dependant boot - should never return. */
	AROS_UFC3(void, boot, 
		AROS_UFCA(STRPTR, argString, A0),
		AROS_UFCA(ULONG, argSize, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6));
}

void DOSBoot(struct ExecBase *SysBase, struct DosLibrary *DOSBase)
{
	struct ExpansionBase *ExpansionBase;
	struct BootNode *bn;
	struct DosList *sysvol;

	struct TagItem bootprocess[] = 
	{
		{ NP_Entry,	(IPTR)intBoot },
		{ NP_Name,	(IPTR)"Boot Process" },
		{ NP_Input,	(IPTR)NULL },
		{ NP_Output,	(IPTR)NULL },
		{ NP_CurrentDir,(IPTR)NULL },
		{ NP_Cli,	1 },
		{ TAG_END, }
	};

	ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",0);
	if( ExpansionBase == NULL )
	{
		D(bug("Urk, no expansion.library, something's wrong!\n"));
		Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_ExpansionLib);
	}

	D(bug("Examining MountList: \n"));

	ForeachNode(&ExpansionBase->MountList, bn)
	{
		D(bug("Node: %p, DevNode: %p, Name = %s\n", bn,
			bn->bn_DeviceNode,
			((struct DosList *)bn->bn_DeviceNode)->dol_DevName
			? ((struct DosList *)bn->bn_DeviceNode)->dol_DevName
			: "(null)"
		));
		AddDosEntry((struct DosList *)bn->bn_DeviceNode);
	}

	bn = (struct BootNode *)ExpansionBase->MountList.lh_Head;
	sysvol = MakeDosEntry("SYS", DLT_DIRECTORY);
	if( sysvol == NULL )
		Alert( AT_DeadEnd | AN_DOSLib | AO_Unknown );

	sysvol->dol_Device = ((struct DosList *)bn->bn_DeviceNode)->dol_Device;
	sysvol->dol_Unit   = ((struct DosList *)bn->bn_DeviceNode)->dol_Unit;
	AddDosEntry( sysvol );

	if(CreateNewProc(bootprocess) == NULL)
	{
		D(bug("CreateNewProc() failed with %ld\n",
			((struct Process *)FindTask(NULL))->pr_Result2));
		Alert( AT_DeadEnd | AN_DOSLib | AG_ProcCreate );
	}
	CloseLibrary((struct Library*)ExpansionBase);
}
