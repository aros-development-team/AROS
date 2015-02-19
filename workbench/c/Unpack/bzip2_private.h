#ifndef PKG_BZIP2_PRIVATE_H
#define PKG_BZIP2_PRIVATE_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#ifdef __AROS__
#include <dos/bptr.h>
#endif

#include <bzlib.h>

struct bzFile
{
    BPTR      bzf_File;
    bz_stream bzf_Stream;
    APTR      bzf_Buffer;
    LONG      bzf_BufferAmount;
};

#endif /* PKG_BZIP2_PRIVATE_H */
