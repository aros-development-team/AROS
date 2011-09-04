#include <hardware/vbe.h>

#include <string.h>

#include "screen.h"
#include "vesa.h"

unsigned char setupVesa(char *str, long *modePtr)
{
    char *vesa = strstr(str, "vesa=");
    short r;
    unsigned char palwidth = 0;
    BOOL prioritise_depth = FALSE, set_refresh = FALSE;

    if (vesa)
    {
        long x=0, y=0, d=0, vfreq=0;
        long mode, setmode;
        unsigned long vesa_size = (unsigned long)&_binary_vesa_size;
        void *vesa_start = &_binary_vesa_start;
 
	vesa += 5;

        while (*vesa >= '0' && *vesa <= '9')
            x = x * 10 + *vesa++ - '0';
        if (*vesa++ == 'x')
        {
            while (*vesa >= '0' && *vesa <= '9')
                y = y * 10 + *vesa++ - '0';
            if (*vesa++ == 'x')
            {
                while (*vesa >= '0' && *vesa <= '9')
                    d = d * 10 + *vesa++ - '0';
            }
            else
                d = 32;
        }
        else
            d = x, x = 10000, y = 10000, prioritise_depth = TRUE;

        /* Check for user-set refresh rate */
        if (*(vesa - 1) == '@')
        {
            while (*vesa >= '0' && *vesa <= '9')
                vfreq = vfreq * 10 + *vesa++ - '0';
            set_refresh = TRUE;
        }
        else
            vfreq = 60;

        kprintf("[VESA] module (@ %p) size=%ld\n", &_binary_vesa_start, (long)&_binary_vesa_size);
        memcpy((void *)0x1000, vesa_start, vesa_size);
        kprintf("[VESA] Module installed\n");

        kprintf("[VESA] BestModeMatch for %ldx%ldx%ld = ", x, y, d);
        mode = findMode(x, y, d, vfreq, prioritise_depth);

        getModeInfo(mode);
	kprintf("%lx\n",mode);

	if (modeinfo->mode_attributes & 0x80)
	    setmode = mode | 0x4000;
	else
	    setmode = mode;
	r = setVbeMode(setmode, set_refresh);
        if (r == 0x004f)
	{
	    if (controllerinfo->capabilities & 0x01)
		paletteWidth(0x0800, &palwidth);
	    else
		palwidth = 6;

	    *modePtr = mode;
	}
	else
	    kprintf("[VESA] mode setting error: 0x%04X\n", r);
    }
    return palwidth;
}
