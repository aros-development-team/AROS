/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

struct Picture_Data
{
 ULONG                 Precision;
 ULONG                 ModeID;
 struct BitMapHeader  *bmhd;
 struct BitMap        *bm;
 struct ColorRegister *ColMap;
 ULONG                *ColRegs;
 ULONG                *GRegs;
 UBYTE                *ColTable;
 UBYTE                *ColTable2;
 ULONG                 Allocated;
 UWORD                 NumColors;
 UWORD                 NumAlloc;
 BOOL                  Remap;
 struct Screen        *TheScreen;
 BOOL                  FreeSourceBitMap;
 Point                *Grab;
 struct BitMap        *DestBM;
 struct BitMap        *ClassBM;
 UWORD                 NumSparse;
 UBYTE                *SparseTable;
};
