/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Enables writecombining for the Vesa FB
    Lang: English
*/
/*****************************************************************************

    NAME

        Vbemtrr

    FORMAT

        Vbemtrr
                
    TEMPLATE

    LOCATION

        Workbench:C/
           
    FUNCTION

    INPUTS

    RESULT

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/exec.h>
#include <aros/bootloader.h>
#include <proto/bootloader.h>
#include <stdio.h>

#define MTRRphysBase_MSR(reg) (0x200 + 2 * (reg))
#define MTRRphysMask_MSR(reg) (0x200 + 2 * (reg) + 1)

static __inline__
void wrmsr(ULONG reg, ULONG high, ULONG low)
{
    __asm__ __volatile__
    (
        "wrmsr\n"
        :
        : "a" (low), "d" (high), "c" (reg)
    );
}

static __inline__
void do_variable_mtrr(ULONG reg, APTR base, ULONG size, UBYTE type)
{
    APTR oldSysStack = SuperState();

    wrmsr(MTRRphysBase_MSR(reg), 0, ((ULONG)base  & 0xFFFFF000) | type);
    wrmsr(MTRRphysMask_MSR(reg), 0, (-size & 0xFFFFF000) | 0x800);

    UserState(oldSysStack);
}

APTR BootLoaderBase;

int main(void)
{
   struct VesaInfo *vi;

    if ((BootLoaderBase = OpenResource("bootloader.resource")))
    {
        if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)))
	{
            printf("Base   = %p\n", vi->FrameBuffer);
	    printf("Size   = %d kb\n", vi->FrameBufferSize);
            printf("Width  = %ld\n", vi->XSize);
            printf("Height = %ld\n", vi->YSize);
            printf("BxL    = %ld\n", vi->BytesPerLine);
            printf("Size   = %ld\n", vi->BytesPerLine * vi->YSize);

            do_variable_mtrr(7, vi->FrameBuffer, (vi->FrameBufferSize * 1024), 0x01);

	    printf("Enabled WriteCombining for the VESA framebuffer\n");

            return 0;
        }

        puts("Couldn't get VESA info");
    }
    else
        puts("Couldn't open bootloader.resource");

    return 1;
}
