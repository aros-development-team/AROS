/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <devices/usbhardware.h>
#include <exec/memory.h>

#include <proto/exec.h>

#if __WORDSIZE == 64

APTR usbGetBuffer(APTR data, ULONG len, UWORD dir)
{
    APTR ret = data;

    if (((IPTR) data + len - 1) >> 32)
    {
        ret = AllocVec(len, MEMF_31BIT | MEMF_PUBLIC);

        if (ret && (dir == UHDIR_OUT))
            CopyMem(data, ret, len);
    }

    return ret;
}

void usbReleaseBuffer(APTR buffer, APTR data, ULONG len, UWORD dir)
{
    if (buffer && (buffer != data))
    {
        if (len && (dir == UHDIR_IN))
            CopyMem(buffer, data, len);

        FreeVec(buffer);
    }
}

#endif
