#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <timer_intern.h>
#include "timervblank.h"

/* Define this in timer_platform.h in order to make use of this code */
#ifdef USE_VBLANK_INT

/* exec.library VBlank interrupt handler  */
AROS_INTH1(VBlankInt, struct TimerBase *, TimerBase)
{
    AROS_INTFUNC_INIT

    /* UpdateEClock and process VBlank timer*/
    EClockUpdate(TimerBase);
    handleVBlank(TimerBase, SysBase);

    /* exec should continue with other servers */
    return 0;

    AROS_INTFUNC_EXIT
}

int vblank_Init(struct TimerBase *LIBBASE)
{
    LIBBASE->tb_VBlankInt.is_Node.ln_Pri  = 0;
    LIBBASE->tb_VBlankInt.is_Node.ln_Type = NT_INTERRUPT;
    LIBBASE->tb_VBlankInt.is_Node.ln_Name = LIBBASE->tb_Device.dd_Library.lib_Node.ln_Name;
    LIBBASE->tb_VBlankInt.is_Code         = (VOID_FUNC)VBlankInt;
    LIBBASE->tb_VBlankInt.is_Data	  = LIBBASE;

    AddIntServer(INTB_VERTB, &LIBBASE->tb_VBlankInt);
    return TRUE; /* We can't fail */
}

/*
 * We intentionally don't ADD2INITLIB() here because some architectures may
 * want to use VBlank interrupt conditionally.
 */

static int vblank_Expunge(struct TimerBase *base)
{
    /* ln_Succ will ne non-empty if this Node was added to a list */
    if (base->tb_VBlankInt.is_Node.ln_Succ)
	RemIntServer(INTB_VERTB, &base->tb_VBlankInt);

    return TRUE;
}

ADD2EXPUNGELIB(vblank_Expunge, 0)

#endif
