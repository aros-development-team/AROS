/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
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
	    
	case DOS_STDPKT:
	    FreeMem((APTR)(ptr-(APTR)(&((struct StandardPacket *)0)->sp_Pkt)),sizeof(struct StandardPacket));
	    break;
	    
	case DOS_EXALLCONTROL:
	    if (((struct InternalExAllControl *)ptr)->fib)
	        FreeDosObject(DOS_FIB, ((struct InternalExAllControl *)ptr)->fib);
	    
	    FreeMem(ptr, sizeof(struct InternalExAllControl));
	    break;
	    
	case DOS_CLI:
	{
	    struct CommandLineInterface *cli=(struct CommandLineInterface *)ptr;
	    BPTR *cur, *next;
	    cur=(BPTR *)BADDR(cli->cli_CommandDir);
	    FreeVec(BADDR(cli->cli_SetName));
	    FreeVec(BADDR(cli->cli_CommandName));
	    FreeVec(BADDR(cli->cli_CommandFile));
	    FreeVec(BADDR(cli->cli_Prompt));
	    FreeMem(ptr,sizeof(struct CommandLineInterface));
	    while(cur!=NULL)
	    {
		next=(BPTR *)BADDR(cur[0]);
		UnLock(cur[1]);
		FreeVec(cur);
		cur=next;
	    }
	    break;

	/*
	    FreeArgs() will not free a RDArgs without a RDA_DAList, 
	    see that function for more information as to why...
	*/
	case DOS_RDARGS:
	    if(((struct RDArgs *)ptr)->RDA_DAList != NULL)
		FreeArgs(ptr);
	    else
		FreeVec(ptr);

	    break;
	}
    }
    AROS_LIBFUNC_EXIT
} /* FreeDosObject */
