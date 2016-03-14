
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

// ANIM-1
LONG generic_xorbm(struct AnimHeader *anhd, struct BitMap *bm, struct BitMap *deltabm )
{
    DFORMATS("[anim.datatype] %s()\n", __func__)

    if ((bm) && (deltabm))
    {
        ULONG           planesize = (ULONG)(bm -> BytesPerRow) * (ULONG)(bm -> Rows);
        ULONG           missing;
        ULONG           i;
        register ULONG  j;
        register ULONG  *bmp,                           /* bm planes                                    */
                        *deltabmp;                      /* deltabm planes                               */

        planesize = planesize / sizeof( ULONG );        /* bmp and deltabmp are ULONGs, not BYTES...    */
        missing   = planesize % sizeof( ULONG );        /* missing bytes                                */

        for( i = 0; i < bm->Depth; i++ )
        {
            j = planesize;

            bmp = (ULONG *)(bm -> Planes[ i ]);
            deltabmp = (ULONG *)(deltabm -> Planes[ i ]);

            while( j-- )
            {
                *bmp++ ^= *deltabmp++;
            }

            if( missing )
            {
                register UBYTE *bmpx = (UBYTE *)bmp;
                register UBYTE *deltabmpx = (UBYTE *)deltabmp;

                j = missing;

                while( j-- )
                {
                    *bmpx++ ^= *deltabmpx++;
                }
            }
        }
        return 0;
    }
    return 1;
}
