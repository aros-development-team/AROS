#include <exec/types.h>
#include <graphics/view.h>


/*
** In case the representation of colors in the ColorTable of the color map
** are changed then this here should be the place where to change the
** algorithms.
*/

ULONG color_distance(struct ColorMap * cm,
                     ULONG r,
                     ULONG g,
                     ULONG b,
                     ULONG index)
{
  /* 
  ** I am assuming 24 bit colors that are represented in the color map as
  ** follows:
  ** cm->ColorTable is a pointer to an array of UWORDs where every
  **     UWORD contains the most significant 4 bits of each color.
  ** cm->LowColorBits is a pointer to an array of UWORDs where every
  **     UWORD contains the least significant 4 bits of each color.
  ** for example a color r=12, g=34, b=56 would be represented as:
  **
  **   cm->ColorTable[x]   = 0x0135
  **   cm->LowColorBits[x] = 0x0246
  */
  
  ULONG dr,dg,db;
  UWORD c1,c2;
  c1 = ((UWORD *)cm->ColorTable)[index];
  c2 = ((UWORD *)cm->LowColorBits)[index];
  
  dr = (r >> (32-8)) - (((c1 >> 4) & 0x00f0) | (c2 >> 8));
  dg = (g >> (32-8)) - (((c1 >> 0) & 0x00f0) | (c2 >> 4));
  db = (b >> (32-8)) - (((c1 << 4) & 0x00f0) | (c2 >> 0));
   
  return dr*dr+dg*dg+db*db;
}


/*
** Test whether the entry in the color map equals the given
** color
*/
BOOL color_equal(struct ColorMap * cm,
                 ULONG r,
                 ULONG g,
                 ULONG b,
                 ULONG index)
{
  if ( ((UWORD *)cm->ColorTable)  [index] == (((r >> 20) & 0x0f00) |
                                              ((g >> 16) & 0x00f0) |
                                              ((b >> 12) & 0x000f))  &&
       ((UWORD *)cm->LowColorBits)[index] == (((r >> 16) & 0x0f00) |
                                              ((g >> 12) & 0x00f0) |
                                              ((b >>  8) & 0x000f)) )
    return TRUE;
  else
    return FALSE;
}


/*
** Set an entry in the color map
*/
VOID color_set(struct ColorMap * cm,
               ULONG r,
               ULONG g,
               ULONG b,
               ULONG index)
{
    ((UWORD *)cm->ColorTable)[index]   = ((r >> 20) & 0x0f00) |
                                         ((g >> 16) & 0x00f0) |
                                         ((b >> 12) & 0x000f);

    ((UWORD *)cm->LowColorBits)[index] = ((r >> 16) & 0x0f00) |
                                         ((g >> 12) & 0x00f0) |
                                         ((b >>  8) & 0x000f);
}
