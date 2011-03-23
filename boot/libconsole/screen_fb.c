#include <string.h>

#include "console.h"

/*
 * Framebuffer access can be extremely slow, so we use a scr_Mirror buffer.
 * Our scr_Mirror is a monochrome text-mode representation of the screen.
 * Unused positions are filled with zero bytes, this helps to determine
 * line lengths for faster scrolling.
 * We hope that machine's memory upper memory really starts after 1MB.
 * We are loaded at 2MB (see ldscript.lds).
 */
char *scr_Mirror = NULL;

/*
 * Calculate length of current line in the scr_Mirror buffer.
 * Similar to strlen() but takes into account maximum line length.
 */
static unsigned int lineLen(const char *s)
{
    int len;

    for (len = 0; len < scr_Width; len++)
    {
    	if (s[len] == 0)
    	    break;
    }

    return len;
}

/* Render a character at (xc, yc) */
static void RenderChar(unsigned char c, unsigned int xc, unsigned int yc)
{
    unsigned int x, y;
    const unsigned char *font = &fontData[c * fontHeight];
    void *ptr = scr_FrameBuffer + scr_BytesPerLine * yc * fontHeight + scr_BytesPerPix * xc * fontWidth;

    /* Store our character in the scr_Mirror buffer */
    if (scr_Mirror)
	scr_Mirror[scr_Width * yc + xc] = c;

    /* Render zero bytes as spaces (do not depend on particular font) */
    if (c == '\0')
    	c = ' ';

    /* Now render it on the screen */
    for (y = 0; y < fontHeight; y++)
    {
        unsigned char in = *font++;
        void *p = ptr;

        for (x = 0; x < fontWidth; x++)
        {
            /* Get pixel from the font data */
            int val = (in & 0x80) ? -1 : 0;

	    /* Draw the pixel. Do it in a single VRAM access, again to speed up */
	    switch (scr_BytesPerPix)
	    {
	    case 4:
	    	*((int *)p) = val;
	    	break;

	    case 2:
	    	*((short *)p) = val;
	    	break;

	    case 1:
	    	*((char *)p) = val;
	    	break;
	    }

	    p += scr_BytesPerPix;
	    in <<= 1;
        }
        ptr += scr_BytesPerLine;
    }
}

void gfxClear(void)
{
    void *ptr = scr_FrameBuffer;
    unsigned int i;

    /* Clear the framebuffer, line by line */
    for (i = 0; i < scr_Height * fontHeight; i++)
    {
    	memset(ptr, 0, scr_BytesPerPix * scr_Width * fontWidth);
    	ptr += scr_BytesPerLine;
    }

    /* Clear scr_Mirror buffer */
    memset(scr_Mirror, 0, scr_Width * scr_Height);
}

void gfxPutc(char chr)
{
    /*
     * If the output went past the last line, do nothing.
     * This means that we stop printing when we have no mirror buffer
     * and the whole screen if filled up.
     * This is good for displaying alerts.
     */
    if (scr_YPos >= scr_Height)
    	return;

    if (chr == '\n')
    {
    	scr_XPos = 0;
    	scr_YPos++;
    }
    else
    {
    	/*
    	 * Replace zero characters with '?', since zero bytes mark
    	 * unused space in the scr_Mirror buffer, and they would screw us up.
    	 */
    	if (chr == '\0')
    	    chr = '?';

    	/* Draw the character at current position */
	RenderChar(chr, scr_XPos, scr_YPos);

	/* Increment current column */
    	scr_XPos++;
	/* Reached end of line ? New line if so. */
    	if (scr_XPos == scr_Width)
    	{
            scr_XPos = 0;
            scr_YPos++;
    	}
    }
    if ((scr_YPos >= scr_Height) && scr_Mirror)
    {
        /* destLen contains length of line being erased */
    	unsigned int destLen = lineLen(scr_Mirror);
    	/* ptr contains address of line being scrolled */
    	char *ptr = scr_Mirror + scr_Width;
    	unsigned int xc, yc;

	/* Reset line number */
	scr_YPos = scr_Height - 1;

	/*
	 * Reprint the whole scr_Mirror (starting from the second line) at (0, 0).
	 * Update only used parts in order to speed up the scrolling.
	 */
        for (yc = 0; yc < scr_YPos; yc++)
        {
            /* Calculate length of the line being scrolled */
            unsigned int srcLen = lineLen(ptr);

	    /*
	     * The next line (being reprinted on top ot the current one)
	     * must completely cover it, so we must copy a minimum of 'destLen' bytes.
	     * Mirror buffer contains zero bytes at unused positions, they will be
	     * rendered as blank spaces, erasing the previous text.
	     */
	    if (srcLen > destLen)
	    	destLen = srcLen;

            for (xc = 0; xc < destLen; xc++)
            	RenderChar(ptr[xc], xc, yc);

	    /* Go to the next line in scr_Mirror buffer */
	    ptr += scr_Width;
            /* Source becomes destination */
            destLen = srcLen;
        }

	/* Clear the bottom line */
	for (xc = 0; xc < destLen; xc++)
	    RenderChar(0, xc, scr_YPos);
    }
}
