/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphical framebuffer console.
*/

#include <bootconsole.h>
#include <string.h>

/*
 * Framebuffer access can be extremely slow, so we use a mirror buffer.
 * Our mirror is a monochrome text-mode representation of the screen.
 * Unused positions are filled with zero bytes, this helps to determine
 * line lengths for faster scrolling.
 * These variables need to survive accross warm reboot, so we explicitly place them in .data section.
 */
__attribute__((section(".data"))) char *fb_Mirror = NULL;

__attribute__((section(".data"))) static unsigned int fb_BytesPerLine = 0; /* Bytes per line  */
__attribute__((section(".data"))) static unsigned int fb_BytesPerPix  = 0; /* Bytes per pixel */

/*
 * Calculate length of current line in the fb_Mirror buffer.
 * Similar to strlen() but takes into account maximum line length.
 */
static unsigned int lineLen(const char *s)
{
    unsigned int len;

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
    void *ptr = scr_FrameBuffer + fb_BytesPerLine * yc * fontHeight + fb_BytesPerPix * xc * fontWidth;

    /* Store our character in the mirror buffer */
    if (fb_Mirror)
        fb_Mirror[scr_Width * yc + xc] = c;

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
            switch (fb_BytesPerPix)
            {
            case 4:
                *((int *)p) = val;
                break;

            case 3:
                /* qemu's truecolor modes are known to be 3 bytes per pixel */
                *((short *)p) = val;
                *((char *)p + 2) = val;
                break;

            case 2:
                *((short *)p) = val;
                break;

            case 1:
                *((char *)p) = val;
                break;
            }

            p += fb_BytesPerPix;
            in <<= 1;
        }
        ptr += fb_BytesPerLine;
    }
}

void * malloc(size_t size);

void fb_Init(unsigned int width, unsigned int height, unsigned int depth, unsigned int pitch)
{
    scr_Width       = width / fontWidth;
    fb_BytesPerPix  = depth >> 3;
    fb_BytesPerLine = pitch;

    fb_Mirror = malloc(scr_Width * (height + fontHeight - 1) / fontHeight);

    fb_Resize(height);
}

void fb_Resize(unsigned int height)
{
    scr_Height = height / fontHeight;

    fb_Clear();
}

void fb_Clear(void)
{
    void *ptr = scr_FrameBuffer;
    unsigned int i;

    /* Reset current position */
    scr_XPos = 0;
    scr_YPos = 0;

    /* Clear the framebuffer, line by line */
    for (i = 0; i < scr_Height * fontHeight; i++)
    {
        memset(ptr, 0, fb_BytesPerPix * scr_Width * fontWidth);
        ptr += fb_BytesPerLine;
    }

    /* Clear mirror buffer */
    if (fb_Mirror)
        memset(fb_Mirror, 0, scr_Width * scr_Height);
}

void fb_Putc(char chr)
{
    /* Ignore null bytes, they are output by formatting routines as terminators */
    if (chr == 0)
        return;

    if (chr == 0xFF)
    {
        fb_Clear();
        return;
    }

    /* Reached end of line ? New line if so. */
    if ((chr == '\n') || (scr_XPos >= scr_Width))
    {
        scr_XPos = 0;
        scr_YPos++;
    }

    if (scr_YPos >= scr_Height)
    {
        if (fb_Mirror)
        {
            /* destLen contains length of line being erased */
            unsigned int destLen = lineLen(fb_Mirror);
            /* ptr contains address of line being scrolled */
            char *ptr = fb_Mirror + scr_Width;
            unsigned int xc, yc;

            /* Reset line number */
            scr_YPos = scr_Height - 1;

            /*
             * Reprint the whole fb_Mirror (starting from the second line) at (0, 0).
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

                /* Go to the next line in fb_Mirror buffer */
                ptr += scr_Width;
                /* Source becomes destination */
                destLen = srcLen;
            }

            /* Clear the bottom line */
            for (xc = 0; xc < destLen; xc++)
                RenderChar(0, xc, scr_YPos);
        }
        else
        {
            /* We dont have a mirror buffer */
            memmove((void *)scr_FrameBuffer, (void *)scr_FrameBuffer + (fontHeight * (fb_BytesPerPix * scr_Width * fontWidth)), (((scr_Height - 1) * fontHeight) * (fb_BytesPerPix * scr_Width * fontWidth)));

            /* Clear last line on screen */
            memset((void *)(scr_FrameBuffer + (((scr_Height - 1) * fontHeight) * (fb_BytesPerPix * scr_Width * fontWidth))), 0, fb_BytesPerPix * scr_Width * fontWidth * fontHeight);
            scr_YPos = scr_Height - 1;
        }
    }

    if (chr == '\n')
        return;

    /* Draw the character at current position and increment current column */
    RenderChar(chr, scr_XPos++, scr_YPos);
}
