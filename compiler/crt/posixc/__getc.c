/*
 * Copyright (C) 2022, The AROS Development Team. All rights reserved.

 * Define fdesc, and getcstream, before including this file.
 */

    FLUSHONREADCHECK

    c = FGetC(fdesc->fcb->handle);
    
    if (c == EOF)
    {
        c = IoErr();
        
        if (c)
        {
            errno = __stdc_ioerr2errno(c);
            getcstream->flags |= __POSIXC_STDIO_ERROR;
        }
        else
        {
            getcstream->flags |= __POSIXC_STDIO_EOF;
        }
        
        c = EOF;
    }

    return c;
