/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:50  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(LONG, FGetC,

/*  SYNOPSIS */
	__AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 51, Dos)

/*  FUNCTION
	Get a character from a buffered file. Buffered I/O is more efficient
	for small amounts of data but less for big chunks. You have to
	use Flush() between buffered and non-buffered I/O or you'll
	clutter your I/O stream.

    INPUTS
	file   - filehandle

    RESULT
	The character read or EOF if the file ended or an error happened.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	IoErr(), Flush()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);
    LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;
    LONG size;

    /* If the file is in write mode... */
    if(fh->fh_Flags&FHF_WRITE)
    {
        /* write the buffer. (In many pieces if the first one isn't enough). */
        UBYTE *pos=fh->fh_Buf;
        while(pos!=fh->fh_Pos)
        {
            size=Write(file,pos,fh->fh_Pos-pos);
            /* An error happened? Return it. */
            if(size<0)
                return EOF;
            pos+=size;
        }
        /* Reinit filehandle. */
        fh->fh_Flags&=~FHF_WRITE;
        fh->fh_Pos=fh->fh_End=fh->fh_Buf;
    }
    /* No normal characters left. */
    if(fh->fh_Pos>=fh->fh_End)
    {
        /* Check for a pushed back EOF. */
        if(fh->fh_Pos>fh->fh_End)
        {
            /* Reinit filehandle and return EOF. */
            fh->fh_Pos=fh->fh_End;
	    *result=0;
            return EOF;
        }
        
        /* Is there a buffer? */
        if(fh->fh_Buf==NULL)
        {
            /* No. Get one. */
            fh->fh_Buf=AllocMem(IOBUFSIZE,MEMF_ANY);
            if(fh->fh_Buf==NULL)
            {
                /* Couldn't get buffer. Return error. */
                *result=ERROR_NO_FREE_STORE;
                return EOF;
            }
            /* Got it. Use it. */
            fh->fh_Flags|=FHF_BUF;
            fh->fh_Size=IOBUFSIZE;
        }
        
        /* Fill the buffer. */
        size=Read(file,fh->fh_Buf,fh->fh_Size);
        
        /* Prepare filehandle for data. */
        if(size<=0)
            size=0;
        fh->fh_Pos=fh->fh_Buf;
        fh->fh_End=fh->fh_Buf+size;
        
        /* No data read? Return EOF. */
        if(!size)
            return EOF;
    }
    
    /* All OK. Get data. */
    return *fh->fh_Pos++;
    __AROS_FUNC_EXIT
} /* FGetC */
