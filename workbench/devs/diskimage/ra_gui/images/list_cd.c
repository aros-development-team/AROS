/*
	Data for "list_cd" Image
*/

#include <exec/types.h>
#include <intuition/intuition.h>

UWORD __chip list_cdData[16] =
{
	0x0000,0x0000,0x07E0,0x0A90,0x1528,0x2804,0x3184,0x2A44,
	0x3244,0x2184,0x2804,0x1008,0x0810,0x07E0,0x0000,0x0000
};

struct Image list_cd =
{
	0, 0,		/* LeftEdge, TopEdge */
	16, 16, 1,	/* Width, Height, Depth */
	list_cdData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};
