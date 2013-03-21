/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/resident.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"

extern const char Dosboot_LibID[];
extern const int Dosboot_End;

AROS_UFP3(static APTR, dosboot_Cleanup,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6));

const struct Resident db_Cleanup =
{
    RTC_MATCHWORD,
    (struct Resident *)&db_Cleanup,
    (void *)&Dosboot_End,
    RTF_AFTERDOS,
    VERSION_NUMBER,
    NT_PROCESS,
    -121,
    "DOSBoot cleanup",
    &Dosboot_LibID[6],
    &dosboot_Cleanup,
};

AROS_UFH3(static APTR, dosboot_Cleanup,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct DOSBootBase *base = OpenResource("dosboot.resource");
    
    if (!base)
    {
    	/* ??? What ??? */
    	return NULL;
    }

    D(bug("[dosboot cleanup] Boot screen 0x%p\n", base->bm_Screen));
    if (base->bm_Screen)
    {
    	/* Close "No boot media" screen. This is actually what we are here for. */
    	CloseBootScreen(base->bm_Screen, base);
    }
    anim_Stop(base);

    /* Well, since we are here, let's completely expunge. :) */
    CloseLibrary(&base->bm_ExpansionBase->LibNode);

    Remove(&base->db_Node);
    FreeMem(base, sizeof(struct DOSBootBase));

    return NULL;

    AROS_USERFUNC_EXIT;
}
