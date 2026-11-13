/*
    Copyright (C) 2016-2017, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/graphics.h>

int main(int argc, char **argv)
{
    struct BitMap *bitmap = NULL;
    ULONG flags = BMF_CLEAR | BMF_MINPLANES;
    UBYTE depth;

    for (depth = 1; depth < 33; depth ++)
    {
        if ((depth > 8) &&
            ((depth != 15)  || (depth != 16) || (depth != 24) || (depth != 32)))
            continue;

        if ((bitmap = AllocBitMap((44 << 3), 220, depth, flags, NULL )) != NULL)
        {
            bug("%dbit BitMap @ 0x%p\n", depth, bitmap);
            FreeBitMap(bitmap);
            bug("Freed\n");
        }
        else
        {
            bug("Failed to allocate %dbit BitMap\n", depth);
        }
    }
    return 0;
}
