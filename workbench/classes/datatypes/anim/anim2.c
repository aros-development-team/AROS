
/*
**
** $Id$
**  anim.datatype 41.8
**
** These are "generic" software implementations of the unpacking code
** and are meant for reference. Arch specific "optimised" versions should be set
** in the class init code (see classbase.c)
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

// ANIM 2
LONG generic_unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    DFORMATS("[anim.datatype] %s()\n", __func__)

    return 0;
}
