/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:55  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:58  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:09  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:20  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

	AROS_LH1(void, SumLibrary,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 71, Exec)

/*  FUNCTION
	Builds the checksum over a given library's jumptable and either puts
	it into the library->lib_Sum field (if the library is marked as changed)
	or compares it with this field and Alert()s at mismatch.

    INPUTS
	library - Pointer to library structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddLibrary(), RemLibrary(), MakeLibrary(), MakeFunctions(), InitStruct().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE oldflags;
    ULONG sum;

    /* Arbitrate for library base */
    Forbid();

    /*
	If the library checksumming is already in progress or if the
	checksum is unused skip this part
    */
    if(library->lib_Flags&LIBF_SUMUSED&&!(library->lib_Flags&LIBF_SUMMING))
    {
	/* As long as the library is marked as changed */
	do
	{
	    ULONG *lp;

	    /* Memorize library flags */
	    oldflags=library->lib_Flags;

	    /* Tell other tasks: Summing in progress */
	    library->lib_Flags|=LIBF_SUMMING;
	    library->lib_Flags&=~LIBF_CHANGED;

	    /* As long as the summing goes multitasking may be permitted. */
	    Permit();

	    /* Build checksum. Note: library bases are LONG aligned */
	    sum=0;
	    /* Get start of jumptable */
	    lp=(ULONG *)((UBYTE *)library+library->lib_NegSize);
	    /* And sum it up */
	    while(lp<(ULONG *)library)
		sum+=*lp++;

	    /* Summing complete. Arbitrate again. */
	    Forbid();

	    /* Remove summing flag */
	    library->lib_Flags&=~LIBF_SUMMING;

	    /* Do it again if the library changed while summing. */
	}while(library->lib_Flags&LIBF_CHANGED);

	/*
	    Alert() if the library wasn't marked as changed and if the
	    checksum mismatches.
	*/
	if(!(oldflags&LIBF_CHANGED)&&library->lib_Sum!=sum)
	    Alert(AT_DeadEnd|AN_LibChkSum);

	/* Set new checksum */
	library->lib_Sum=sum;
    }

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* SumLibrary */

