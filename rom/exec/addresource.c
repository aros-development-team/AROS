/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a resource to the public list of resources.
    Lang: english
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <string.h>

#include "exec_debug.h"
#include "exec_intern.h"

/* Kludge for old kernels (PPC native) */
#ifndef KrnStatMemory
#define KrnStatMemory(...)
#endif
#ifndef KrnGetSystemAttr
#define KrnGetSystemAttr(x) AROS_STACKSIZE
#endif

/*****************************************************************************

    NAME */

	AROS_LH1(void, AddResource,

/*  SYNOPSIS */
	AROS_LHA(APTR, resource, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 81, Exec)

/*  FUNCTION
	Adds a given resource to the system's resource list.

    INPUTS
	resource - Pointer to a ready for use resource.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemResource(), OpenResource()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ASSERT_VALID_PTR(resource);

    /* Just in case the user forgot them */
    ((struct Node *)resource)->ln_Type=NT_RESOURCE;

    /* Arbitrate for the resource list */
    Forbid();

    /* And add the resource */
    Enqueue(&SysBase->ResourceList,(struct Node *)resource);

    /* All done. */
    Permit();

    /*
     * A tricky part.
     * kernel.resource is the first one to get initialized. After that
     * some more things can wake up (hostlib.resource for example).
     * They might want to use AllocMem() and even AllocPooled().
     * Here we switch off boot mode of the memory manager. Currently we
     * only set up page size for pool manager. When memory protection
     * is implemented, more things will be done here.
     * This allows to use exec pools even in kernel.resource itself.
     * To do this its startup code needs to be changed to call AddResource()
     * itself. Right after this exec's pool manager will be up and running.
     */
    if (!strcmp(((struct Node *)resource)->ln_Name, "kernel.resource"))
    {
        KernelBase = resource;
        DINIT("Post-kernel init");

	/* If there's no MMU support, PageSize will stay zero */
	KrnStatMemory(0, KMS_PageSize, &PrivExecBase(SysBase)->PageSize, TAG_DONE);
	DINIT("Memory page size: %lu", PrivExecBase(SysBase)->PageSize);

	/*
	 * On MMU-less hardware kernel.resource will report zero page size.
	 * In this case we use MEMCHUNK_TOTAL as allocation granularity.
	 * This is because our Allocate() relies on the fact that all chunks
	 * are at least MemChunk-aligned, otherwise we end up in
	 * "Corrupt memory list" alert.
	 */
	if (!PrivExecBase(SysBase)->PageSize)
	    PrivExecBase(SysBase)->PageSize = MEMCHUNK_TOTAL;

	PrivExecBase(SysBase)->StackSize = KrnGetSystemAttr(KATTR_MinStack);
	DINIT("Minimum stack size: %lu", PrivExecBase(SysBase)->StackSize);

	/* We print the notice here because kprintf() works only after KernelBase is set up */
	if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
	    RawDoFmt("[exec] Mungwall enabled\n", NULL, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
    }

    AROS_LIBFUNC_EXIT
} /* AddResource */
