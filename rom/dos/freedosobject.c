/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/01/27 00:36:21  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:29  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:30  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:47  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:52  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(void, FreeDosObject,

/*  SYNOPSIS */
	AROS_LHA(ULONG, type, D1),
	AROS_LHA(APTR,  ptr,  D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 39, Dos)

/*  FUNCTION
	Frees an object allocated with AllocDosObject.

    INPUTS
	type - object type. The same parameter as given to AllocDosObject().
	ptr  - Pointer to object.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    switch(type)
    {
	case DOS_FILEHANDLE:
	{
	    struct FileHandle *fh=(struct FileHandle *)ptr;
	    if(fh->fh_Flags&FHF_BUF)
		FreeMem(fh->fh_Buf,fh->fh_Size);
	    FreeMem(fh,sizeof(struct FileHandle));
	    break;
	}
	case DOS_FIB:
	    FreeMem(ptr,sizeof(struct FileInfoBlock));
	    break;
	case DOS_EXALLCONTROL:
	    FreeMem(ptr,sizeof(struct ExAllControl));
	    break;
	case DOS_CLI:
	{
	    struct CommandLineInterface *cli=(struct CommandLineInterface *)ptr;
	    BPTR *cur, *next;
	    cur=(BPTR *)BADDR(cli->cli_CommandDir);
	    FreeMem(ptr,sizeof(struct CommandLineInterface));
	    while(cur!=NULL)
	    {
		next=(BPTR *)BADDR(cur[0]);
		UnLock(cur[1]);
		FreeVec(cur);
		cur=next;
	    }
	    break;
	}
    }
    AROS_LIBFUNC_EXIT
} /* FreeDosObject */
