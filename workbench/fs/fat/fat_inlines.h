/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef __FAT_HANDLER_INLINES_H
#define __FAT_HANDLER_INLINES_H

/* IO layer */

#include "cache.h"

static inline LONG FS_GetBlocks (ULONG n, UBYTE *dst, ULONG count) {
    struct cache_block *b;
    ULONG err, i;

    for (i = 0; i < count; i++) {
        if ((err = cache_get_block(glob->cache, glob->diskioreq->iotd_Req.io_Device, glob->diskioreq->iotd_Req.io_Unit, n+i, 0, &b)) != 0)
            return err;

        CopyMem(b->data, (APTR) ((UBYTE *) dst + i * glob->blocksize), glob->blocksize);

        cache_put_block(glob->cache, b, 0);
    }

    return 0;

    /*
    glob->diskioreq->iotd_Req.io_Command = CMD_READ;
    glob->diskioreq->iotd_Req.io_Data = dst;
    glob->diskioreq->iotd_Req.io_Offset = n * glob->blocksize;
    glob->diskioreq->iotd_Req.io_Length = count * glob->blocksize;
    glob->diskioreq->iotd_Req.io_Flags = IOF_QUICK;
    
    DoIO((struct IORequest *) glob->diskioreq);

    return glob->diskioreq->iotd_Req.io_Error;
    */
}

#endif

