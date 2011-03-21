#include "screen.h"
#include "support.h"

static void RenderChar(unsigned char c, unsigned char *ptr)
{
    unsigned int x, y, b;
    const unsigned char *font = &fontData[c * fontHeight];

    for (y = 0; y < fontHeight; y++)
    {
        unsigned char in = *font++;
        unsigned char *p = ptr;

        for (x = 0; x < fontWidth; x++)
        {
            unsigned char val = (in & (0x80 >> x)) ? 0xFF : 0;

	    for (b = 0; b < bpp; b++)
	    	*p++ = val;
        }
        ptr += pitch;
    }
}

void gfxClear(void)
{
    __bs_bzero(fb, pitch * hc * fontHeight);
}

void gfxPutc(char chr)
{
    unsigned char *ptr;

    if (chr == '\n')
    {
    	x = 0;
    	y++;
    }
    else
    {
        ptr = fb + pitch * y * fontHeight + bpp * x * fontWidth;

        RenderChar(chr, ptr);
        x++;

        if (x == wc)
        {
            x = 0;
            y++;
        }
    }
    if (y >= hc)
    {
        y = hc - 1;

	ptr = __bs_memcpy(fb, fb + pitch * fontHeight, pitch * y * fontHeight);
	__bs_bzero(ptr, pitch * fontHeight);
    }
}
