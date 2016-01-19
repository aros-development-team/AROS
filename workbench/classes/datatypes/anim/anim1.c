
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

// ANIM-1
LONG generic_xorbm(struct AnimHeader *anhd, struct BitMap *bm, struct BitMap *deltabm )
{
    DFORMATS("[anim.datatype] %s()\n", __func__)

    return 0;
}
