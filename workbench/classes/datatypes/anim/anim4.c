
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

// ANIM-4
LONG generic_unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG generic_unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}
