/*
	Data for "list_plugin" Image
*/

#include <exec/types.h>
#include <intuition/intuition.h>

UWORD __chip list_pluginData[16] =
{
	0xFF00,0xFF00,0xFFB0,0xFFF8,0xFFF8,0xFFB0,0xFF00,0xFF00,
	0xFF00,0xF200,0xE000,0xE000,0xF200,0xFF00,0xFF00,0xFF00
};

struct Image list_plugin =
{
	0, 0,		/* LeftEdge, TopEdge */
	16, 16, 1,	/* Width, Height, Depth */
	list_pluginData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};
