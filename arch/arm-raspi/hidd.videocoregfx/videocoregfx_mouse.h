#ifndef _VIDEOCOREGFX_MOUSE_H
#define _VIDEOCOREGFX_MOUSE_H

#include <hidd/graphics.h>

struct MouseData {
	APTR shape;
	OOP_Object *oopshape;
	ULONG width;
	ULONG height;
	ULONG x;
	ULONG y;
	LONG visible;
};

#endif /* _VIDEOCOREGFX_MOUSE_H */
