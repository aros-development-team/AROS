/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <dos/dos.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

__AROS_LH2(APTR, InitResident,

/*  SYNOPSIS */
	__AROS_LA(struct Resident *, resident, A1),
	__AROS_LA(BPTR,              segList,  D1),

/* LOCATION */
	struct ExecBase *, SysBase, 17, Exec)

/*  FUNCTION
	Test the resident structure and build the library or device
	with the information given therein. The Init() vector is
	called and the base address returned.

    INPUTS
	resident - Pointer to resident structure.
	segList  - Pointer to loaded module, 0 for resident modules.

    RESULT
	A pointer to the library or device ready to add to the exec lists.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT
    
    /* Check for validity */
    if(resident->rt_MatchWord!=RTC_MATCHWORD||
       resident->rt_MatchTag!=resident)
        return NULL;

    /* Depending on the autoinit flag... */
    if(resident->rt_Flags&RTF_AUTOINIT)
    {
	/* ...initialize automatically... */
        struct init
        {
            ULONG dSize;
            APTR vectors;
            APTR structure;
            ULONG_FUNC init;
        };
        struct init *init=(struct init *)resident->rt_Init;
        struct Library *library;
	library=MakeLibrary(init->vectors,init->structure,
			    init->init,init->dSize,segList);
	if(library!=NULL)
	{
	    library->lib_Node.ln_Type=resident->rt_Type;
	    library->lib_Node.ln_Name=resident->rt_Name;
	    library->lib_Version     =resident->rt_Version;
	    library->lib_IdString    =resident->rt_IdString;
	}
	return library;
    }else
	/* ...or let the library do it. */
	return __AROS_ABS_CALL3(struct Library *,resident->rt_Init,NULL,D0,
				segList,A0,SysBase,A6);
    __AROS_FUNC_EXIT
} /* InitResident */
