/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <proto/exec.h>

#include "dos_intern.h"

#include <aros/debug.h>


/*****************************************************************************

    NAME */
#include <dos/stdio.h>
#include <proto/dos.h>

	AROS_LH4(LONG, SetVBuf,

/*        SetVBuf -- set buffering modes and size */

/*  SYNOPSIS */
	AROS_LHA(BPTR  , file, D1),
	AROS_LHA(STRPTR, buff, D2),
	AROS_LHA(LONG  , type, D3),
	AROS_LHA(LONG  , size, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 61, Dos)

/*  FUNCTION
        Changes the buffering modes and buffer size for a filehandle.
        With buff == NULL, the current buffer will be deallocated and a
        new one of (approximately) size will be allocated.  If buffer is
        non-NULL, it will be used for buffering and must be at least
        max(size,208) bytes long, and MUST be longword aligned.  If size
        is -1, then only the buffering mode will be changed.
    
    INPUTS
        file - Filehandle
        buff - buffer pointer for buffered I/O or NULL.
        type - buffering mode (see <dos/stdio.h>)
        size - size of buffer for buffered I/O (sizes less than 208 bytes
               will be rounded up to 208), or -1.

    RESULT
        0 if operation succeeded. 

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct FileHandle *fh = (struct FileHandle *)BADDR(file);
        
    ASSERT_VALID_PTR( fh );
    ASSERT_VALID_PTR_OR_NULL(buff);

    switch (type)
    {
        case BUF_LINE: 
            fh->fh_Flags = (fh->fh_Flags & ~FHF_NOBUF) | FHF_LINEBUF; 
            break;
        
        case BUF_FULL: 
            fh->fh_Flags = fh->fh_Flags & ~(FHF_NOBUF | FHF_LINEBUF); 
            break;
        
        case BUF_NONE: 
            fh->fh_Flags = (fh->fh_Flags | FHF_NOBUF) & ~FHF_LINEBUF; 
            break;
        
    	default:
    	    return EOF;
    }

    if (size != -1)
    {
        if (size < 208)
        {
            size = 208;
        }

        if (NULL != buff)
        {
            vbuf_free(fh);
            
            fh->fh_Size = size;
            fh->fh_Buf = fh->fh_Pos = buff;
            fh->fh_End = buff + size;
        }
    	else
    	{
            if (NULL == vbuf_alloc(fh, size))
            {
                return(EOF);
            }
    	}
    }

    return 0;
    
    AROS_LIBFUNC_EXIT
} /* SetVBuf */


void
vbuf_free(FileHandlePtr fh)
{
    /* free buffer allocated by system */
    if (fh->fh_Flags & FHF_BUF)
    {
        FreeMem(fh->fh_Buf, fh->fh_Size);
        
        fh->fh_Buf = fh->fh_Pos = fh->fh_End = NULL;
        fh->fh_Size = 0;
    }

    fh->fh_Flags &= ~FHF_BUF;
}


IPTR
vbuf_alloc(FileHandlePtr fh, ULONG size)
{
        STRPTR
    buf = AllocMem(size, MEMF_ANY);

    if (NULL != buf)
    {
        fh->fh_Size = size;
        fh->fh_Flags |= FHF_BUF;

        fh->fh_Pos = fh->fh_Buf = fh->fh_End = buf;
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }
    
    return(fh->fh_Buf);
}
