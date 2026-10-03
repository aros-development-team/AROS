/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

*/
#include "dos_intern.h"

#include <aros/debug.h>

static LONG write_direct(BPTR file, CONST UBYTE *buffer, ULONG length,
                        struct DosLibrary *DOSBase);


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
        Buffered write of a number of blocks to a stream.
        May write fewer blocks than requested.

    INPUTS
        fh        - Write to this file
        block     - The data begins here
        blocklen  - number of bytes per block.  Must be > 0.
        numblocks - number of blocks to write.  Must be > 0.

    RESULT
        The number of blocks written to the file or EOF on error. IoErr()
        gives additional information in case of an error.

    NOTES
        Some releases of AmigaOS may not clear IoErr(), while AROS
        does. For full backwards compatibility, you may want to call
        SetIoErr(0L) before FWrite() if you need to be able to check
        the error code.

    EXAMPLE

    BUGS

    SEE ALSO
        Open(), FRead(), FPutc(), Close()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(BADDR(fh));
    ASSERT_VALID_PTR(block);
    ASSERT(blocklen > 0);
    ASSERT(numblocks > 0);

    ULONG   written = 0;
    const UBYTE  *ptr;

    ptr = block;

    SetIoErr(0);

    if (blocklen == 0 || numblocks == 0)
        return 0;

    while (written < numblocks)
    {
        ULONG blocks = numblocks - written;
        ULONG length;

        /* Batch blocks without overflowing the product or the signed byte
         * count. A single oversized block is split into smaller writes.
         */
        if (blocklen <= 0x7FFFFFFFUL)
        {
            ULONG maxblocks = 0x7FFFFFFFUL / blocklen;
            if (blocks > maxblocks)
                blocks = maxblocks;
            length = blocks * blocklen;
        }
        else
        {
            blocks = 1;
            length = blocklen;
        }

        while (length > 0)
        {
            ULONG chunk = length > 0x7FFFFFFFUL ? 0x7FFFFFFFUL : length;
            if (FWriteChars(fh, ptr, chunk, DOSBase) != (LONG)chunk)
                return EOF;
            ptr += chunk;
            length -= chunk;
        }
        written += blocks;
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

    if (fh == NULL)
        return EOF;

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
            written = write_direct(file, buffer, length, DOSBase);
        }
    }
    else if (!(fh->fh_Flags & FHF_LINEBUF) && length >= (ULONG)fh->fh_End)
    {
        /* At least a buffer's worth: write it with one packet. Flush()
         * also moves an append-mode handle to the end of the file. */
        if ((fh->fh_Pos == 0 && !(fh->fh_Flags & FHF_APPEND)) || Flush(file))
            written = write_direct(file, buffer, length, DOSBase);
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

static LONG write_direct(BPTR file, CONST UBYTE *buffer, ULONG length,
                        struct DosLibrary *DOSBase)
{
    ULONG written = 0;

    while (written < length)
    {
        ULONG chunk = length - written;
        LONG size;

        if (chunk > 0x7FFFFFFFUL)
            chunk = 0x7FFFFFFFUL;
        size = Write(file, buffer + written, chunk);
        /* A zero-length result makes no progress; do not loop forever. */
        if (size <= 0)
            return EOF;
        written += size;
    }

    return written;
}
