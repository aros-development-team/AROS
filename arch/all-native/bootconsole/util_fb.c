#include <bootconsole.h>

void fb_SetMirror(char *addr)
{
    if (addr)
    {
	/*
	 * Don't use memcpy() here because libc memcpy() may use exec.library/CopyMem()
	 * which can be not available yet.
	 */
	unsigned int i;

    	for (i = 0; i < scr_Width * scr_Height; i++)
    	    addr[i] = fb_Mirror[i];
    }

    fb_Mirror = addr;
}
