/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define CLIP(x) ((x)>0xff ? 0xff : ((x)<0x00 ? 0x00 : (x)))
#define MOD2(x) (((x)+1)&~1)
#define MOD16(x) (((x)+15)&~15)

struct HistEntry
{
 unsigned int Count;
 unsigned long Red;
 unsigned long Green;
 unsigned long Blue;
};

BOOL AllocSrcBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes );
BOOL AllocDestBM( struct Picture_Data *pd );
void FreeSource( struct Picture_Data *pd );
void FreeDest( struct Picture_Data *pd );
void InitGreyColTable( struct Picture_Data *pd );
void InitRGBColTable( struct Picture_Data *pd );
BOOL ConvertBitmap2Chunky( struct Picture_Data *pd );
BOOL ConvertChunky2Bitmap( struct Picture_Data *pd );
BOOL CreateMaskPlane( struct Picture_Data *pd );

BOOL ConvertTC2TC( struct Picture_Data *pd );
BOOL ConvertCM2TC( struct Picture_Data *pd );
BOOL ConvertCM2CM( struct Picture_Data *pd );
BOOL ConvertTC2CM( struct Picture_Data *pd );
