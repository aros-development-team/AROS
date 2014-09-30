/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <hardware/vbe.h>
#include <utility/tagitem.h>

#include <bootconsole.h>
#include <stdarg.h>

int __vcformat(void *data, int(*outc)(int, void *), const char * format, va_list args);
int kprintf(const char *format, ...);

void __startup start(struct TagItem *tags)
{
    struct vbe_mode *vbemode = NULL;

    fb_Mirror = (void *)0x100000;

    con_InitTagList(tags);

    kprintf("Test module succesfully started\n");

    kprintf("Taglist at 0x%p:\n", tags);
    for (; tags->ti_Tag != TAG_DONE; tags++)
    {
    	kprintf("0x%08lX 0x%p\n", tags->ti_Tag, tags->ti_Data);

    	switch (tags->ti_Tag)
    	{
    	case KRN_VBEModeInfo:
	    vbemode = (struct vbe_mode *)tags->ti_Data;
	    break;
	}
    }

    if (vbemode)
    {
    	kprintf("VBE mode structure at 0x%p\n", vbemode);
    	kprintf("Mode : %dx%dx%d\n", vbemode->x_resolution, vbemode->y_resolution, vbemode->bits_per_pixel);
    	kprintf("Base : 0x%08X\n", vbemode->phys_base);
    	kprintf("Pitch: %u\n", vbemode->bytes_per_scanline);
    	kprintf("Flags: 0x%08X\n", vbemode->mode_attributes);
    }

    for (;;);
}

static int kputc(int c, void *data)
{
    con_Putc(c);
    return 1;
}

int kprintf(const char *format, ...)
{
    va_list ap;
    int res;

    va_start(ap, format);
    res = __vcformat(NULL, kputc, format, ap);
    va_end(ap);

    return res;
}

#ifdef __arm__

/*
 * ARM bootconsole doesn't have own serial port code because
 * no known ARM hardware uses PC-compatible serial ports.
 */
void serial_Init(char *opts)
{
}

void serial_Putc(char chr)
{
}

#endif
