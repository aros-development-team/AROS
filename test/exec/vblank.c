/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>

#include <proto/dos.h>
#include <proto/exec.h>

int __nocommandline = 1;

static int counter = 0;

static AROS_INTH1(vblank_handler, APTR, mydata)
{
    AROS_INTFUNC_INIT

    (void)mydata;
    counter++;
    return 0;
    
    AROS_INTFUNC_EXIT

}

static struct Interrupt vblank_int =
{
    .is_Code = (APTR)vblank_handler
};

int main(void)
{
    AddIntServer(INTB_VERTB, &vblank_int);
    
    while (!(CheckSignal(SIGBREAKF_CTRL_C)))
    {
    	Printf("\rVBlank counter: %lu          ", counter);
    }
    
    RemIntServer(INTB_VERTB, &vblank_int);
    
    Printf("\nTerminated\n");

    return 0;
}
