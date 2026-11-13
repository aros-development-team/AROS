/*
	Data for "list_checkmark" Image
*/

#include <exec/types.h>
#include <intuition/intuition.h>

UWORD __chip list_checkmarkData[16] =
{
	0x0000,0x0000,0x0000,0x0000,0x0008,0x001C,0x0038,0x1070,
	0x38E0,0x1DC0,0x0F80,0x0700,0x0200,0x0000,0x0000,0x0000
};

struct Image list_checkmark =
{
	0, 0,		/* LeftEdge, TopEdge */
	16, 16, 1,	/* Width, Height, Depth */
	list_checkmarkData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};
