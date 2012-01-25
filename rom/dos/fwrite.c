/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Lang: english
*/
#include "dos_intern.h"

#include <aros/debug.h>


/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, FWrite,
/*      FWrite -- Writes a number of blocks to an output (buffered) */

/*  SYNOPSIS */
	AROS_LHA(BPTR , fh, D1),
	AROS_LHA(CONST_APTR , block, D2),
	AROS_LHA(ULONG, blocklen, D3),
	AROS_LHA(ULONG, numblocks, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 55, Dos)

/*  FUNCTION
	Write a number of blocks to a file.

    INPUTS
	fh        - Write to this file
	block     - The data begins here
    blocklen  - number of bytes per block.  Must be > 0.
    numblocks - number of blocks to write.  Must be > 0.

    RESULT
	The number of blocks written to the file or EOF on error. IoErr()
	gives additional information in case of an error.

    SEE ALSO
	Open(), FRead(), FPutc(), Close()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(BADDR(fh));
    ASSERT_VALID_PTR(block);
    ASSERT(blocklen > 0);
    ASSERT(numblocks > 0);

    ULONG   written;
    const UBYTE  *ptr;

    ptr = block;

    SetIoErr(0);

    for(written = 0; written < numblocks; written++)
    {
        if (FWriteChars(fh, ptr, blocklen, DOSBase) != blocklen)
        {
            return(EOF);
        }
        else
        {
            ptr += blocklen;
        }
    }
    
    return written;

    AROS_LIBFUNC_EXIT
} /* FWrite */


LONG
FWriteChars(BPTR file, CONST UBYTE* buffer, ULONG length, struct DosLibrary *DOSBase)
{
    ASSERT_VALID_PTR(BADDR(file));
    ASSERT_VALID_PTR(buffer);

    /* Get pointer to filehandle. */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Check if file is in write mode */
    if (!(fh->fh_Flags & FHF_WRITE))
    {
        if (fh->fh_Pos < fh->fh_End)
        {
            /* Read mode. Try to seek back to the current position. */
            if (Seek(file, fh->fh_Pos - fh->fh_End, OFFSET_CURRENT) < 0)
            {
                fh->fh_Pos = fh->fh_End = 0;
        
                return EOF;
            }
        }
        
        /* Is there a buffer? */
        if (fh->fh_Buf == BNULL)
        {
            if (vbuf_alloc(fh, NULL, IOBUFSIZE) == NULL)
            {
                return(EOF);
            }
        }
    
        /* Prepare buffer */
        fh->fh_Flags |= FHF_WRITE;

        fh->fh_Pos = 0;
        fh->fh_End = fh->fh_BufSize;
    }

        LONG
    written = -1;
    
    if (fh->fh_Flags & FHF_NOBUF)
    {
            LONG
        goOn = TRUE;

        if (fh->fh_Pos != 0)
        {
            goOn = Flush(file);
        }

        if (goOn)
        {
            written = Write(file, buffer, length);
        }
    }
    else
    {
        for (written = 0; written < length; ++written)
        {
            /* Check if there is still some space in the buffer */
            if (fh->fh_Pos >= fh->fh_End)
            {
                if (!Flush(file))
                {
                    written = -1;
                    break;
                }
            }

            /* Write data */
            ((UBYTE *)BADDR(fh->fh_Buf))[fh->fh_Pos++] = buffer[written];
            
            if (fh->fh_Flags & FHF_LINEBUF
                && (buffer[written] == '\n' || buffer[written] == '\r'
                    || buffer[written] == '\0'))
            {
                if (!Flush(file))
                {
                    written = -1;
                    break;
                }
            }
        }
    }
    
    return(written);
}
