/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

        AROS_LH1(void, MatchEnd,

/*  SYNOPSIS */
        AROS_LHA(struct AnchorPath *, AP, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 139, Dos)

/*  FUNCTION
        Free the memory and file locks that were allocated by calls to
        MatchFirst() and MatchNext().

    INPUTS
        AP  - pointer to Anchor Path structure which had been passed to
              MatchFirst() before.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AChain *ac = AP->ap_Base, *acnext;

    if (ac)
    {
#if MATCHFUNCS_NO_DUPLOCK
        /*
        ** CurrentDir to a valid lock, ie. one that will not be
        ** killed further below
        */
        
        CurrentDir(ac->an_Lock);
#endif
        
        while(ac)
        {
            acnext = ac->an_Child;

            /*
            ** Dont unlock lock in first AChain because it is the same
            ** as the current directory when MatchFirst was called. And
            ** this lock was not DupLock()ed (except MATCHFUNCS_NO_DUPLOCK == 0)!!!
            */
            
            if (ac->an_Lock
#if MATCHFUNCS_NO_DUPLOCK
                && (ac != AP->ap_Base)
#endif
               )
            {
                UnLock(ac->an_Lock);
            }
            Match_FreeAChain(ac, DOSBase);

            ac = acnext;
        }
    }
    
    AP->ap_Current = NULL;
    AP->ap_Base = NULL;
    
    AROS_LIBFUNC_EXIT
    
} /* MatchEnd */
