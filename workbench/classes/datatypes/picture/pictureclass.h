/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct Picture_Data
{
 /*
  *  public entries, accessible with SET/GET
  */
 ULONG                 Precision;
 ULONG                 ModeID;
 struct BitMapHeader   bmhd;
 struct BitMap        *SrcBM; /* PDTA_BitMap and PDTA_ClassBM */
 ULONG                 SrcColRegs[768]; /* PDTA_CRegs */
 struct BitMap        *DestBM;
 ULONG                 DestColRegs[768]; /* PDTA_GRegs */
 struct Screen        *DestScreen;
 struct ColorRegister  ColMap[256];
 UBYTE                 ColTable[256];
 UBYTE                 ColTable2[256];
 UWORD                 NumColors;
 UWORD                 NumAlloc;
 UBYTE                 SparseTable[256];
 UWORD                 NumSparse;
 Point                 Grab;
 UWORD                 MaxDitherPens;
 UWORD                 DitherQuality;
 UWORD                 ScaleQuality;
 BOOL                  FreeSource;
 BOOL                  Remap;
 BOOL                  UseFriendBM;
 /*
  *  private entries
  */
 UBYTE                 *SrcBuffer;
 ULONG                 SrcWidth;
 ULONG                 SrcWidthBytes;
 ULONG                 SrcHeight;
 LONG                  SrcPixelFormat;
 UWORD                 SrcPixelBytes;
        
 UWORD                 DestDepth;
 UBYTE                 *DestBuffer;
 ULONG                 DestWidth;
 ULONG                 DestWidthBytes;
 ULONG                 DestHeight;
 LONG                  DestPixelFormat;
 UWORD                 DestPixelBytes;
        
 ULONG                 ColTableXRGB[256];
        
 BOOL                  TrueColorSrc;
 BOOL                  TrueColorDest;
 BOOL                  Layouted;
 BOOL                  KeepSrcBM;
 BOOL                  UseBM;
 BOOL                  UseCM;
};
