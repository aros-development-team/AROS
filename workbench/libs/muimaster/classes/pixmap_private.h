#ifndef _PIXMAP_PRIVATE_H_
#define _PIXMAP_PRIVATE_H_

/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

struct Pixmap_DATA
{
    LONG leftOffset;
    LONG topOffset;
    APTR data;
    LONG width;
    LONG height;
    ULONG format;
    APTR clut;
    ULONG alpha;
    ULONG compression;
    ULONG compressedSize;
    ULONG screenDepth;
    APTR uncompressedData;
    APTR ditheredData;
    APTR ditheredMask;
    LONG ditheredPenMap[256];
    struct BitMap *ditheredBitmap;
};

#endif /* _PIXMAP_PRIVATE_H_ */
