/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

struct HistEntry
{
 unsigned int Count;
 unsigned long Red;
 unsigned long Green;
 unsigned long Blue;
};

BOOL AllocSrcBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes );
BOOL AllocDestBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes );
void FreeSource( struct Picture_Data *pd );
void FreeDest( struct Picture_Data *pd );

void ConvertTC2TC( struct Picture_Data *pd );
void ConvertCM2TC( struct Picture_Data *pd );
void ConvertCM2CM( struct Picture_Data *pd );
void ConvertTC2CM( struct Picture_Data *pd );

#if 0
unsigned int *MakeARGB(unsigned long *ColRegs, unsigned int Count);
unsigned int CountColors(unsigned int *ARGB, unsigned int Count);
int HistSort(const void *HistEntry1, const void *HistEntry2);
#endif
