/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#ifndef BUFFER_PROTOS_H
#define BUFFER_PROTOS_H

#include <exec/types.h>

#if (__WORDSIZE == 64)

APTR usbGetBuffer(APTR data, ULONG len, UWORD dir);
void usbReleaseBuffer(APTR buffer, APTR data, ULONG len, UWORD dir);

#else

/* On 32-bit systems we don't need mirroring */

#define usbGetBuffer(data, len, dir) ({ (void)(len); (void)(dir); (data);})
#define usbReleaseBuffer(buffer, data, len, dir) \
    do { (void)(buffer); (void)(data); (void)(len); (void)(dir); } while (0)

#endif

#endif /* BUFFER_PROTOS_H */
