/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <clib/dos_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(LONG, Flush,

/*  SYNOPSIS */
	__AROS_LA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 60, Dos)

/*  FUNCTION
	Flushes any pending writes on the file. If the file was used
	for input and there is still some data to read it tries to
	seek back to the expected position.

    INPUTS
	file - filehandle

    RESULT
	!=0 on success, 0 on error. IoErr() gives additional information
	in that case.

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

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);
    long success=1;

    /* If the file is in write mode... */
    if(fh->fh_Flags&FHF_WRITE)
    {
	UBYTE *pos;
	/* Unset write flag */
	fh->fh_Flags&=~FHF_WRITE;

	/* Write the data. (In many pieces if the first one isn't enough). */
	pos=fh->fh_Buf;
        while(pos!=fh->fh_Pos)
        {
            LONG size;
	    size=Write(file,pos,fh->fh_Pos-pos);

	    /* An error happened? No success. */
	    if(size<0)
	    {
	        success=0;
		break;
	    }
	    pos+=size;
	}
    }else if(fh->fh_Pos<fh->fh_End)
        /* Read mode. Try to seek back to the current position. */
        if(Seek(file,fh->fh_Pos-fh->fh_End,OFFSET_CURRENT)<0)
            success=0;
    /* Reinit the buffer and return. */
    fh->fh_Pos=fh->fh_End=fh->fh_Buf;
    return success;
    __AROS_FUNC_EXIT
} /* Flush */
