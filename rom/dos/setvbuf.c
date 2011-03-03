/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
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
        With buff == NULL, the current buffer will be deallocated (if it
        was not a user-supplied one) and a new one of (approximately) size
        will be allocated. If buffer is non-NULL, it will be used for
        buffering and must be at least max(size,208) bytes long, and MUST
        be longword aligned. If size is -1, then only the buffering mode
        will be changed.

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
        vbuf_free(fh);
        if (!vbuf_alloc(fh, buff, size))
            return EOF;
    }

    return 0;
    
    AROS_LIBFUNC_EXIT
} /* SetVBuf */


void
vbuf_free(FileHandlePtr fh)
{
    if (fh->fh_Flags & FHF_BUF)
    {
	/* free buffer allocated by system */
    	if (fh->fh_Flags & FHF_OWNBUF)
	    FreeMem(fh->fh_Buf, fh->fh_Size);

        fh->fh_Buf = fh->fh_Pos = fh->fh_End = NULL;
        fh->fh_Size = 0;
    }

    fh->fh_Flags &= ~(FHF_BUF | FHF_OWNBUF);
}

APTR vbuf_alloc(FileHandlePtr fh, STRPTR buf, ULONG size)
{
    ULONG flags = FHF_BUF;

    if (size < 208)
	size = 208;

    if (!buf)
    {
    	buf = AllocMem(size, MEMF_ANY);
    	flags |= FHF_OWNBUF;
    }

    if (NULL != buf)
    {
        fh->fh_Size = size;
        fh->fh_Flags |= flags;

        if(fh->fh_Flags & FHF_WRITE)
        {
            fh->fh_Pos = fh->fh_Buf = buf;
            fh->fh_End = fh->fh_Buf + fh->fh_Size;
        }
        else
        {
            fh->fh_Pos = fh->fh_Buf = fh->fh_End = buf;
        }
    }

    return buf;
}

void vbuf_inject(BPTR fh, CONST_STRPTR argptr, ULONG size, struct DosLibrary *DOSBase)
{
    FileHandlePtr fhinput;

    if (!fh)
    	return;
    fhinput = BADDR(fh);

    /* Deallocate old filehandle's buffer (if any) */
    vbuf_free(fhinput);

    /* Must be always buffered or EndCLI won't work */
    if (vbuf_alloc(fhinput, NULL, size + 1) && IsInteractive(fh))
    {
    	D(bug("[vbuf] Handle 0x%p, injecting string: %s\n", fh, argptr));

	/* ugly hack */

    	if (size > 0)
    	{
	    CopyMem(argptr, fhinput->fh_Buf, size);
	    fhinput->fh_Pos = fhinput->fh_Buf;

	    /*
	     * Append EOL if there's no one.
	     * Without it ReadArgs() blocks in FGets() reading arguments.
	     */
	    if (fhinput->fh_Buf[size - 1] != '\n')
	    	fhinput->fh_Buf[size++] = '\n';
	}
	else
	    /*
	     * Inject EOF if the string is empty.
	     * This is for the same purpose as appending EOL above, but looks more consistent.
	     * CHECKME: are these things correct at all?
	     */
	    fhinput->fh_Pos = fhinput->fh_Pos + 1;

        fhinput->fh_End = fhinput->fh_Buf + size;
    }
}
