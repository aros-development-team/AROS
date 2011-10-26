#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>

#include <proto/dos.h>
#include <proto/exec.h>

int __nocommandline = 1;

static int counter = 0;

AROS_UFH4(static ULONG, vblank_handler,
	  AROS_UFHA(APTR, unused, A0),
	  AROS_UFHA(APTR, mydata, A1),
	  AROS_UFHA(APTR, self, A5),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    counter++;
    return 0;
    
    AROS_USERFUNC_EXIT

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
