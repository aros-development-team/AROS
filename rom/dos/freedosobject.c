/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:52  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(void, FreeDosObject,

/*  SYNOPSIS */
	__AROS_LA(ULONG, type, D1),
	__AROS_LA(APTR,  ptr,  D2),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
    __AROS_FUNC_EXIT
} /* FreeDosObject */
