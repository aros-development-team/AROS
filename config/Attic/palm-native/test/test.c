#include "../registers.h"

void drawlinehoriz(int y);

static int global = 0;

int main(void)
{
	WREG_W(CSA) = RREG_W(CSA) & 0x01ff;
	drawlinehoriz(30);
	if (10 == global)
		return 1;
	return 0;
}

void drawlinehoriz(int y)
{	
	ULONG lssa = RREG_L(LSSA);
	ULONG height = RREG_W(LYMAX);
	ULONG width = RREG_W(LXMAX)>>3;
	ULONG x = 0;

#if 0	
	if (y < 0 || y >= height)
		return;
#endif
	
	while (x <= width) {
		*(char *)(lssa+((width+1) * y)+x) = 0xff;
		x++;
	}
	
}
