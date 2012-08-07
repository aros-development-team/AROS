/*
	Data for "list_disk" Image
*/

#include <exec/types.h>
#include <intuition/intuition.h>

UWORD __chip list_diskData[16] =
{
	0x0000,0x0000,0x1FF8,0x300C,0x306C,0x306C,0x300C,0x3FFC,
	0x3FFC,0x300C,0x354C,0x32AC,0x300C,0x3FFC,0x0000,0x0000
};

struct Image list_disk =
{
	0, 0,		/* LeftEdge, TopEdge */
	16, 16, 1,	/* Width, Height, Depth */
	list_diskData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};
