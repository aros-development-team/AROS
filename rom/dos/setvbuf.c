/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "dos_intern.h"

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

    if (fh == NULL)
        return EOF;
        
    ASSERT_VALID_PTR( fh );
    ASSERT_VALID_PTR_OR_NULL(buff);

    D(bug("[SetVBuf] file=%p, buff=%p (was %p), mode=%d (flags 0x%x), size %d (was %d)\n", file, buff, BADDR(fh->fh_Buf), type, fh->fh_Flags, size, fh->fh_BufSize));

    /* Ensure that buff is BPTR aligned */
    if (buff != BADDR(MKBADDR(buff)))
        return EOF;

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

    if (size >= 0)
    {
        if (fh->fh_OrigBuf == fh->fh_Buf) {
            vbuf_free(fh);
        } else {
            /* Not ours, so we're not going to free it. */
        }
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
	    FreeMem(BADDR(fh->fh_Buf), fh->fh_BufSize);

        fh->fh_Buf = BNULL;
        fh->fh_Pos = fh->fh_End = 0;
        fh->fh_BufSize = 0;
        fh->fh_OrigBuf = BNULL;
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
    	fh->fh_OrigBuf = MKBADDR(buf);
    	flags |= FHF_OWNBUF;
    }

    if (NULL != buf)
    {
        fh->fh_BufSize = size;
        fh->fh_Flags  |= flags;
        fh->fh_Buf     = MKBADDR(buf);
        fh->fh_Pos     = 0;
	fh->fh_End     = (fh->fh_Flags & FHF_WRITE) ? fh->fh_BufSize : 0;
    }

    return buf;
}

BOOL vbuf_inject(BPTR fh, CONST_STRPTR argptr, ULONG size, struct DosLibrary *DOSBase)
{
    FileHandlePtr fhinput;
    STRPTR buf;

    if (!fh || !argptr)
    	return FALSE;
    fhinput = BADDR(fh);

    /* Handle the trivial case, where we have enough room 
     * in the minimal buffer.
     *
     * For AOS 1.3 C:Run compatabilty, the injected buffer
     * MUST start at Pos==0. Yes, that stupid command does
     * not check that fh_Pos is nonzero.
     */
    if ((fhinput->fh_Flags & FHF_BUF) && size <= 208) {
        CopyMem(argptr, BADDR(fhinput->fh_Buf), size);
        fhinput->fh_Pos = 0;
        fhinput->fh_End = size;
        return TRUE;
    }

    /* Check to see if this FileHandle has been mangled
     * by someone else. BCPL programs like to do this,
     * to work around argument injection issues with the
     * old BCPL version of RunCommand.
     */
    if (fhinput->fh_Flags & FHF_BUF) {
        if (fhinput->fh_Flags & FHF_OWNBUF) {
            if (fhinput->fh_Buf != fhinput->fh_OrigBuf) {
                D(bug("%s: Not injecting to fh %p - nonstandard buffering detected\n", __func__, fhinput));
                return FALSE;
            }
        }
    }


    /* Deallocate old filehandle's buffer (if any) */
    vbuf_free(fhinput);

    /* Must be always buffered or EndCLI won't work */
    buf = vbuf_alloc(fhinput, NULL, size);
    if (buf)
    {
    	D(bug("[vbuf_inject] Handle 0x%p, buffer 0x%p, injecting string: %s, size: %u\n", fh, buf, argptr, size));

	/* ugly hack */
	fhinput->fh_Pos = 0;
    	if (size > 0)
    	{
	    CopyMem(argptr, buf, size);
	    DB2(bug("[vbuf_inject] Buffer contents:\n"); hexdump(buf, (IPTR)buf, size));
	}
	fhinput->fh_End = size;
    }

    return TRUE;
}
