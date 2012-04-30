/*
	Data for "list_writeprotected" Image
*/

#include <exec/types.h>
#include <intuition/intuition.h>

UWORD __chip list_writeprotectedData[16] =
{
	0x0000,0x0000,0x0600,0x0900,0x1100,0x2200,0x2600,0x1B00,
	0x0180,0x00C0,0x0160,0x00B0,0x0018,0x0030,0x0000,0x0000
};

struct Image list_writeprotected =
{
	0, 0,		/* LeftEdge, TopEdge */
	16, 16, 1,	/* Width, Height, Depth */
	list_writeprotectedData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};
