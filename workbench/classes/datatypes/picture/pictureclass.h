/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct Picture_Data
{
 ULONG                 Precision;
 ULONG                 ModeID;
 struct BitMapHeader   bmhd;
 struct BitMap        *bm;
 struct ColorRegister  ColMap[256];
 ULONG                 CRegs[768];
 ULONG                 GRegs[768];
 UBYTE                 ColTable[256];
 UBYTE                 ColTable2[256];
 UWORD                 NumColors;
 ULONG                 Allocated;
 UWORD                 NumAlloc;
 BOOL                  Remap;
 struct Screen        *TheScreen;
 BOOL                  FreeSourceBitMap;
 Point                 Grab;
 struct BitMap        *DestBM;
 struct BitMap        *ClassBM;
 UWORD                 NumSparse;
 UBYTE                 SparseTable[256];
 LONG                  PixelFormat;                  
 /*
  *  private entries
  */
 UBYTE                *ChunkyBuffer;
 UWORD                PixelSize;
 UWORD                pad;
 ULONG                CBWidth;
 ULONG                CBWidthBytes;
 ULONG                CBHeight;
 BOOL                 TrueColorSrc;
 BOOL                 TrueColorDest;
 BOOL                 BitmapMode;
 BOOL                 Remapped
};
