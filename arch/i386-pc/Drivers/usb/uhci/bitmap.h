#ifndef BITMAP_H
#define BITMAP_H

#include <exec/types.h>

#define BITMAP_SIZE (128)

LONG allocAddress(UBYTE *);
void freeAddress(UBYTE *, LONG);

#endif
