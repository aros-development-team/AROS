#ifndef VESAGFX_CURSOR_H
#define VESAGFX_CURSOR_H

#include <hidd/gfx.h>

struct MouseData {
	APTR shape;
	OOP_Object *oopshape;
	ULONG width;
	ULONG height;
	ULONG x;
	ULONG y;
	LONG visible;
};

#endif /* VESAGFX_CURSOR_H */
