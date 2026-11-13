/*
    Copyright (C) 2009-2014, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

//#include <asm/mpc5200b.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/openfirmware.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/rtas.h>

#include "rtas_private.h"

static int rtas_init(struct RTASBase *RTASBase)
{
        int retval = TRUE;
        void *OpenFirmwareBase;
        void *key, *prop;

        D(bug("[RTAS] RTAS Init.\n"));

        OpenFirmwareBase = RTASBase->OpenFirmwareBase = OpenResource("openfirmware.resource");
        RTASBase->KernelBase = OpenResource("kernel.resource");

        /* Get some addresses from OF tree */
        if (OpenFirmwareBase)
        {
                key = OF_OpenKey("/rtas");
                if (key)
                {
                        prop = OF_FindProperty(key, "load_base");
                        if (prop)
                        {
                                void **reg = OF_GetPropValue(prop);
                                D(bug("[RTAS] RTAS loaded at %08x\n", *reg));
                                RTASBase->rtas_base = *reg;
                        }

                        prop = OF_FindProperty(key, "entry");
                        if (prop)
                        {
                                void **reg = OF_GetPropValue(prop);
                                D(bug("[RTAS] Entry at %08x\n", *reg));
                                RTASBase->rtas_entry = *reg;
                        }
                }
        }
        else
                retval = FALSE;


        return retval;
}

extern void return_entry();
extern void return_2();

/*****************************************************************************

    NAME */
        AROS_LH5(int32_t, RTASCall,

/*  SYNOPSIS */
                        AROS_LHA(const char *,  name,   A0),
                        AROS_LHA(uint32_t,              nargs,  D0),
                        AROS_LHA(uint32_t,              nrets,  D1),
                        AROS_LHA(uint32_t *,    output, A1),
                        AROS_LHA(va_list,               args,   A2),

/*  LOCATION */
                        struct RTASBase *, RTASBase, 1, Rtas)
/*  FUNCTION
        Invoke an Open Firmware RTAS service through the PowerPC kernel.

    INPUTS
        name - Non-NULL, NUL-terminated property name beneath /rtas.
        nargs - Number of uint32_t argument cells to consume from args.
        nrets - Number of return cells, including the primary status/result.
                nargs must be at most 16; nrets must be at most 16 - nargs.
        output - Writable array of at least nrets - 1 uint32_t cells when
                 nrets exceeds 1, or NULL to discard secondary results.
        args - Initialized va_list containing nargs arguments of type
               uint32_t, in the order required by the selected service.
               It is not read when nargs is zero.

    RESULT
        The first firmware return cell, interpreted as int32_t, or 0
        after a call requesting no return cells. Returns -1 without calling
        firmware if the property cannot be found or its token is zero.
        Firmware may itself return -1; the two cases are not distinguished.

        When nrets exceeds 1 and output is non-NULL, secondary cells are
        copied even if the primary status indicates failure. Their meaning
        depends on the service. output is unchanged when no call is made.

    NOTES
        Open rtas.resource before calling. The selected service defines
        argument values, return codes and effects. Services may change the
        clock, reboot or power off the machine and need not return.

        The wrapper uses one shared 16-cell argument/return area. Serialize
        all callers externally, including argument preparation. Interrupts
        are disabled only after that shared area has been filled and remain
        disabled through firmware execution and result copying. There is no
        timeout, cancellation or retry for a busy firmware status.

        Argument cells are passed unchanged. Addresses embedded in them must
        already meet the service's physical-address and lifetime requirements;
        only the wrapper's own call area is translated. output is used by
        the wrapper after firmware returns and is not retained. Treat args
        as consumed; use a separate va_copy when its state must be preserved,
        and perform the matching va_end in the caller.

    BUGS
        Counts, pointers and firmware property sizes are not validated.
        Exceeding the combined 16-cell capacity corrupts resource state.
        Resource initialization does not require a valid RTAS entry point.
        The result-copy path dereferences a physical return-area address
        after the kernel call and assumes that address is directly accessible.

    SEE ALSO
        openfirmware.resource/OF_FindProperty(), exec.library/ShutdownA()

*****************************************************************************/
{
        AROS_LIBFUNC_INIT

        void *OpenFirmwareBase = RTASBase->OpenFirmwareBase;
        void *KernelBase = RTASBase->KernelBase;

        int32_t token = 0;
        int32_t retval = -1;

        D(bug("[RTAS] Calling method '%s'\n", name));

        void *key = OF_OpenKey("/rtas");
        if (key)
        {
                void *prop = OF_FindProperty(key, name);

                if (prop)
                {
                        token = *(int32_t *)OF_GetPropValue(prop);
                }
        }

        if (token)
        {
                D(bug("[RTAS] Method '%s', token %d\n", name, token));
                int i;

                RTASBase->rtas_args.token = token;
                RTASBase->rtas_args.nargs = nargs;
                RTASBase->rtas_args.nret = nrets;
                RTASBase->rtas_args.rets = KrnVirtualToPhysical(&RTASBase->rtas_args.args[nargs]);

                for (i=0; i < nargs; i++)
                {
                        RTASBase->rtas_args.args[i] = va_arg(args, uint32_t);
                }

                Disable();

                {
                        register intptr_t args asm ("r4") = KrnVirtualToPhysical(&RTASBase->rtas_args);
                        register intptr_t base asm ("r5") = RTASBase->rtas_base;
                        register intptr_t entry asm ("r6") = RTASBase->rtas_entry;

                        asm volatile("li %%r3,%0; sc"::"i"(9 /*SC_RTAS*/),"r"(args),"r"(base),"r"(entry):"memory","r3");
                }

                if (nrets > 1 && output)
                {
                        for (i=0; i < nrets-1; i++)
                                output[i] = RTASBase->rtas_args.rets[i+1];
                }

                retval = nrets > 0 ? RTASBase->rtas_args.rets[0] : 0;

                D(bug("[RTAS] The call returned %d\n", retval));

                Enable();
        }
        else
                retval = -1;

        return retval;

        AROS_LIBFUNC_EXIT
}

ADD2INITLIB(rtas_init, 0)
