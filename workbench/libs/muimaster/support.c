#include <proto/graphics.h>

#include "support.h"

/* check if region is entirely within given bounds */
int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height)
{
	if ((left < r->bounds.MinX) && (left + width  - 1 > r->bounds.MaxX)
	 && (top  < r->bounds.MinY) && (top  + height - 1 > r->bounds.MaxY))
		return 1;

	return 0;
}

