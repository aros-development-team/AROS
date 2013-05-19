/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Delete a memory pool including all its memory.
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeletePool,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 117, Exec)

/*  FUNCTION
	Delete a pool including all it's memory.

    INPUTS
	poolHeader - The pool allocated with CreatePool() or NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), AllocPooled(), FreePooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR_OR_NULL(poolHeader);

    D(bug("[exec] DeletePool(0x%p)\n", poolHeader));

    /* It is legal to DeletePool(NULL) */
    if (poolHeader != NULL)
    {
        struct TraceLocation tp = CURRENT_LOCATION("DeletePool");
        struct Pool *pool = poolHeader + MEMHEADER_TOTAL;
        struct Node *p, *p2; /* Avoid casts */
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)poolHeader;

        D(bug("[DeletePool] Pool header 0x%p\n", pool));

        /*
         * The poolHeader is in fact a MemHeader structure. Check, if it was
         * MEMF_MANAGED memory...
         */
        if (!(mhe->mhe_MemHeader.mh_Attributes & MEMF_MANAGED))
        {
            /*
             * We are going to deallocate the whole pool.
             * Scan mungwall's allocations list and remove all chunks belonging to the pool.
             */
            MungWall_Scan(pool, &tp, SysBase);

            /*
             * Free the list of puddles.
             * Remember that initial puddle containing the pool structure is also in this list.
             * We do not need to free it until everything else is freed.
             */
            for (p = (struct Node *)pool->PuddleList.mlh_Head; p->ln_Succ; p = p2)
            {
                p2 = p->ln_Succ;

                D(bug("[DeletePool] Puddle 0x%p...", p));

                if (p != poolHeader)
                {
                    D(bug(" freeing"));
                    FreeMemHeader(p, &tp, SysBase);
                }
                D(bug("\n"));
            }


        }

        /* Free the last puddle, containing the pool header */
        D(bug("[DeletePool] Freeing initial puddle 0x%p\n", poolHeader));
        FreeMemHeader(poolHeader, &tp, SysBase);
    }

    AROS_LIBFUNC_EXIT
} /* DeletePool */
