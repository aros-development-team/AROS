/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(void, UnLoadSeg,

/*  SYNOPSIS */
	__AROS_LA(BPTR, seglist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 26, Dos)

/*  FUNCTION
	Free a segment list allocated with LoadSeg().

    INPUTS
	seglist - The segment list.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	LoadSeg()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    BPTR next;

    while(seglist)
    {
        next=*(BPTR *)BADDR(seglist);
        FreeVec(BADDR(seglist));
        seglist=next;
    }

    __AROS_FUNC_EXIT
} /* UnLoadSeg */
