
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

LONG generic_unpackilbmbody( struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *data, ULONG len )
{
    const BYTE *src = (const BYTE *)data;
    //const BYTE *end = src + len;
    UBYTE nplanes = bmh->bmh_Depth, p;
    UWORD pitch = bm->BytesPerRow;
    UWORD out = 0;
    UWORD y, ofs;

    DFORMATS("[anim.datatype] %s()\n", __func__);

    // The mask plane is interleaved after the bitmap planes, so we need to count
    // it as another plane when reading.
    if (bmh->bmh_Masking == mskHasMask)
        nplanes += 1;

    for (y = 0; y < bmh->bmh_Height; ++y)
    {
        for (p = 0; p < nplanes; ++p)
        {
            if (p < bmh->bmh_Depth)
            {
                // Read data into bitplane
                if (bmh->bmh_Compression == cmpNone)
                {
                    memcpy(&bm->Planes[p][out], src, pitch);
                    src += pitch;
                }
                else for (ofs = 0; ofs < pitch;)
                {
                    if (*src >= 0)
                    {
                        memcpy(&bm->Planes[p][out + ofs], src + 1, *src + 1);
                        ofs += *src + 1;
                        src += *src + 2;
                    }
                    else
                    {
                        memset(&bm->Planes[p][out + ofs], src[1], -*src + 1);
                        ofs += -*src + 1;
                        src += 2;
                    }
                }
            }
            else
            {
                // This is the mask plane. Skip over it.
                if (bmh->bmh_Compression == cmpNone)
                {
                    src += pitch;
                }
                else for (ofs = 0; ofs < pitch;)
                {
                    if (*src >= 0)
                    {
                        ofs += *src + 1;
                        src += *src + 2;
                    }
                    else
                    {
                        ofs += -*src + 1;
                        src += 2;
                    }
                }
            }
        }
        out += pitch;
    }

    return 0;
}
