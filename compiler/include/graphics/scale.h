#ifndef	GRAPHICS_SCALE_H
#define	GRAPHICS_SCALE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

/* BitScaleArgs structure used by BitMapScale() */

struct BitScaleArgs {
  UWORD          bsa_SrcX;
  UWORD          bsa_SrcY;		
  UWORD          bsa_SrcWidth;
  UWORD          bsa_SrcHeight;
  UWORD          bsa_XSrcFactor;
  UWORD          bsa_YSrcFactor;
  UWORD          bsa_DestX;
  UWORD          bsa_DestY;
  UWORD          bsa_DestWidth;
  UWORD          bsa_DestHeight;
  UWORD          bsa_XDestFactor;
  UWORD          bsa_YDestFactor;
  struct BitMap *bsa_SrcBitMap;
  struct BitMap *bsa_DestBitMap;
  ULONG          bsa_Flags;
  UWORD          bsa_XDDA;
  UWORD          bsa_YDDA;
  LONG           bsa_Reserved1;
  LONG           bsa_Reserved2;
};


#endif     /* GRAPHICS_SCALE_H */
