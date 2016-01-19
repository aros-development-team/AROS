
/*
**
**  $VER: anim.c 1.12 (12.11.97)
**  anim.datatype 1.12
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct ClassBase;
struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"

// ANIM-I
LONG generic_unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    DFORMATS("[anim.datatype] %s()\n", __func__)
    return 0;
}
