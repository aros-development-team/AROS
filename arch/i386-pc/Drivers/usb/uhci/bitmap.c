#include "bitmap.h"

LONG allocAddress(UBYTE *bitmap) {
LONG num;
LONG byte;
LONG bit;

	for (num=0;num<BITMAP_SIZE;num++)
	{
		byte = num / 8;
		bit = num % 8;
		if (!(bitmap[byte] & (1<<bit)))
		{
			bitmap[byte] |= 1<<bit;
			return num;
		}
	}
	return 0;
}

void freeAddress(UBYTE *bitmap, LONG num) {
LONG byte;
LONG bit;

	byte = num/8;
	bit = num % 8;
	bitmap[byte] &= ~(1<<bit);
}

