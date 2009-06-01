#ifndef GARSHNELIB_H
#define GARSHNELIB_H

#include <exec/types.h>
#include <graphics/text.h>

#ifndef min
#define min( x, y ) ( (x) < (y) ? (x) : (y) )
#endif

typedef struct _Triplet
{
	ULONG Red;
	ULONG Green;
	ULONG Blue;
} Triplet;

#define CastAndShift( x ) (( SPFix( SPMul( x, SPFlt( Colors )))) << Shift )

#endif
