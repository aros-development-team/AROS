/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <string.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH2(UBYTE *, NextPubScreen,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, screen  , A0),
        AROS_LHA(UBYTE *        , namebuff, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 89, Intuition)

/*  FUNCTION
        Gets the next public screen in the system; this allows visitor windows
        to jump among public screens in a cycle.

    INPUTS
        screen   - Pointer to the public screen your window is open in or
                   NULL if you don't have a pointer to a public screen.
        namebuff - Pointer to a buffer with (at least) MAXPUBSCREENNAME+1
                   characters to put the name of the next public screen in.

    RESULT
        Returns 'namebuff' or NULL if there are no public screens.

    NOTES
        We cannot guarantee that the public screen, the name of which you got
        by using this function, is available when you call for instance 
        LockPubScreen(). Therefore you must be prepared to handle failure of
        that kind of functions.

        This function may return the name of a public screen which is in
        private mode.

        The cycle order is undefined, so draw no conclusions based on it!

    EXAMPLE

    BUGS

    SEE ALSO
        OpenScreen(), PubScreenStatus()

    INTERNALS
        Maybe we should correct the + 1 stupidity.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PubScreenNode *ps;
    struct IntScreen     *scr = (struct IntScreen *)screen;
    struct List          *list;

    list = LockPubScreenList();

    if(scr == NULL)
    {
        ps = (struct PubScreenNode *)list->lh_Head;
        ASSERT(ps != NULL);
    }
    else
    {
        ps = scr->pubScrNode;
        if (ps != NULL)
        {
            ps = (struct PubScreenNode *)ps->psn_Node.ln_Succ;
            ASSERT(ps != NULL);
        }
        else
            namebuff = NULL;
    }

    /* A "valid" node in a list must have ln_Succ != NULL */

    if (ps->psn_Node.ln_Succ == NULL)
        namebuff = NULL;

    if (namebuff != NULL)
        strcpy(namebuff, ps->psn_Node.ln_Name);

    UnlockPubScreenList();

    return namebuff;

    AROS_LIBFUNC_EXIT
} /* NextPubScreen */
