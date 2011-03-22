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
    /* Just clear the whole framebuffer and don't care */
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
    	unsigned int i;
    	void *dest = fb;
    	void *src = fb + pitch * fontHeight;
    	unsigned int len = bpp * wc * fontWidth;

        y = hc - 1;

	/*
	 * Scroll the screen line-by-line.
	 * We don't do it in one memcpy() call (however we could)
	 * because on some hardware (Mac) lines include huge padding
	 * (pitch == 8192 for 1280x1024x32 framebuffer).
	 * Copying those empty regions slows down the operation significantly.
	 */
	for (i = 0; i < y * fontHeight; i++)
	{
	    __bs_memcpy(dest, src, len);
	    src  += pitch;
	    dest += pitch;
	}
	
	/* Clear the bottom line, again line-by-line */
	for (i = 0; i < fontHeight; i++)
	{
	    __bs_bzero(dest, len);
	    dest += pitch;
	}
    }
}
