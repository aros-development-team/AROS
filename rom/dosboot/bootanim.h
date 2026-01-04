
#include <oop/oop.h>

#include "dosboot_intern.h"

struct AnimData
{
    struct Library      *OOPBase;
    OOP_Object          *gfxhidd;
    UBYTE               **framedata;

    IPTR                ad_State;
    UWORD               x;
    UWORD               y;
    ULONG               framecnt;
    ULONG               tick;
    UBYTE               frame;
};

// reserve 8 bits for the main bootanim code
#define STATEFLAG_CORE          8

void WriteChunkRegion(struct DOSBootBase *DOSBootBase,
                                    struct RastPort *rp, UWORD x, UWORD y, UWORD width,
                                    UBYTE *data, UWORD regx, UWORD regy, UWORD regwidth, UWORD regheight);
