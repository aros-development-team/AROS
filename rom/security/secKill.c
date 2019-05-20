/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/security.h>

#include <stdio.h>

#include "security_intern.h"
#include "security_task.h"

/*****************************************************************************

    NAME */
	AROS_LH1(BOOL, secKill,

/*  SYNOPSIS */
	/* (task) */
	AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 30, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL res = FALSE;
    UBYTE *sp;
    struct secExtOwner *xowner;
    
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    xowner = GetTaskExtOwner(secBase, FindTask(NULL));
    if (task && (task != FindTask(NULL)) && 
                    (task != (struct Task*)secBase->Server) &&
             (secGetRelationshipA(xowner, 0, NULL) & secRelF_ROOT_UID)) {
        Disable();
        switch (task->tc_Node.ln_Type) {
            case NT_TASK:
                    RemTask(task);
                    res = TRUE;
                    break;

            case NT_PROCESS:
                    Remove((struct Node*)task);
                    task->tc_State = TS_READY;
                    sp = task->tc_SPReg;
#if (0)
                    if (SysBase->AttnFlags & AFF_68881) {
                        ULONG size;
                        if ((size = *(ULONG *)sp)!=NULL) {
                            sp += 110;

                            if (size == 0x90)
                                sp += 12;

                            if ((SysBase->LibNode.lib_Version > 37) ||
                                 ((SysBase->LibNode.lib_Version == 37) &&
                                 (SysBase->LibNode.lib_Revision >= 132)))
                                sp += 2;

                            size = sp[1];
                            sp += size;
                        }
                        sp += 4;
                    }
#endif
                    *(IPTR *)sp = (IPTR)CleanUpBody;
                    AddHead((struct List*)&SysBase->TaskReady, (struct Node*)task);
                    res = TRUE;
                    break;
        }
        Enable();
    }
    secFreeExtOwner(xowner);
    return(res);

    AROS_LIBFUNC_EXIT

} /* secKill */

