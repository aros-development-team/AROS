/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Create a new process (in an old way).
    Lang: english
*/
#include "dos_intern.h"
#include <dos/dostags.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(struct MsgPort *, CreateProc,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),
	AROS_LHA(LONG  , pri, D2),
	AROS_LHA(BPTR  , segList, D3),
	AROS_LHA(LONG  , stackSize, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 23, Dos)

/*  FUNCTION
	CreateProc() will create a new process (a process is a superset
	of an exec Task), with the name 'name' and the priority 'pri'.

	You should pass a segList as returned by LoadSeg() (or similar)
	in the 'segList' parameter, and specify the stack size in
	'stackSize'.

	You should really use CreateNewProc() rather than this function
	as it is much more flexible.

    INPUTS
	name        -   Name of the new process.
	pri         -   Starting priority.
	segList     -   BCPL pointer to a seglist.
	stackSize   -   The size of the initial process stack.

    RESULT
	Pointer to the pr_MsgPort in the Process structure. Will
	return NULL on failure.

    NOTES
	This will not free the seglist when the process finishes.

	This does not return a pointer to the Process structure, but
	rather the MsgPort structure contained within it. You can
	get the real Process structure by:

	struct Process *pr;
	struct MsgPort *mp;

	mp = CreateProc(...);
	pr = (struct Process *)((struct Task *)mp - 1);

	// Shouldn't use mp after this point

    EXAMPLE

    BUGS

    SEE ALSO
	CreateNewProc(), LoadSeg(), UnLoadSeg()

    INTERNALS
	Basically passes this call to CreateNewProc().

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct Process *pr;

    /* Don't forget to find out some extra defaults here */
    struct TagItem procTags[6] = {
	{ NP_Seglist,       (IPTR)segList },
	{ NP_FreeSeglist,   FALSE },
	{ NP_StackSize,     stackSize },
	{ NP_Name,          (IPTR)name },
	{ NP_Priority,      pri },
	{ TAG_DONE,         0 }
    };

    if((pr = CreateNewProc(procTags)))
    {
	return (struct MsgPort *)&pr->pr_MsgPort;
    }
    else
    {
	return NULL;
    }

    AROS_LIBFUNC_EXIT
} /* CreateProc */
