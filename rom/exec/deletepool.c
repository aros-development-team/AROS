#include <aros/libcall.h>
#include "machine.h"
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, DeletePool,

/*  SYNOPSIS */
	__AROS_LA(APTR, poolHeader, A0),

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

    HISTORY
	16-10-95    created by m. fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT

    struct Pool *pool=(struct Pool *)poolHeader;

    /* It is legal to DeletePool(NULL) */
    if(pool!=NULL)
    {
	void *p;
	struct Block *bl;
	ULONG size;

	/* Calculate the total size of a puddle including header. */
	size=pool->PuddleSize+MEMHEADER_TOTAL;
	/* Free the list of puddles */
	while((p=RemHead((struct List *)&pool->PuddleList))!=NULL)
	    FreeMem(p,size);

	/* Free the list of single Blocks */
	while((bl=(struct Block *)RemHead((struct List *)&pool->BlockList))!=NULL)
	    FreeMem(bl,bl->Size);

	FreeMem(pool,sizeof(struct Pool));
    }
    __AROS_FUNC_EXIT
} /* DeletePool */

